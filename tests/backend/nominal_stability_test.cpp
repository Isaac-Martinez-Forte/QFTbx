// Tests of the Nichols-chart Nyquist criterion (Cohen-Chait-Yaniv) that
// completes the loop-shaping feasibility test (phase 8b.2b): classical
// textbook loops with known closed-loop stability verdicts.

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

//The criterion as a plain transcription: the loop sampled with complex
//arithmetic, the phase of every sample from atan2, unwrapped, refined
//where it steps too far, and the ray crossings counted on the unwrapped
//phase. The checker computes the same thing without the arc tangents and
//with the gain factored out; this is what it must agree with.
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
    //Random zero-pole-gain controllers over three plants of different
    //character, including gains that swing the verdict either way and a
    //loop with integrators that starts on a ray.
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
    //The zeros and poles fix the phase; the gain only scales the
    //magnitudes. One profile per shape, and the verdict for any gain is
    //read off it.
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
        EXPECT_EQ(checker.isNominallyStable(shape), NominalStabilityChecker::isStable(profile, k));
        verdicts.push_back(checker.isNominallyStable(shape));
    }
    EXPECT_TRUE(verdicts.front()) << "a low gain over a triple pole is stable";
    EXPECT_FALSE(verdicts.back()) << "a high gain over a triple pole is unstable";
    EXPECT_EQ(checker.statistics().profilesComputed, 1u) << "one shape, one profile";
    EXPECT_EQ(checker.statistics().verdicts, 12u);

    delete plant;
}

} // namespace
