// Characterisation tests of the natural interval extension (phase 8b.0
// safety net): the projection of a controller parameter box onto the
// Nichols plane that every interval loop-shaping algorithm relies on
// (thesis section 1.2.5). The fundamental containment property is the
// contract: the Nichols value of every controller instance inside the box
// must lie inside the projected box.

#include <gtest/gtest.h>

//A failed comparison of C-XSC values must report, not crash: see the header.
#include "tests/backend/cxsc_printing.h"

#include <cmath>
#include <complex>
#include <optional>
#include <vector>

#include <QPointF>
#include "src/core/range.h"
#include <QVector>

#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/parameter.h"

namespace {

using Complex = std::complex<qreal>;

//C(s) = k (s + z) / (s + p), evaluated by hand on the Nichols plane.
struct NicholsPoint {
    qreal magnitudeDb;
    qreal phaseDegrees;
};

NicholsPoint zpkAt(qreal k, qreal z, qreal p, qreal w, Complex p0)
{
    const Complex s(0.0, w);
    const Complex value = k * (s + z) / (s + p) * p0;
    qreal phase = std::arg(value) * 180.0 / M_PI;
    //nicholsBox maps phases onto the (-360, 0] branch, the Nichols convention
    //shared with the boundaries.
    if (phase > 0.0) {
        phase -= 360.0;
    }
    return {20.0 * std::log10(std::abs(value)), phase};
}

//An empty optional zero/pole stands for an empty vector (a pure-gain
//controller).
LtiSystem* makeZpk(Parameter k, std::optional<Parameter> z, std::optional<Parameter> p)
{
    std::vector<Parameter> nume;
    std::vector<Parameter> deno;
    if (z.has_value()) {
        nume.push_back(*z);
    }
    if (p.has_value()) {
        deno.push_back(*p);
    }
    return new ZeroPoleGain(QStringLiteral("test"), std::move(nume), std::move(deno),
                            std::move(k), Parameter(qreal(0)));
}

TEST(NaturalIntervalExtension, CertainControllerProjectsToAPoint)
{
    LtiSystem* controlador = makeZpk(Parameter(2.0), Parameter(3.0),
                                     Parameter(5.0));

    NaturalIntervalExtension extension;
    const cxsc::complex nominal(1.0, 0.0);
    const cxsc::cinterval box = extension.nicholsBox(controlador, 1.0, nominal);

    const NicholsPoint expected = zpkAt(2.0, 3.0, 5.0, 1.0, Complex(1.0, 0.0));

    EXPECT_NEAR(cxsc::_double(Inf(Re(box))), expected.magnitudeDb, 1e-9);
    EXPECT_NEAR(cxsc::_double(Sup(Re(box))), expected.magnitudeDb, 1e-9);
    EXPECT_NEAR(cxsc::_double(Inf(Im(box))), expected.phaseDegrees, 1e-9);
    EXPECT_NEAR(cxsc::_double(Sup(Im(box))), expected.phaseDegrees, 1e-9);

    delete controlador;
}

TEST(NaturalIntervalExtension, UncertainGainBoxContainsTheTrueExtremes)
{
    //k in [1, 10] with fixed zero/pole: only the gain is uncertain, so the
    //magnitude interval is exact and the true phase set crosses the 0/-360
    //branch cut (the true phase is -352.87 deg, i.e. +7.13 deg). The 8b.1
    //rewrite fixed the branch mapping, which returned the COMPLEMENTARY
    //arc [-273, -74] here (a containment violation, the historical bug
    //this test used to pin): a set crossing the cut now degrades to the
    //whole branch, conservative but correct.
    LtiSystem* controlador = makeZpk(Parameter("k", Range(1.0, 10.0), 1.0),
                                     Parameter(3.0), Parameter(5.0));

    NaturalIntervalExtension extension;
    const cxsc::cinterval box = extension.nicholsBox(controlador, 1.0,
                                                  cxsc::complex(1.0, 0.0));

    const NicholsPoint low = zpkAt(1.0, 3.0, 5.0, 1.0, Complex(1.0, 0.0));
    const NicholsPoint high = zpkAt(10.0, 3.0, 5.0, 1.0, Complex(1.0, 0.0));

    //Containment of the true extremes, in magnitude AND phase.
    EXPECT_LE(cxsc::_double(Inf(Re(box))), low.magnitudeDb);
    EXPECT_GE(cxsc::_double(Sup(Re(box))), high.magnitudeDb);
    EXPECT_LE(cxsc::_double(Inf(Im(box))), low.phaseDegrees);
    EXPECT_GE(cxsc::_double(Sup(Im(box))), low.phaseDegrees);

    //Pinned current hull (any tightening or widening should be deliberate):
    //the magnitude is the exact true range; the phase is the whole branch.
    EXPECT_NEAR(cxsc::_double(Inf(Re(box))), low.magnitudeDb, 1e-9);
    EXPECT_NEAR(cxsc::_double(Sup(Re(box))), high.magnitudeDb, 1e-9);
    EXPECT_NEAR(cxsc::_double(Inf(Im(box))), -360.0, 1e-9);
    EXPECT_NEAR(cxsc::_double(Sup(Im(box))), 0.0, 1e-9);

    delete controlador;
}

TEST(NaturalIntervalExtension, PureGainControllerProjectsExactly)
{
    //A controller with no zeros and no poles (the thesis benchmarks with
    //n = 1): both products are the neutral 1. The historical code left
    //them UNINITIALIZED and read garbage.
    LtiSystem* controlador = makeZpk(Parameter(2.0), std::nullopt, std::nullopt);

    NaturalIntervalExtension extension;
    const cxsc::cinterval box = extension.nicholsBox(controlador, 1.0,
                                                     cxsc::complex(1.0, 0.0));

    EXPECT_NEAR(cxsc::_double(Inf(Re(box))), 20.0 * std::log10(2.0), 1e-9);
    EXPECT_NEAR(cxsc::_double(Sup(Re(box))), 20.0 * std::log10(2.0), 1e-9);
    //A positive real value sits ON the branch cut: its phase is the branch
    //endpoint pair, enclosed conservatively.
    EXPECT_GE(cxsc::_double(Sup(Im(box))), -360.0);

    delete controlador;
}

TEST(NaturalIntervalExtension, HugeBoxesStayFiniteInDecibels)
{
    //The widest search box of the thesis benchmarks: the interval products
    //must never reach log10(0) or log10(inf) (fi_lib aborts the process on
    //both; observed live through the NK crash).
    LtiSystem* controlador = makeZpk(Parameter("kc", Range(1e-9, 1e8), 1.0),
                                     Parameter("z1", Range(1e-9, 1e4), 1.0),
                                     Parameter("p1", Range(1e-9, 1e4), 1.0));

    NaturalIntervalExtension extension;
    const cxsc::cinterval box = extension.nicholsBox(controlador, 100.0,
                                                     cxsc::complex(1e-8, -1e8));

    EXPECT_TRUE(std::isfinite(cxsc::_double(Inf(Re(box)))));
    EXPECT_TRUE(std::isfinite(cxsc::_double(Sup(Re(box)))));
    EXPECT_TRUE(std::isfinite(cxsc::_double(Inf(Im(box)))));
    EXPECT_TRUE(std::isfinite(cxsc::_double(Sup(Im(box)))));

    delete controlador;
}

TEST(NaturalIntervalExtension, SampledInstancesStayInsideTheBox)
{
    //The fundamental theorem of interval analysis: every instance of the
    //box maps inside the projection.
    LtiSystem* controlador = makeZpk(Parameter("k", Range(0.5, 4.0), 1.0),
                                     Parameter("z", Range(1.0, 6.0), 2.0),
                                     Parameter("p", Range(2.0, 9.0), 3.0));

    NaturalIntervalExtension extension;
    const qreal w = 2.0;
    const Complex p0Value(0.8, -0.4);
    const cxsc::cinterval box =
        extension.nicholsBox(controlador, w, cxsc::complex(0.8, -0.4));

    const qreal magLo = cxsc::_double(Inf(Re(box)));
    const qreal magHi = cxsc::_double(Sup(Re(box)));
    const qreal phaseLo = cxsc::_double(Inf(Im(box)));
    const qreal phaseHi = cxsc::_double(Sup(Im(box)));

    for (qreal k : {0.5, 1.7, 4.0}) {
        for (qreal z : {1.0, 3.3, 6.0}) {
            for (qreal p : {2.0, 5.1, 9.0}) {
                const NicholsPoint point = zpkAt(k, z, p, w, p0Value);
                EXPECT_LE(magLo - 1e-9, point.magnitudeDb)
                    << "k=" << k << " z=" << z << " p=" << p;
                EXPECT_GE(magHi + 1e-9, point.magnitudeDb)
                    << "k=" << k << " z=" << z << " p=" << p;
                EXPECT_LE(phaseLo - 1e-9, point.phaseDegrees)
                    << "k=" << k << " z=" << z << " p=" << p;
                EXPECT_GE(phaseHi + 1e-9, point.phaseDegrees)
                    << "k=" << k << " z=" << z << " p=" << p;
            }
        }
    }

    delete controlador;
}

} // namespace
