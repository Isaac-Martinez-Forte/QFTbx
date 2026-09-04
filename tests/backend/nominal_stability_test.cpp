// Tests of the Nichols-chart Nyquist criterion (Cohen-Chait-Yaniv) that
// completes the loop-shaping feasibility test (phase 8b.2b): classical
// textbook loops with known closed-loop stability verdicts.

#include <gtest/gtest.h>

#include <string>

#include <vector>

#include "src/core/math/point.h"

#include "src/core/loopshaping/nominal_stability_checker.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/parameter.h"

using namespace qftbx;

namespace {

LtiSystem* makeZpk(double k, std::initializer_list<double> zeros,
                   std::initializer_list<double> poles)
{
    std::vector<Parameter> nume;
    for (double z : zeros) {
        nume.push_back(Parameter(z));
    }
    std::vector<Parameter> deno;
    for (double p : poles) {
        deno.push_back(Parameter(p));
    }
    return new ZeroPoleGain(std::string("test"), nume, deno,
                            Parameter(k), Parameter(double(0)));
}

std::vector<double> designFrequencies{0.1, 1.0, 10.0};

TEST(NominalStability, LowGainOverAStablePlantIsStable)
{
    LtiSystem* plant = makeZpk(1.0, {}, {1.0, 2.0});
    LtiSystem* controller = makeZpk(1.0, {}, {});

    NominalStabilityChecker checker(plant, &designFrequencies);
    EXPECT_TRUE(checker.isNominallyStable(controller));

    delete plant;
    delete controller;
}

TEST(NominalStability, TriplePoleBeyondCriticalGainIsUnstable)
{
    // L0 = k/(s+1)^3 crosses -180 deg at w = sqrt(3) with |L0| = k/8:
    // the classical threshold is k = 8.
    LtiSystem* plant = makeZpk(1.0, {}, {1.0, 1.0, 1.0});
    LtiSystem* unstable = makeZpk(30.0, {}, {});
    LtiSystem* stable = makeZpk(4.0, {}, {});

    NominalStabilityChecker checker(plant, &designFrequencies);
    EXPECT_FALSE(checker.isNominallyStable(unstable));
    EXPECT_TRUE(checker.isNominallyStable(stable));

    delete plant;
    delete unstable;
    delete stable;
}

TEST(NominalStability, IntegratorLoopWithPhaseMarginIsStable)
{
    // L0 = 0.5/(s(s+1)): one integrator, positive phase margin.
    LtiSystem* plant = makeZpk(1.0, {}, {0.0, 1.0});
    LtiSystem* controller = makeZpk(0.5, {}, {});

    NominalStabilityChecker checker(plant, &designFrequencies);
    EXPECT_TRUE(checker.isNominallyStable(controller));

    delete plant;
    delete controller;
}

TEST(NominalStability, ConditionallyStableLoopNeedsTheNetCount)
{
    // L0 = k (s+1)^2 / (s+0.01)^3: at low frequency the phase dives past
    // -180 (three slow poles) and the zeros bring it back: with high gain
    // the two crossings cancel (net zero, stable); with the gain lowered
    // so that only the first crossing stays above 0 dB the loop is
    // unstable. The classical conditionally-stable example: a plain
    // "never cross -180 above 0 dB" rule gets BOTH verdicts wrong.
    LtiSystem* plant = makeZpk(1.0, {1.0, 1.0}, {0.01, 0.01, 0.01});
    LtiSystem* highGain = makeZpk(10.0, {}, {});
    LtiSystem* lowGain = makeZpk(0.005, {}, {});

    NominalStabilityChecker checker(plant, &designFrequencies);
    EXPECT_TRUE(checker.isNominallyStable(highGain));
    EXPECT_FALSE(checker.isNominallyStable(lowGain));

    delete plant;
    delete highGain;
    delete lowGain;
}

} // namespace
