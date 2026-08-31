// Characterisation tests of the natural interval extension (phase 8b.0
// safety net): the projection of a controller parameter box onto the
// Nichols plane that every interval loop-shaping algorithm relies on
// (thesis section 1.2.5). The fundamental containment property is the
// contract: the Nichols value of every controller instance inside the box
// must lie inside the projected box.

#include <gtest/gtest.h>

#include <cmath>
#include <complex>

#include <QPointF>
#include <QVector>

#include "Modelo/LoopShaping/NaturalIntervalExtension/natural_interval_extension.h"
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
    //get_box maps phases onto the (-360, 0] branch, the Nichols convention
    //shared with the boundaries.
    if (phase > 0.0) {
        phase -= 360.0;
    }
    return {20.0 * std::log10(std::abs(value)), phase};
}

LtiSystem* makeZpk(Parameter* k, Parameter* z, Parameter* p)
{
    auto* nume = new QVector<Parameter*>{z};
    auto* deno = new QVector<Parameter*>{p};
    return new ZeroPoleGain(QStringLiteral("test"), nume, deno, k,
                            new Parameter(qreal(0)));
}

TEST(NaturalIntervalExtension, CertainControllerProjectsToAPoint)
{
    LtiSystem* controlador = makeZpk(new Parameter(2.0), new Parameter(3.0),
                                     new Parameter(5.0));

    Natura_Interval_extension extension;
    const cxsc::complex nominal(1.0, 0.0);
    const cxsc::cinterval box = extension.get_box(controlador, 1.0, nominal, false);

    const NicholsPoint expected = zpkAt(2.0, 3.0, 5.0, 1.0, Complex(1.0, 0.0));

    EXPECT_NEAR(cxsc::_double(Inf(Re(box))), expected.magnitudeDb, 1e-9);
    EXPECT_NEAR(cxsc::_double(Sup(Re(box))), expected.magnitudeDb, 1e-9);
    EXPECT_NEAR(cxsc::_double(Inf(Im(box))), expected.phaseDegrees, 1e-9);
    EXPECT_NEAR(cxsc::_double(Sup(Im(box))), expected.phaseDegrees, 1e-9);

    delete controlador;
}

TEST(NaturalIntervalExtension, UncertainGainBoxContainsAndOverestimates)
{
    //k in [1, 10]. The TRUE image is a +20 dB magnitude segment at constant
    //phase; the natural extension multiplies interval-by-rectangle and
    //returns the interval HULL, so the box legitimately overestimates BOTH
    //axes (the conservatism the thesis notes in section 3.1). The contract
    //is containment; the exact current hull is pinned as characterisation.
    LtiSystem* controlador = makeZpk(new Parameter("k", QPointF(1.0, 10.0), 1.0),
                                     new Parameter(3.0), new Parameter(5.0));

    Natura_Interval_extension extension;
    const cxsc::cinterval box = extension.get_box(controlador, 1.0,
                                                  cxsc::complex(1.0, 0.0), false);

    const NicholsPoint low = zpkAt(1.0, 3.0, 5.0, 1.0, Complex(1.0, 0.0));
    const NicholsPoint high = zpkAt(10.0, 3.0, 5.0, 1.0, Complex(1.0, 0.0));

    //Containment of the true extremes holds in magnitude...
    EXPECT_LE(cxsc::_double(Inf(Re(box))), low.magnitudeDb);
    EXPECT_GE(cxsc::_double(Sup(Re(box))), high.magnitudeDb);

    // BUG: ...but NOT in phase. The true instance phase (-352.87 deg, i.e.
    // +7.13 deg) lies outside the box's phase interval [-273.04, -73.96]
    // even modulo 360. The raw theta1 - theta2 interval DOES contain it
    // ([-9.4, 62] deg), so the branch mapping applied afterwards in
    // get_box() mangles the interval. A real real-positive gain must not
    // touch the phase at all. To dissect in 8b.1.
    EXPECT_GT(cxsc::_double(Inf(Im(box))), low.phaseDegrees); //violation pinned

    //Pinned current hull (any tightening or widening should be deliberate).
    EXPECT_NEAR(cxsc::_double(Inf(Re(box))), -10.714931765381902, 1e-9);
    EXPECT_NEAR(cxsc::_double(Sup(Re(box))), 18.353157742768811, 1e-9);
    EXPECT_NEAR(cxsc::_double(Inf(Im(box))), -273.04477844419398, 1e-9);
    EXPECT_NEAR(cxsc::_double(Sup(Im(box))), -73.95966089697427, 1e-9);

    delete controlador;
}

TEST(NaturalIntervalExtension, SampledInstancesStayInsideTheBox)
{
    //The fundamental theorem of interval analysis: every instance of the
    //box maps inside the projection.
    LtiSystem* controlador = makeZpk(new Parameter("k", QPointF(0.5, 4.0), 1.0),
                                     new Parameter("z", QPointF(1.0, 6.0), 2.0),
                                     new Parameter("p", QPointF(2.0, 9.0), 3.0));

    Natura_Interval_extension extension;
    const qreal w = 2.0;
    const Complex p0Value(0.8, -0.4);
    const cxsc::cinterval box =
        extension.get_box(controlador, w, cxsc::complex(0.8, -0.4), false);

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
