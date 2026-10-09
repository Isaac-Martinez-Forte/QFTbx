/**
 * @file
 * @brief The contraction of a box's gain removes only gains it can prove
 * wrong.
 *
 * The Routh table over pieces of the gain shaves the ceiling of a stable
 * plant to its stability limit, 1/(s(s+1)(s+2)) with a pure gain being
 * stable below 6, and raises the floor of an unstable one, 1/(s-1) needing
 * a gain above 1, each to within the resolution of the pieces and never
 * past the limit. On the published problems, over random boxes of
 * controllers, no controller with a gain the shaving removed is stable with
 * every plant of the sweep and the nominal one, and no controller with a
 * gain the whole contraction removed, by the specifications or by the
 * stability, both meets every specification and closes the loop stably, and
 * the specifications contract a box as much as the exact phase and modulus
 * of its corners allow, also where its phase crosses a whole turn. The
 * zero exclusion proves unstable, on the ACC'90 plant, boxes whose zero and
 * pole nearly cancel, where the Routh table proves nothing; and on problems
 * with stable controllers no box it proves holds one.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <complex>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/exact_point_check.h"
#include "src/core/loopshaping/common/family_stability_checker.h"
#include "src/core/loopshaping/common/gain_contractor.h"
#include "src/core/math/range.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/system/parameter.h"
#include "src/core/system/zero_pole_gain.h"
#include "tests/backend/published_problems.h"

using namespace qftbx;
using namespace qftbx_tests;

namespace {

std::unique_ptr<LtiSystem> plantWithPoles(const std::vector<double> & poles)
{
    std::vector<Parameter> denominator;
    for (const double pole : poles) {
        denominator.emplace_back(pole);
    }
    return std::make_unique<ZeroPoleGain>(std::string("plant"), std::vector<Parameter>{}, denominator,
                                          Parameter(1.0), Parameter(0.0));
}

std::unique_ptr<LtiSystem> pureGain(Range gains)
{
    return std::make_unique<ZeroPoleGain>(std::string("gain"), std::vector<Parameter>{}, std::vector<Parameter>{},
                                          Parameter(std::string("k"), gains, gains.min), Parameter(0.0));
}

std::vector<double> removedGains(Range box, Range kept, std::mt19937 & generator)
{
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::vector<double> gains;
    for (int k = 0; k < 5; ++k) {
        if (box.min < kept.min) {
            gains.push_back(box.min * std::pow(kept.min / box.min, unit(generator) * 0.999));
        }
        if (kept.max < box.max) {
            gains.push_back(box.max * std::pow(kept.max / box.max, unit(generator) * 0.999));
        }
    }
    return gains;
}

}

TEST(GainShave, TheRouthTableShavesTheCeilingOfAStablePlant)
{
    const std::unique_ptr<LtiSystem> plant = plantWithPoles({0.0, 1.0, 2.0});
    const std::unique_ptr<LtiSystem> structure = pureGain(Range(1.0, 100.0));
    FamilyStabilityChecker family(plant.get(), structure.get(), ParameterGrids());

    const std::optional<Range> left = family.shaveUnstableGains(structure.get(), Range(1.0, 100.0));
    ASSERT_TRUE(left.has_value());
    EXPECT_EQ(left->min, 1.0);
    EXPECT_GE(left->max, 6.0) << "a stable gain was removed";
    EXPECT_LE(left->max, 6.0 * std::pow(10.0, 0.01));
}

TEST(GainShave, TheRouthTableRaisesTheFloorOfAnUnstablePlant)
{
    const std::unique_ptr<LtiSystem> plant = plantWithPoles({-1.0});
    const std::unique_ptr<LtiSystem> structure = pureGain(Range(0.01, 100.0));
    FamilyStabilityChecker family(plant.get(), structure.get(), ParameterGrids());

    const std::optional<Range> left = family.shaveUnstableGains(structure.get(), Range(0.01, 100.0));
    ASSERT_TRUE(left.has_value());
    EXPECT_LE(left->min, 1.0) << "a stable gain was removed";
    EXPECT_GE(left->min, std::pow(10.0, -0.01));
    EXPECT_EQ(left->max, 100.0);

    EXPECT_FALSE(family.shaveUnstableGains(structure.get(), Range(0.01, 0.5)).has_value())
        << "every gain below 1 is unstable";
}

TEST(GainShave, NoGainTheShavingRemovesIsStable)
{
    std::size_t removed = 0;
    for (const char * name : {"dcm-T33.qft", "toolbox-1.qft"}) {
        const std::string file = example(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        LtiSystem * structure = project.controllerStructure();
        FamilyStabilityChecker family(project.plant(), structure, project.sweepGrids());
        ASSERT_TRUE(family.usable());

        std::mt19937 generator(43);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        for (int trial = 0; trial < 200; ++trial) {
            const std::unique_ptr<LtiSystem> box = randomBox(*structure, std::pow(10.0, -2.0 + 2.0 * unit(generator)), generator);
            const Range gains = box->gain().range();
            family.isStable(pointInside(*box, generator));
            const std::optional<Range> left = family.shaveUnstableGains(box.get(), gains);
            const Range kept = left.has_value() ? *left : Range(gains.max, gains.max);
            const std::vector<double> sampled = left.has_value() ? removedGains(gains, kept, generator)
                                                                 : std::vector<double>{gains.min, gains.max};
            for (const double gain : sampled) {
                PointController point = pointInside(*box, generator);
                point.gain = gain;
                EXPECT_FALSE(family.isStable(point) && family.isStableAtNominal(point))
                    << name << " trial " << trial << ": the removed gain " << gain << " is stable";
                ++removed;
            }
        }
    }
    EXPECT_GT(removed, 0u);
}

TEST(GainContractor, NoGainTheContractionRemovesIsADesign)
{
    std::size_t removed = 0;
    std::size_t emptied = 0;
    for (const char * name : {"dcm-T33.qft", "toolbox-2.qft", "toolbox-1.qft"}) {
        const std::string file = example(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        LtiSystem * structure = project.controllerStructure();
        std::vector<double> & omega = *project.omega()->values();
        const SpecificationSet specifications = toSpecificationSet(*project.specifications());
        ExactPointCheck exact(*project.plant(), structure, omega, project.templates(), specifications);
        FamilyStabilityChecker family(project.plant(), structure, project.sweepGrids());
        std::vector<std::complex<double>> nominalPlantValues;
        for (const double w : omega) {
            nominalPlantValues.push_back(project.plant()->evaluate(w));
        }
        GainContractor contractor(exact, family, omega, nominalPlantValues);

        std::mt19937 generator(47);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        for (int trial = 0; trial < 150; ++trial) {
            const std::unique_ptr<LtiSystem> box = randomBox(*structure, std::pow(10.0, -2.0 + 2.0 * unit(generator)), generator);
            const Range gains = box->gain().range();
            const GainContractor::Contraction contraction = contractor.contract(box.get());
            const bool isEmptied = contraction.outcome == GainContractor::Outcome::EmptiedBySpecifications
                                   || contraction.outcome == GainContractor::Outcome::EmptiedByStability
                                   || contraction.outcome == GainContractor::Outcome::EmptiedByZeroExclusion;
            emptied += isEmptied ? 1 : 0;
            const std::vector<double> sampled = isEmptied
                    ? std::vector<double>{gains.min, std::sqrt(gains.min * gains.max), gains.max}
                    : removedGains(gains, contraction.gains, generator);
            for (const double gain : sampled) {
                PointController point = pointInside(*box, generator);
                point.gain = gain;
                const bool meets = exact.checkOf(point).worstExcessDb <= 0.0;
                EXPECT_FALSE(meets && family.isStable(point) && family.isStableAtNominal(point))
                    << name << " trial " << trial << ": the removed gain " << gain << " is a design";
                ++removed;
            }
        }
    }
    EXPECT_GT(removed, 0u);
    EXPECT_GT(emptied, 0u);
}

TEST(GainContractor, ContractsAsMuchAsTheExactPhaseOfTheBoxAllows)
{
    std::size_t acrossATurn = 0, contractedAcrossATurn = 0;
    for (const auto & [name, extra] : {std::pair<const char *, const char *>{"msf.qft", "pz"}, {"maglev-lower.qft", "p"}}) {
        const std::string file = example(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        LtiSystem * given = project.controllerStructure();
        std::vector<Parameter> zeros = given->numerator(), poles = given->denominator();
        for (const char step : std::string(extra)) {
            (step == 'p' ? poles : zeros).emplace_back(std::string(step == 'p' ? "p" : "z") + "x", Range(0.01, 1000.0), 0.01);
        }
        const std::unique_ptr<LtiSystem> structure = given->create(given->name(), zeros, poles, given->gain(), given->delay());
        std::vector<double> & omega = *project.omega()->values();
        const SpecificationSet specifications = toSpecificationSet(*project.specifications());
        ExactPointCheck exact(*project.plant(), structure.get(), omega, project.templates(), specifications);
        FamilyStabilityChecker family(project.plant(), structure.get(), project.sweepGrids());
        std::vector<std::complex<double>> nominalPlantValues;
        for (const double w : omega) {
            nominalPlantValues.push_back(project.plant()->evaluate(w));
        }
        GainContractor contractor(exact, family, omega, nominalPlantValues);

        std::mt19937 generator(61);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        for (int trial = 0; trial < 400; ++trial) {
            const std::unique_ptr<LtiSystem> box = randomBox(*structure, std::pow(10.0, -1.0 + 2.0 * unit(generator)), generator);
            Range expected = box->gain().range();
            bool emptied = false, crosses = false;
            for (std::size_t i = 0; i < omega.size() && !emptied; ++i) {
                const double w = omega[i];
                double phaseLow = std::arg(nominalPlantValues[i]), phaseHigh = phaseLow;
                double modulusLow = std::abs(nominalPlantValues[i]), modulusHigh = modulusLow;
                for (Parameter & z : box->numerator()) {
                    phaseLow += std::atan(w / z.range().max);
                    phaseHigh += std::atan(w / z.range().min);
                    modulusLow *= std::hypot(w, z.range().min);
                    modulusHigh *= std::hypot(w, z.range().max);
                }
                for (Parameter & q : box->denominator()) {
                    phaseLow -= std::atan(w / q.range().min);
                    phaseHigh -= std::atan(w / q.range().max);
                    modulusLow /= std::hypot(w, q.range().max);
                    modulusHigh /= std::hypot(w, q.range().min);
                }
                const Range phase(phaseLow * 180.0 / M_PI, phaseHigh * 180.0 / M_PI);
                const bool turn = phase.max - phase.min < 360.0 && std::floor(phase.max / 360.0) > std::floor(phase.min / 360.0);
                const double before = expected.min, beforeMax = expected.max;
                const ExactPointCheck::SectorVerdict verdict = exact.sectorVerdict(
                            i, phase, Range(20.0 * std::log10(expected.min * modulusLow), 20.0 * std::log10(expected.max * modulusHigh)));
                if (verdict.provablyInfeasible) {
                    emptied = true;
                    break;
                }
                if (std::isfinite(verdict.forbiddenBelowDb)) {
                    expected.min = std::max(expected.min, std::pow(10.0, verdict.forbiddenBelowDb / 20.0) / modulusHigh);
                }
                if (std::isfinite(verdict.forbiddenAboveDb)) {
                    expected.max = std::min(expected.max, std::pow(10.0, verdict.forbiddenAboveDb / 20.0) / modulusLow);
                }
                emptied = expected.min > expected.max;
                crosses = crosses || (turn && (expected.min > before || expected.max < beforeMax));
            }
            acrossATurn += crosses ? 1 : 0;

            const GainContractor::Contraction contraction = contractor.contract(box.get());
            const bool isEmptied = contraction.outcome != GainContractor::Outcome::Unchanged
                                   && contraction.outcome != GainContractor::Outcome::Contracted;
            if (emptied) {
                EXPECT_TRUE(isEmptied) << name << " trial " << trial << ": the specifications leave no gain";
                contractedAcrossATurn += crosses && isEmptied ? 1 : 0;
                continue;
            }
            if (!isEmptied) {
                EXPECT_GE(contraction.gains.min, expected.min * (1.0 - 1e-6)) << name << " trial " << trial;
                EXPECT_LE(contraction.gains.max, expected.max * (1.0 + 1e-6)) << name << " trial " << trial;
            }
            contractedAcrossATurn += crosses && (isEmptied || (contraction.gains.min >= expected.min * (1.0 - 1e-6)
                                                               && contraction.gains.max <= expected.max * (1.0 + 1e-6))) ? 1 : 0;
        }
    }
    std::printf("CONTRACTION across a turn: %zu boxes contracted by a frequency whose phase crosses a whole turn, %zu as much by the contractor\n",
                acrossATurn, contractedAcrossATurn);
    EXPECT_GT(acrossATurn, 0u);
}

TEST(ZeroExclusion, ProvesTheNearCancellationsTheRouthTableCannot)
{
    const std::string file = example("acc90.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    LtiSystem * base = project.controllerStructure();
    ASSERT_NE(base, nullptr);
    ASSERT_GE(base->denominator().size(), 1u);
    const std::unique_ptr<LtiSystem> structure = base->create(base->name(), base->numerator(),
                                                              {base->denominator().front()}, base->gain(), base->delay());
    ASSERT_EQ(structure->numerator().size(), 1u);
    FamilyStabilityChecker family(project.plant(), structure.get(), ParameterGrids());

    std::mt19937 generator(53);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::size_t routhProven = 0, onlyOnAxis = 0, sampled = 0;
    for (int trial = 0; trial < 300; ++trial) {
        const double centre = std::pow(10.0, -1.0 + 3.0 * unit(generator));
        const double offset = 1.0 + std::pow(10.0, -4.0 + 3.0 * unit(generator));
        const double width = std::pow(10.0, -4.0 + 2.0 * unit(generator));
        const double gain = std::pow(10.0, 3.0 + 4.0 * unit(generator));
        const std::unique_ptr<LtiSystem> box = structure->create(
                    "box", {Parameter(std::string("z"), Range(centre, centre * (1.0 + width)), centre)},
                    {Parameter(std::string("p"), Range(centre * offset, centre * offset * (1.0 + width)), centre * offset)},
                    Parameter(std::string("k"), Range(gain, gain * 2.0), gain));
        if (family.isBoxUnstableAtNominal(box.get())) {
            ++routhProven;
            continue;
        }
        if (!family.isBoxUnstableAtNominalOnAxis(box.get())) {
            continue;
        }
        ++onlyOnAxis;
        for (int point = 0; point < 20; ++point) {
            EXPECT_FALSE(family.isStableAtNominal(pointInside(*box, generator))) << "trial " << trial;
            ++sampled;
        }
    }
    std::printf("ZERO-EXCLUSION acc90: %zu boxes proven by the Routh table, %zu by the zero exclusion alone, %zu points sampled\n",
                routhProven, onlyOnAxis, sampled);
    EXPECT_GT(onlyOnAxis, 0u);
}

TEST(ZeroExclusion, NoBoxItProvesHoldsAStableController)
{
    std::size_t proven = 0, holdingStable = 0;
    for (const char * name : {"dcm-T33.qft", "toolbox-1.qft", "toolbox-2.qft", "maglev-upper.qft", "dcm-k.qft"}) {
        const std::string file = example(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        LtiSystem * structure = project.controllerStructure();
        FamilyStabilityChecker family(project.plant(), structure, ParameterGrids());

        std::mt19937 generator(59);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        for (int trial = 0; trial < 300; ++trial) {
            const std::unique_ptr<LtiSystem> box = randomBox(*structure, std::pow(10.0, -3.0 + 3.0 * unit(generator)), generator);
            const bool isProven = family.isBoxUnstableAtNominalOnAxis(box.get());
            proven += isProven ? 1 : 0;
            std::size_t stable = 0;
            for (int point = 0; point < 20; ++point) {
                stable += family.isStableAtNominal(pointInside(*box, generator)) ? 1 : 0;
            }
            holdingStable += stable > 0 ? 1 : 0;
            EXPECT_FALSE(isProven && stable > 0) << name << " trial " << trial << ": " << stable
                                                 << " stable controllers in a box proven unstable";
        }
    }
    std::printf("ZERO-EXCLUSION published: %zu boxes proven unstable, %zu holding a stable controller\n",
                proven, holdingStable);
    EXPECT_GT(proven, 0u);
    EXPECT_GT(holdingStable, 0u);
}
