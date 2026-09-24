/**
 * @file
 * @brief Tests of the Nichols-chart Nyquist criterion for nominal stability.
 *
 * The checker completes the feasibility test with the criterion of Cohen,
 * Chait and Yaniv: crossings of the -180 degree rays above 0 dB, counted on
 * the unwrapped phase. Classical loops with known verdicts are checked, among
 * them the conditionally stable loop whose two crossings cancel, where a
 * never-cross rule gets both verdicts wrong; random controllers over three
 * plants, a double integrator among them, must agree with the roots of the
 * closed-loop characteristic polynomial wherever the loop is strictly proper
 * (a loop that does not roll off is refused by the checker whatever its
 * roots); one phase profile serves every gain
 * of a shape; a box is rejected
 * whole only when its corner is unstable and its enclosure excludes the
 * critical point at every frequency; plants with right-half-plane poles have
 * verdicts fixed by Routh; a delay in the denominator gets no verdict; and
 * the ACC'90 double integrator, whose loop starts on the ray at infinite
 * magnitude, rejects the designs the search had returned and accepts a lead.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <random>
#include <string>
#include <vector>

#include "src/core/math/constants.h"

#include "src/core/math/point.h"
#include "src/core/math/polynomial.h"
#include <limits>
#include <optional>

#include "src/core/loopshaping/common/nominal_stability_checker.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/free_form.h"
#include "src/core/common/exception.h"
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
    LtiSystem* plant = makeZpk(1.0, {}, {0.0, 1.0});
    LtiSystem* controller = makeZpk(0.5, {}, {});

    NominalStabilityChecker checker(plant, &designFrequencies);
    EXPECT_TRUE(checker.isNominallyStable(controller));

    delete plant;
    delete controller;
}

TEST(NominalStability, ConditionallyStableLoopNeedsTheNetCount)
{
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

double lastWorstRealPart = 0.0;

std::optional<bool> rootsVerdict(LtiSystem * plant, const PointController & controller)
{
    std::vector<double> numerator, denominator;
    for (Parameter & p : plant->numerator()) numerator.push_back(p.nominal());
    for (Parameter & p : plant->denominator()) denominator.push_back(p.nominal());
    const std::optional<LtiSystem::Polynomials> P = plant->polynomialsAt(numerator, denominator, plant->gain().nominal());
    std::vector<double> C_num{controller.gain}, C_den{1.0};
    for (double z : controller.zeros) C_num = qftbx::math::polynomialProduct(C_num, {1.0, z});
    for (double p : controller.poles) C_den = qftbx::math::polynomialProduct(C_den, {1.0, p});
    const std::vector<double> characteristic = qftbx::math::polynomialSum(
                qftbx::math::polynomialProduct(P->numerator, C_num), qftbx::math::polynomialProduct(P->denominator, C_den));
    double worst = -std::numeric_limits<double>::infinity();
    for (const std::complex<double> & root : qftbx::math::polynomialRoots(characteristic)) worst = std::max(worst, root.real());
    lastWorstRealPart = worst;
    if (std::abs(worst) < 1e-6) return std::nullopt;
    return worst < 0.0;
}

TEST(NominalStability, TheVerdictAgreesWithTheClosedLoopRoots)
{
    const Settings::Stability tolerances;
    std::vector<LtiSystem *> plants{
        makeZpk(1.0, {}, {0.0, 1.0, 3.0}),
        makeZpk(1.0, {1.0, 1.0}, {0.01, 0.01, 0.01}),
        makeZpk(1.0, {}, {0.0, 0.0, 2.0}),
    };

    std::mt19937 generator(13);
    std::uniform_real_distribution<double> gain(0.001, 1000.0);
    std::uniform_real_distribution<double> zero(0.05, 50.0);
    std::uniform_real_distribution<double> pole(0.5, 500.0);
    std::uniform_int_distribution<int> count(0, 2);

    int stable = 0;
    int total = 0;
    for (LtiSystem * plant : plants) {
        NominalStabilityChecker checker(plant, &designFrequencies, tolerances);
        for (int i = 0; i < 400; ++i) {
            PointController point;
            point.gain = gain(generator);
            for (int z = count(generator); z > 0; --z) point.zeros.push_back(zero(generator));
            for (int p = count(generator); p > 0; --p) point.poles.push_back(pole(generator));
            const std::ptrdiff_t relativeDegree =
                    static_cast<std::ptrdiff_t>(plant->denominator().size() + point.poles.size())
                    - static_cast<std::ptrdiff_t>(plant->numerator().size() + point.zeros.size());
            if (relativeDegree <= 0) continue;
            const std::optional<bool> expected = rootsVerdict(plant, point);
            if (!expected.has_value()) continue;
            EXPECT_EQ(checker.isNominallyStable(point), *expected)
                << "plant " << plant->expression() << " k=" << point.gain
                << " zeros " << ::testing::PrintToString(point.zeros) << " poles " << ::testing::PrintToString(point.poles)
                << " worst real part of the closed loop " << lastWorstRealPart;
            stable += *expected;
            ++total;
        }
        delete plant;
    }
    EXPECT_GT(stable, total / 10) << "the sample must exercise both verdicts";
    EXPECT_LT(stable, total - total / 10);
}

TEST(NominalStability, AProfileServesEveryGainOfAShape)
{
    LtiSystem * plant = makeZpk(1.0, {}, {1.0, 1.0, 1.0});
    NominalStabilityChecker checker(plant, &designFrequencies);

    PointController shape;
    shape.zeros = {2.0};
    shape.poles = {20.0};
    const NominalStabilityChecker::Profile & profile = checker.profileOf(shape);
    EXPECT_TRUE(profile.decided);

    std::vector<bool> verdicts;
    for (double k : {0.5, 2.0, 8.0, 30.0, 120.0, 500.0}) {
        shape.gain = k;
        EXPECT_EQ(checker.isNominallyStable(shape), checker.isStable(profile, k));
        verdicts.push_back(checker.isNominallyStable(shape));
    }
    EXPECT_TRUE(verdicts.front()) << "a low gain over a triple pole is stable";
    EXPECT_FALSE(verdicts.back()) << "a high gain over a triple pole is unstable";
    EXPECT_EQ(checker.statistics().profilesComputed, 1u) << "one shape, one profile";
    EXPECT_EQ(checker.statistics().verdicts, 12u);

    delete plant;
}

}

namespace {

LtiSystem* makeBox(Range k, Range z, Range p)
{
    std::vector<Parameter> nume{Parameter("z1", z, z.min)};
    std::vector<Parameter> deno{Parameter("p1", p, p.min)};
    return new ZeroPoleGain(std::string("box"), nume, deno, Parameter("kc", k, k.min), Parameter(double(0)));
}

}

TEST(NominalStability, ALagBoxUnstableThroughoutIsRejectedWhole)
{
    LtiSystem* plant = makeZpk(1.0, {}, {0.0, 1.0});
    std::vector<double> omega{0.1, 0.5, 1.0, 2.0, 15.0, 100.0};
    NominalStabilityChecker checker(plant, &omega);
    NaturalIntervalExtension extension;

    LtiSystem* lag = makeBox(Range(10.0, 20.0), Range(500.0, 1000.0), Range(0.01, 0.1));
    EXPECT_TRUE(checker.isBoxUnstable(lag, extension));

    LtiSystem* good = makeBox(Range(400.0, 700.0), Range(1.5, 2.5), Range(100.0, 200.0));
    EXPECT_FALSE(checker.isBoxUnstable(good, extension));

    LtiSystem* straddling = makeBox(Range(1.0, 100.0), Range(1.5, 3.0), Range(0.01, 3.0));
    EXPECT_FALSE(checker.isBoxUnstable(straddling, extension));

    delete plant;
    delete lag;
    delete good;
    delete straddling;
}

TEST(NominalStability, OneUnstablePoleNeedsTheGainAboveOne)
{
    LtiSystem* plant = makeZpk(1.0, {}, {-1.0});
    NominalStabilityChecker checker(plant, &designFrequencies);
    EXPECT_EQ(checker.rightHalfPlanePoles(), 1);

    LtiSystem* low = makeZpk(0.5, {}, {});
    LtiSystem* high = makeZpk(2.0, {}, {});
    EXPECT_FALSE(checker.isNominallyStable(low));
    EXPECT_TRUE(checker.isNominallyStable(high));

    delete plant;
    delete low;
    delete high;
}

TEST(NominalStability, AnUnstablePoleOfAFreeFormPlant)
{
    std::vector<Parameter> none;
    std::vector<Parameter> denominator{Parameter(std::string("a"), 2.5)};
    FreeForm plant(std::string("P"), none, denominator, Parameter(1.0), Parameter(0.0),
                   std::string("1"), std::string("s^2-a"));

    NominalStabilityChecker checker(&plant, &designFrequencies);
    EXPECT_EQ(checker.rightHalfPlanePoles(), 1);

    LtiSystem* below = makeZpk(20.0, {1.0}, {10.0});
    LtiSystem* above = makeZpk(30.0, {1.0}, {10.0});
    EXPECT_FALSE(checker.isNominallyStable(below));
    EXPECT_TRUE(checker.isNominallyStable(above));

    delete below;
    delete above;
}

TEST(NominalStability, PolesOnTheAxisAreIndentedFromTheExactRoots)
{
    std::vector<Parameter> none;
    std::vector<Parameter> denominator{Parameter(std::string("a"), 430.25)};
    FreeForm plant(std::string("P"), none, denominator, Parameter(877.5), Parameter(0.0),
                   std::string("1"), std::string("s^2+a"));

    NominalStabilityChecker checker(&plant, &designFrequencies);
    EXPECT_EQ(checker.rightHalfPlanePoles(), 0);

    LtiSystem* fromNk = makeZpk(0.5574, {500.005}, {0.08891485964165118, 1.5811546414724906});
    LtiSystem* fromMc2 = makeZpk(2.2659, {0.06007}, {0.01095, 0.04099});
    LtiSystem* stabilising = makeZpk(10000.0, {23.3}, {228.5, 554.1});
    EXPECT_FALSE(checker.isNominallyStable(fromNk));
    EXPECT_FALSE(checker.isNominallyStable(fromMc2));
    EXPECT_TRUE(checker.isNominallyStable(stabilising));

    delete fromNk;
    delete fromMc2;
    delete stabilising;
}

TEST(NominalStability, ADenominatorThatIsNotAPolynomialGetsNoVerdict)
{
    std::vector<Parameter> none;
    FreeForm plant(std::string("P"), none, none, Parameter(1.0), Parameter(0.0),
                   std::string("1"), std::string("s+exp(-s)"));
    EXPECT_THROW(NominalStabilityChecker checker(&plant, &designFrequencies), qftbx::InvalidInput);
}

TEST(NominalStability, ADoubleIntegratorStartsOnTheRayAtInfiniteMagnitude)
{
    std::vector<Parameter> ev{Parameter(std::string("ev"), 0.5)};
    FreeForm plant(std::string("ACC90"), ev, ev, Parameter(1.0), Parameter(0.0),
                   std::string("ev"), std::string("s^2*(s^2+0.02*s+2*ev)"));
    std::vector<double> frequencies{0.1, 0.98, 1.02, 1.1, 1.2, 1.5, 2.0, 3.0, 5.0, 8.0, 12.0, 30.0};
    NominalStabilityChecker checker(&plant, &frequencies);
    EXPECT_EQ(checker.rightHalfPlanePoles(), 0);

    LtiSystem* gainOnly = makeZpk(1000.0, {0.01}, {0.01});
    LtiSystem* returnedOnce = makeZpk(1000.0, {28.00666428}, {0.01});
    LtiSystem* returnedTwice = makeZpk(1000.0, {500.005}, {0.01});
    LtiSystem* stabilising = makeZpk(0.03, {0.1}, {1.0});
    EXPECT_FALSE(checker.isNominallyStable(gainOnly)) << "every closed-loop pole of k/(s^2 ...) with k = 1000 is in the right half-plane";
    EXPECT_FALSE(checker.isNominallyStable(returnedOnce));
    EXPECT_FALSE(checker.isNominallyStable(returnedTwice));
    EXPECT_TRUE(checker.isNominallyStable(stabilising)) << "a lead below the resonance stabilises the whole family";

    delete gainOnly;
    delete returnedOnce;
    delete returnedTwice;
    delete stabilising;
}
