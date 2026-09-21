/**
 * @file
 * @brief Tests of the Nichols-chart Nyquist criterion for nominal stability.
 *
 * The checker completes the feasibility test with the criterion of Cohen,
 * Chait and Yaniv: crossings of the -180 degree rays above 0 dB, counted on
 * the unwrapped phase. Classical loops with known verdicts are checked, among
 * them the conditionally stable loop whose two crossings cancel, where a
 * never-cross rule gets both verdicts wrong; random controllers over three
 * plants must agree with a transcription of the criterion through arc
 * tangents; one phase profile serves every gain of a shape; a box is rejected
 * whole only when its corner is unstable and its enclosure excludes the
 * critical point at every frequency; plants with right-half-plane poles have
 * verdicts fixed by Routh; and a delay in the denominator gets no verdict.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <random>
#include <string>
#include <vector>

#include "src/core/math/constants.h"

#include "src/core/math/point.h"

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

bool referenceVerdict(LtiSystem * plant, const PointController & controller,
                      const std::vector<double> & grid, const Settings::Stability & tolerances)
{
    struct Sample { double w; std::complex<double> loop; double phase; };
    const auto loopAt = [&](double w) {
        const std::complex<double> jw(0.0, w);
        std::complex<double> value(controller.gain, 0.0);
        for (double z : controller.zeros) value *= jw + std::complex<double>(z, 0.0);
        for (double p : controller.poles) value /= jw + std::complex<double>(p, 0.0);
        return value * plant->evaluate(w);
    };
    const auto phaseOf = [](std::complex<double> v) { return std::arg(v) * 180.0 / qftbx::math::kPi; };

    std::vector<Sample> curve;
    for (double w : grid) {
        const std::complex<double> loop = loopAt(w);
        curve.push_back({w, loop, phaseOf(loop)});
    }
    int budget = tolerances.refinementBudget;
    for (std::size_t i = 0; i + 1 < curve.size() && budget > 0;) {
        double step = std::abs(curve[i + 1].phase - curve[i].phase);
        if (step > 180.0) step = 360.0 - step;
        if (step > tolerances.maxPhaseStepDegrees && curve[i + 1].w - curve[i].w > 1e-12 * curve[i].w) {
            const double w = std::sqrt(curve[i].w * curve[i + 1].w);
            const std::complex<double> loop = loopAt(w);
            curve.insert(curve.begin() + static_cast<std::ptrdiff_t>(i) + 1, {w, loop, phaseOf(loop)});
            --budget;
        } else {
            ++i;
        }
    }
    if (budget <= 0) return false;
    if (std::abs(curve.back().loop) >= 1.0) return false;

    std::vector<double> unwrapped(curve.size(), 0.0);
    unwrapped[0] = curve[0].phase;
    for (std::size_t i = 1; i < curve.size(); ++i) {
        double delta = curve[i].phase - curve[i - 1].phase;
        if (delta > 180.0) delta -= 360.0; else if (delta < -180.0) delta += 360.0;
        unwrapped[i] = unwrapped[i - 1] + delta;
    }
    const auto rayBelow = [](double phase) { return std::floor((phase + 180.0) / 360.0) * 360.0 - 180.0; };
    double crossings = 0.0;
    const double startRay = rayBelow(unwrapped[0] + 1e-6);
    if (std::abs(unwrapped[0] - startRay) < 1e-3 && std::abs(curve[0].loop) > 1.0) {
        std::size_t next = 1;
        while (next + 1 < curve.size() && std::abs(unwrapped[next] - unwrapped[0]) < 1e-9) ++next;
        crossings += (unwrapped[next] > unwrapped[0]) ? 0.5 : -0.5;
    }
    for (std::size_t i = 0; i + 1 < curve.size(); ++i) {
        const double a = unwrapped[i], b = unwrapped[i + 1];
        if (a == b) continue;
        const double low = std::min(a, b), high = std::max(a, b), sign = (b > a) ? 1.0 : -1.0;
        for (double level = rayBelow(high); level > low; level -= 360.0) {
            if (level >= high) continue;
            const double t = (level - a) / (b - a);
            const double from = std::abs(curve[i].loop), to = std::abs(curve[i + 1].loop);
            if (from * std::pow(to / from, t) > 1.0) crossings += sign;
        }
    }
    return std::abs(crossings) < 0.25;
}

std::vector<double> gridOf(const std::vector<double> & omega, const Settings::Stability & tolerances)
{
    double minOmega = omega.front(), maxOmega = omega.front();
    for (double o : omega) { minOmega = std::min(minOmega, o); maxOmega = std::max(maxOmega, o); }
    const double logFrom = std::log10(minOmega) - tolerances.decadesBeyond;
    const double logTo = std::log10(maxOmega) + tolerances.decadesBeyond;
    std::vector<double> grid;
    for (int i = 0; i < tolerances.baseGridPoints; ++i) {
        grid.push_back(std::pow(10.0, logFrom + (logTo - logFrom) * i / (tolerances.baseGridPoints - 1)));
    }
    return grid;
}

TEST(NominalStability, TheArrayVerdictAgreesWithTheTranscribedCriterion)
{
    const Settings::Stability tolerances;
    const std::vector<double> grid = gridOf(designFrequencies, tolerances);
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
            const bool expected = referenceVerdict(plant, point, grid, tolerances);
            EXPECT_EQ(checker.isNominallyStable(point), expected)
                << "plant " << plant->expression() << " k=" << point.gain;
            stable += expected;
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
