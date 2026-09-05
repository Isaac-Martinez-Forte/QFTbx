// Characterisation tests of the natural interval extension (phase 8b.0
// safety net): the projection of a controller parameter box onto the
// Nichols plane that every interval loop-shaping algorithm relies on
// (thesis section 1.2.5). The fundamental containment property is the
// contract: the Nichols value of every controller instance inside the box
// must lie inside the projected box.

#include <gtest/gtest.h>

#include <string>


#include <cmath>
#include <complex>
#include <optional>
#include <vector>

#include "src/core/math/point.h"
#include "src/core/math/range.h"

#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/parameter.h"

using namespace qftbx;

namespace {

using Complex = std::complex<double>;

//C(s) = k (s + z) / (s + p), evaluated by hand on the Nichols plane.
struct NicholsPoint {
    double magnitudeDb;
    double phaseDegrees;
};

NicholsPoint zpkAt(double k, double z, double p, double w, Complex p0)
{
    const Complex s(0.0, w);
    const Complex value = k * (s + z) / (s + p) * p0;
    double phase = std::arg(value) * 180.0 / M_PI;
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
    return new ZeroPoleGain(std::string("test"), std::move(nume), std::move(deno),
                            std::move(k), Parameter(double(0)));
}

TEST(NaturalIntervalExtension, CertainControllerProjectsToAPoint)
{
    LtiSystem* controller = makeZpk(Parameter(2.0), Parameter(3.0),
                                     Parameter(5.0));

    NaturalIntervalExtension extension;
    const std::complex<double> nominal(1.0, 0.0);
    const NicholsBox box = extension.nicholsBox(controller, 1.0, nominal);

    const NicholsPoint expected = zpkAt(2.0, 3.0, 5.0, 1.0, Complex(1.0, 0.0));

    EXPECT_NEAR(box.magnitudeDb.lower(), expected.magnitudeDb, 1e-9);
    EXPECT_NEAR(box.magnitudeDb.upper(), expected.magnitudeDb, 1e-9);
    EXPECT_NEAR(box.phaseDegrees.lower(), expected.phaseDegrees, 1e-9);
    EXPECT_NEAR(box.phaseDegrees.upper(), expected.phaseDegrees, 1e-9);

    delete controller;
}

TEST(NaturalIntervalExtension, UncertainGainBoxContainsTheTrueExtremes)
{
    //k in [1, 10] with fixed zero/pole: only the gain is uncertain, so the
    //magnitude interval is exact and the phase is a single value, -352.87
    //deg (i.e. +7.13 deg), which a positive gain does not move. The
    //historical branch mapping returned the COMPLEMENTARY arc [-273, -74]
    //here, a containment violation; the rectangle product that replaced it
    //could only answer with the whole branch; the polar form reads the
    //phase exactly.
    LtiSystem* controller = makeZpk(Parameter("k", Range(1.0, 10.0), 1.0),
                                     Parameter(3.0), Parameter(5.0));

    NaturalIntervalExtension extension;
    const NicholsBox box = extension.nicholsBox(controller, 1.0,
                                                  std::complex<double>(1.0, 0.0));

    const NicholsPoint low = zpkAt(1.0, 3.0, 5.0, 1.0, Complex(1.0, 0.0));
    const NicholsPoint high = zpkAt(10.0, 3.0, 5.0, 1.0, Complex(1.0, 0.0));

    //Containment of the true extremes, in magnitude AND phase.
    EXPECT_LE(box.magnitudeDb.lower(), low.magnitudeDb);
    EXPECT_GE(box.magnitudeDb.upper(), high.magnitudeDb);
    EXPECT_LE(box.phaseDegrees.lower(), low.phaseDegrees);
    EXPECT_GE(box.phaseDegrees.upper(), low.phaseDegrees);

    //Pinned current hull (any tightening or widening should be deliberate):
    //the magnitude is the exact true range and the phase the true value,
    //up to the outward rounding of the arithmetic.
    EXPECT_NEAR(box.magnitudeDb.lower(), low.magnitudeDb, 1e-9);
    EXPECT_NEAR(box.magnitudeDb.upper(), high.magnitudeDb, 1e-9);
    EXPECT_NEAR(box.phaseDegrees.lower(), low.phaseDegrees, 1e-9);
    EXPECT_NEAR(box.phaseDegrees.upper(), low.phaseDegrees, 1e-9);

    delete controller;
}

TEST(NaturalIntervalExtension, PureGainControllerProjectsExactly)
{
    //A controller with no zeros and no poles (the thesis benchmarks with
    //n = 1): both products are the neutral 1. The historical code left
    //them UNINITIALIZED and read garbage.
    LtiSystem* controller = makeZpk(Parameter(2.0), std::nullopt, std::nullopt);

    NaturalIntervalExtension extension;
    const NicholsBox box = extension.nicholsBox(controller, 1.0,
                                                     std::complex<double>(1.0, 0.0));

    EXPECT_NEAR(box.magnitudeDb.lower(), 20.0 * std::log10(2.0), 1e-9);
    EXPECT_NEAR(box.magnitudeDb.upper(), 20.0 * std::log10(2.0), 1e-9);
    //A positive real value sits ON the branch cut: its phase is the branch
    //endpoint pair, enclosed conservatively.
    EXPECT_GE(box.phaseDegrees.upper(), -360.0);

    delete controller;
}

TEST(NaturalIntervalExtension, HugeBoxesStayFiniteInDecibels)
{
    //The widest search box of the thesis benchmarks: the interval products
    //must never reach log10(0) or log10(inf) (fi_lib aborts the process on
    //both; observed live through the NK crash).
    LtiSystem* controller = makeZpk(Parameter("kc", Range(1e-9, 1e8), 1.0),
                                     Parameter("z1", Range(1e-9, 1e4), 1.0),
                                     Parameter("p1", Range(1e-9, 1e4), 1.0));

    NaturalIntervalExtension extension;
    const NicholsBox box = extension.nicholsBox(controller, 100.0,
                                                     std::complex<double>(1e-8, -1e8));

    EXPECT_TRUE(std::isfinite(box.magnitudeDb.lower()));
    EXPECT_TRUE(std::isfinite(box.magnitudeDb.upper()));
    EXPECT_TRUE(std::isfinite(box.phaseDegrees.lower()));
    EXPECT_TRUE(std::isfinite(box.phaseDegrees.upper()));

    delete controller;
}

TEST(NaturalIntervalExtension, SampledInstancesStayInsideTheBox)
{
    //The fundamental theorem of interval analysis: every instance of the
    //box maps inside the projection.
    LtiSystem* controller = makeZpk(Parameter("k", Range(0.5, 4.0), 1.0),
                                     Parameter("z", Range(1.0, 6.0), 2.0),
                                     Parameter("p", Range(2.0, 9.0), 3.0));

    NaturalIntervalExtension extension;
    const double w = 2.0;
    const Complex p0Value(0.8, -0.4);
    const NicholsBox box =
        extension.nicholsBox(controller, w, std::complex<double>(0.8, -0.4));

    const double magLo = box.magnitudeDb.lower();
    const double magHi = box.magnitudeDb.upper();
    const double phaseLo = box.phaseDegrees.lower();
    const double phaseHi = box.phaseDegrees.upper();

    for (double k : {0.5, 1.7, 4.0}) {
        for (double z : {1.0, 3.3, 6.0}) {
            for (double p : {2.0, 5.1, 9.0}) {
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

    delete controller;
}

} // namespace
