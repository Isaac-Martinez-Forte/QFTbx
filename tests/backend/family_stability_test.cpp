/**
 * @file
 * @brief The verifier closes the loop with every plant of the sweep.
 *
 * The four plant forms give the same polynomials their evaluation does, and
 * the Routh table in interval arithmetic proves non-Hurwitz exactly the
 * interval polynomials every member of which fails, leaving undecided those
 * that hold a Hurwitz member or put zero inside a pivot; the
 * swept family walks the sweep in the order of the templates, first grid
 * fastest, names each member by its values, and says why it cannot be
 * walked; the
 * magnetic levitation benchmark tells apart the design that stabilises the
 * whole family from the one the specifications alone let through (42 of the
 * 121 plants unstable, the case that made the check necessary); the ACC'90
 * benchmark, where the minimum-gain answer is the floor of the gain box,
 * shows that a pole on the imaginary axis is not stability; a project
 * without a sweep record, a loop with a delay and a plant that is not
 * rational are reported as not checked and never approved as stable. A box
 * of controllers wholly beyond the Routh limit of the DC motor's worst plant
 * is proven unstable by the interval Routh table, one that straddles the
 * limit, or lies below it, is not, and no controller sampled inside a box the
 * table proves unstable is stable; the same holds with the nominal plant,
 * on the DC motor of Tharewal's example 3.1 and on the magnetic levitation
 * plant, whose poles sit on the imaginary axis. And the gate itself: on the
 * magnetic levitation problem the search returns a design the family does
 * not accept until it is given the sweep, and then only designs every plant
 * is stable under.
 */

#include <gtest/gtest.h>

#include <complex>
#include <filesystem>
#include <cmath>
#include <cstdio>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/family_stability_checker.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/loopshaping/common/swept_family.h"
#include "src/core/math/interval.h"
#include "src/core/math/interval_polynomial.h"
#include "src/core/math/range.h"
#include "src/core/math/polynomial.h"
#include "src/core/math/sequences.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/loopshaping/mc3/algorithm_mc3.h"

using namespace qftbx;

namespace {

std::complex<double> at(const std::vector<double> & polynomial, std::complex<double> s)
{
    std::complex<double> value(0.0, 0.0);
    for (const double coefficient : polynomial) {
        value = value * s + coefficient;
    }
    return value;
}

void expectPolynomialsMatchEvaluation(LtiSystem & system)
{
    std::vector<double> numerator, denominator;
    for (const Parameter & p : system.numerator()) numerator.push_back(p.nominal());
    for (const Parameter & p : system.denominator()) denominator.push_back(p.nominal());
    const double gain = system.gain().nominal();

    const std::optional<LtiSystem::Polynomials> polynomials = system.polynomialsAt(numerator, denominator, gain);
    ASSERT_TRUE(polynomials.has_value()) << system.expression();

    for (const double w : {0.1, 1.0, 3.7, 40.0}) {
        const std::complex<double> s(0.0, w);
        const std::complex<double> fromPolynomials = at(polynomials->numerator, s) / at(polynomials->denominator, s);
        const std::complex<double> direct = system.valueAt(w, numerator, denominator, gain, 0.0);
        EXPECT_NEAR(std::abs(fromPolynomials - direct), 0.0, 1e-9 * std::abs(direct)) << system.expression() << " at w = " << w;
    }
}

std::string example(const char * name)
{
    return (std::filesystem::path(QFTBX_EXAMPLES_DIR) / name).string();
}

ParameterGrids gridsOf(LtiSystem & plant, std::size_t points)
{
    ParameterGrids grids;
    const auto take = [&](const Parameter & parameter) {
        if (parameter.isUncertain() && grids.count(parameter.name()) == 0) {
            grids[parameter.name()] = math::linspace(parameter.range().min, parameter.range().max, points);
        }
    };
    for (const Parameter & p : plant.numerator()) take(p);
    for (const Parameter & p : plant.denominator()) take(p);
    take(plant.gain());
    return grids;
}

}

TEST(PolynomialArithmetic, ProductAndSumHighestDegreeFirst)
{
    EXPECT_EQ(math::polynomialProduct({1.0, 2.0}, {1.0, 3.0}), (std::vector<double>{1.0, 5.0, 6.0}));
    EXPECT_EQ(math::polynomialProduct({}, {1.0, 3.0}), (std::vector<double>{1.0, 3.0}));
    EXPECT_EQ(math::polynomialSum({1.0, 2.0, 3.0}, {4.0, 5.0}), (std::vector<double>{1.0, 6.0, 8.0}));
    EXPECT_EQ(math::polynomialSum({4.0, 5.0}, {1.0, 2.0, 3.0}), (std::vector<double>{1.0, 6.0, 8.0}));
}

TEST(IntervalRouth, ProvesNonHurwitzOnlyWhatEveryMemberFails)
{
    using math::provablyNotHurwitz;
    EXPECT_FALSE(provablyNotHurwitz({1.0, 3.0, 2.0})) << "(s + 1)(s + 2) is Hurwitz";
    EXPECT_TRUE(provablyNotHurwitz({1.0, -1.0, 1.0})) << "a negative coefficient";
    EXPECT_TRUE(provablyNotHurwitz({1.0, 0.0, 1.0})) << "a zero coefficient: roots on the axis";
    EXPECT_TRUE(provablyNotHurwitz({1.0, 1.0, 1.0, 2.0})) << "positive coefficients, b c < a d";
    EXPECT_FALSE(provablyNotHurwitz({1.0, 2.0, 3.0, 1.0})) << "positive coefficients, b c > a d";
    EXPECT_FALSE(provablyNotHurwitz({-1.0, -3.0, -2.0})) << "the same Hurwitz polynomial with its sign flipped";

    EXPECT_TRUE(provablyNotHurwitz({Interval(1.0), Interval(1.0), Interval(0.5, 1.5), Interval(2.0)}))
            << "every member has c < 2";
    EXPECT_FALSE(provablyNotHurwitz({Interval(1.0), Interval(1.0), Interval(0.5, 3.0), Interval(2.0)}))
            << "members with c > 2 are Hurwitz: nothing is proven";
    EXPECT_FALSE(provablyNotHurwitz({Interval(-1.0, 1.0), Interval(3.0), Interval(2.0)}))
            << "a leading coefficient of no definite sign";
}

TEST(PlantPolynomials, EveryFormAgreesWithItsEvaluation)
{
    std::vector<Parameter> zeros{Parameter(std::string("z"), 2.0)};
    std::vector<Parameter> poles{Parameter(std::string("p1"), 0.5), Parameter(std::string("p2"), 30.0)};
    ZeroPoleGain zpk("zpk", zeros, poles, Parameter(7.0), Parameter(0.0));
    expectPolynomialsMatchEvaluation(zpk);

    TimeConstantGain tcg("tcg", zeros, poles, Parameter(7.0), Parameter(0.0));
    expectPolynomialsMatchEvaluation(tcg);

    std::vector<Parameter> numerator{Parameter(std::string("b1"), 1.0), Parameter(std::string("b0"), 2.0)};
    std::vector<Parameter> denominator{Parameter(std::string("a2"), 1.0), Parameter(std::string("a1"), 30.5), Parameter(std::string("a0"), 15.0)};
    PolynomialForm polynomial("poly", numerator, denominator, Parameter(7.0), Parameter(0.0));
    expectPolynomialsMatchEvaluation(polynomial);

    std::vector<Parameter> none;
    std::vector<Parameter> a{Parameter(std::string("a"), 430.25)};
    FreeForm free("free", none, a, Parameter(877.5), Parameter(0.0), std::string("1"), std::string("s^2+a"));
    expectPolynomialsMatchEvaluation(free);
    const std::optional<LtiSystem::Polynomials> maglev = free.polynomialsAt({}, {430.25}, 877.5);
    ASSERT_TRUE(maglev.has_value());
    EXPECT_EQ(maglev->denominator.size(), 3u);
    EXPECT_NEAR(maglev->denominator[2], 430.25, 1e-9);
    EXPECT_NEAR(maglev->numerator.back(), 877.5, 1e-9);

    FreeForm delayed("delayed", none, none, Parameter(1.0), Parameter(0.0), std::string("1"), std::string("s+exp(-s)"));
    EXPECT_FALSE(delayed.polynomialsAt({}, {}, 1.0).has_value());
}

TEST(SweptFamily, MembersWalkTheSweepFirstGridFastest)
{
    const std::string file = example("maglev-lower.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    const ParameterGrids sweep = gridsOf(*project.plant(), 11);
    const std::vector<double> & a = sweep.at("a");
    const std::vector<double> & k = sweep.at("k");

    const SweptFamily family(*project.plant(), sweep);
    ASSERT_TRUE(family.usable());
    ASSERT_EQ(family.size(), 121u);

    for (std::size_t i = 0; i < family.size(); ++i) {
        const std::vector<std::pair<std::string, double>> values = family.valuesOf(i);
        ASSERT_EQ(values.size(), 2u) << "member " << i;
        EXPECT_EQ(values[0].first, "a");
        EXPECT_EQ(values[0].second, a[i % 11]) << "member " << i;
        EXPECT_EQ(values[1].first, "k");
        EXPECT_EQ(values[1].second, k[i / 11]) << "member " << i;

        const std::optional<LtiSystem::Polynomials> direct = project.plant()->polynomialsAt({}, {a[i % 11]}, k[i / 11]);
        ASSERT_TRUE(direct.has_value());
        EXPECT_EQ(family.member(i).numerator, direct->numerator) << "member " << i;
        EXPECT_EQ(family.member(i).denominator, direct->denominator) << "member " << i;
    }
}

TEST(SweptFamily, WhyItCannotBeWalked)
{
    const std::string file = example("maglev-lower.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    LtiSystem & plant = *project.plant();
    const ParameterGrids sweep = gridsOf(plant, 3);

    EXPECT_EQ(SweptFamily(plant, ParameterGrids()).state(), SweptFamily::State::NoSweepRecord);
    EXPECT_EQ(SweptFamily().state(), SweptFamily::State::NoSweepRecord);

    std::vector<Parameter> none;
    ZeroPoleGain delayed("delayed", none, none, Parameter(1.0), Parameter(0.2));
    EXPECT_EQ(SweptFamily(delayed, sweep).state(), SweptFamily::State::Delay);

    FreeForm transcendental("transcendental", none, none, Parameter(1.0), Parameter(0.0),
                            std::string("1"), std::string("s+exp(-s)"));
    EXPECT_EQ(SweptFamily(transcendental, sweep).state(), SweptFamily::State::NotRational);

    ParameterGrids unrelated;
    unrelated["x"] = {1.0, 2.0};
    const SweptFamily nominalOnly(plant, unrelated);
    EXPECT_TRUE(nominalOnly.usable()) << "a sweep that names none of the plant's parameters walks the nominal plant";
    EXPECT_EQ(nominalOnly.size(), 1u);
    EXPECT_TRUE(nominalOnly.valuesOf(0).empty());
}

TEST(FamilyStability, TheMaglevDesignsAreToldApart)
{
    const std::string file = example("maglev-lower.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    LtiSystem * structure = project.controllerStructure();
    ASSERT_NE(structure, nullptr);
    const std::vector<double> & omega = *project.omega()->values();
    const SpecificationSet specifications = toSpecificationSet(*project.specifications());
    const ParameterGrids sweep = gridsOf(*project.plant(), 11);

    const auto controller = [&](double k, std::vector<double> zeros, std::vector<double> poles) {
        std::vector<Parameter> z, p;
        for (double v : zeros) z.emplace_back(v);
        for (double v : poles) p.emplace_back(v);
        return structure->create("designed", z, p, Parameter(k), Parameter(0.0));
    };

    std::unique_ptr<LtiSystem> stabilising = controller(0.01, {7.822421875, 366.31175373399486}, {0.01});
    const SpecificationCheck good = checkAgainstSpecifications(*stabilising, *project.plant(), omega,
                                                               project.templates(), specifications, &sweep);
    EXPECT_TRUE(good.family.checked);
    EXPECT_EQ(good.family.members, 121u);
    EXPECT_EQ(good.family.unstableMembers, 0u);
    EXPECT_LT(good.family.worstRealPart, 0.0);
    EXPECT_TRUE(good.satisfied());

    std::unique_ptr<LtiSystem> approvedByTheBounds = controller(0.01, {9.30572040929699, 697.8305848598663}, {0.014330125702369627});
    const SpecificationCheck bad = checkAgainstSpecifications(*approvedByTheBounds, *project.plant(), omega,
                                                              project.templates(), specifications, &sweep);
    EXPECT_LE(bad.worstExcessDb, 0.0) << "the specifications alone let this design through";
    EXPECT_TRUE(bad.family.checked);
    EXPECT_EQ(bad.family.members, 121u);
    EXPECT_EQ(bad.family.unstableMembers, 42u);
    EXPECT_NEAR(bad.family.worstRealPart, 0.2403, 1e-3);
    ASSERT_EQ(bad.family.worstMember.size(), 2u);
    EXPECT_FALSE(bad.satisfied());

    const SpecificationCheck unrecorded = checkAgainstSpecifications(*approvedByTheBounds, *project.plant(), omega,
                                                                     project.templates(), specifications, nullptr);
    EXPECT_FALSE(unrecorded.family.checked);
    EXPECT_EQ(unrecorded.family.notChecked, FamilyStability::NotChecked::NoSweepRecord);
    EXPECT_EQ(unrecorded.family.unstableMembers, 0u);
}

TEST(FamilyStability, ADelayLeavesTheFamilyUnchecked)
{
    const std::string file = example("maglev-lower.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    const ParameterGrids sweep = gridsOf(*project.plant(), 3);
    std::unique_ptr<LtiSystem> delayed = project.controllerStructure()->create(
                "delayed", {Parameter(7.8), Parameter(366.3)}, {Parameter(0.01)}, Parameter(0.01), Parameter(0.2));
    const SpecificationCheck check = checkAgainstSpecifications(*delayed, *project.plant(), *project.omega()->values(),
                                                                project.templates(), toSpecificationSet(*project.specifications()), &sweep);
    EXPECT_FALSE(check.family.checked);
    EXPECT_EQ(check.family.notChecked, FamilyStability::NotChecked::Delay);
}

TEST(FamilyStability, APoleOnTheAxisIsNotStability)
{
    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR "/acc90.qft"));
    const ParameterGrids sweep = gridsOf(*project.plant(), 25);
    const SpecificationSet specifications = toSpecificationSet(*project.specifications());

    std::unique_ptr<LtiSystem> atTheFloor = project.controllerStructure()->create(
                "floor", {Parameter(0.01)}, {Parameter(921.87578125)}, Parameter(0.001), Parameter(0.0));
    const SpecificationCheck floor = checkAgainstSpecifications(*atTheFloor, *project.plant(),
                                                                *project.omega()->values(), project.templates(),
                                                                specifications, &sweep);
    EXPECT_LT(floor.worstExcessDb, -90.0) << "the smaller the loop the wider the margin";
    EXPECT_TRUE(floor.family.checked);
    EXPECT_EQ(floor.family.unstableMembers, floor.family.members)
        << "the double integrator leaves a pair of poles on the axis when the gain goes to zero";
    EXPECT_LT(floor.family.worstRealPart, 0.0) << "negative, and a millionth of the largest pole";
    EXPECT_FALSE(floor.satisfied());

    std::unique_ptr<LtiSystem> lead = project.controllerStructure()->create(
                "lead", {Parameter(0.1)}, {Parameter(1.0)}, Parameter(0.03), Parameter(0.0));
    const SpecificationCheck stabilising = checkAgainstSpecifications(*lead, *project.plant(),
                                                                      *project.omega()->values(), project.templates(),
                                                                      specifications, &sweep);
    EXPECT_EQ(stabilising.family.unstableMembers, 0u) << "a lead below the resonance does stabilise the family";
    EXPECT_LT(stabilising.family.worstRealPart, -1e-3);
}

TEST(FamilyStabilityGate, ABoxBeyondTheRouthLimitIsProvenUnstable)
{
    const std::string file = example("dcm-T33.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    LtiSystem * structure = project.controllerStructure();
    ASSERT_NE(structure, nullptr);
    FamilyStabilityChecker family(project.plant(), structure, project.sweepGrids());
    ASSERT_TRUE(family.usable());

    const auto box = [&](Range gain, Range zero, Range pole) {
        return structure->create("box", {Parameter(std::string("z1"), zero, zero.min)},
                                 {Parameter(std::string("p1"), pole, pole.min)},
                                 Parameter(std::string("k"), gain, gain.min), Parameter(0.0));
    };

    EXPECT_FALSE(family.isStable({45.0, {1000.0}, {466.5}})) << "a gain above the Routh limit of a = 1, k = 10";
    EXPECT_TRUE(family.isStable({40.0, {1000.0}, {466.5}}));

    std::unique_ptr<LtiSystem> beyond = box(Range(45.0, 50.0), Range(999.0, 1000.0), Range(466.0, 467.0));
    EXPECT_TRUE(family.isBoxUnstable(beyond.get())) << "every controller of the box destabilises the worst plant";

    std::unique_ptr<LtiSystem> crossing = box(Range(40.5, 41.5), Range(999.0, 1000.0), Range(466.0, 467.0));
    EXPECT_FALSE(family.isBoxUnstable(crossing.get())) << "the box straddles the limit: nothing is proven";

    std::unique_ptr<LtiSystem> below = box(Range(1.0, 2.0), Range(999.0, 1000.0), Range(466.0, 467.0));
    EXPECT_FALSE(family.isBoxUnstable(below.get())) << "every controller of the box is stable";

    std::unique_ptr<LtiSystem> wide = box(Range(0.01, 1000.0), Range(0.01, 1000.0), Range(0.01, 1000.0));
    EXPECT_FALSE(family.isBoxUnstable(wide.get())) << "the initial box holds stable controllers";

    EXPECT_EQ(family.statistics().boxVerdicts, 4u);
    EXPECT_EQ(family.statistics().boxPrunes, 1u);
}

TEST(FamilyStabilityGate, AProvenBoxHoldsNoStableController)
{
    for (const char * name : {"dcm-T33.qft", "toolbox-1.qft"}) {
        const std::string file = example(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        LtiSystem * structure = project.controllerStructure();
        ASSERT_NE(structure, nullptr);
        FamilyStabilityChecker family(project.plant(), structure, project.sweepGrids());
        ASSERT_TRUE(family.usable());

        std::mt19937 generator(17);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        const auto logDraw = [&](const Parameter & parameter) {
            const Range r = parameter.range();
            return std::exp(std::log(r.min) + unit(generator) * (std::log(r.max) - std::log(r.min)));
        };
        const auto around = [&](double centre, double relativeWidth, const Range & within) {
            const double lo = std::max(within.min, centre / (1.0 + relativeWidth));
            const double hi = std::min(within.max, centre * (1.0 + relativeWidth));
            return Range(lo, hi);
        };

        std::size_t proven = 0, sampled = 0;
        for (int trial = 0; trial < 400; ++trial) {
            const double width = std::pow(10.0, -3.0 + 3.0 * unit(generator));
            std::vector<Parameter> zeros, poles;
            for (const Parameter & z : structure->numerator()) {
                zeros.emplace_back(z.name(), around(logDraw(z), width, z.range()), z.range().min);
            }
            for (const Parameter & q : structure->denominator()) {
                poles.emplace_back(q.name(), around(logDraw(q), width, q.range()), q.range().min);
            }
            const Range gain = around(logDraw(structure->gain()), width, structure->gain().range());
            std::unique_ptr<LtiSystem> box = structure->create("box", zeros, poles,
                                                               Parameter(std::string("k"), gain, gain.min), Parameter(0.0));
            family.isStable(cornerOf(box.get(), true));
            if (!family.isBoxUnstable(box.get())) {
                continue;
            }
            ++proven;
            for (int point = 0; point < 50; ++point) {
                PointController inside;
                for (const Parameter & z : box->numerator()) inside.zeros.push_back(z.range().min + unit(generator) * (z.range().max - z.range().min));
                for (const Parameter & q : box->denominator()) inside.poles.push_back(q.range().min + unit(generator) * (q.range().max - q.range().min));
                inside.gain = gain.min + unit(generator) * (gain.max - gain.min);
                EXPECT_FALSE(family.isStable(inside)) << name << " trial " << trial << ": a controller inside a proven box is stable";
                EXPECT_FALSE(family.isStableByRoots(inside)) << name << " trial " << trial;
                ++sampled;
            }
        }
        std::printf("FAMILY-BOX %-13s %zu of 400 random boxes proven unstable, %zu controllers sampled inside\n", name, proven, sampled);
        EXPECT_GT(proven, 0u) << name;
    }

    ProjectController project;
    project.load(example("toolbox-1.qft"));
    LtiSystem * structure = project.controllerStructure();
    FamilyStabilityChecker family(project.plant(), structure, project.sweepGrids());
    family.isStable({70.0, {1000.0}, {154.4}});
    const PointController returned{50.76606550592597, {974.9780649705508}, {130.90456263300797}};
    std::unique_ptr<LtiSystem> design = structure->create("design", {Parameter(std::string("z1"), Range(974.9, 975.1), 974.9)},
                                                          {Parameter(std::string("p1"), Range(130.9, 130.91), 130.9)},
                                                          Parameter(std::string("k"), Range(50.766, 50.77), 50.766), Parameter(0.0));
    EXPECT_TRUE(family.isStable(returned)) << "the design the battery returned stabilises the family by Routh";
    EXPECT_TRUE(family.isStableByRoots(returned)) << "and by the roots";
    EXPECT_FALSE(family.isBoxUnstable(design.get())) << "a box holding that design cannot be proven unstable";
}

TEST(FamilyStabilityGate, ABoxProvenUnstableAtTheNominalPlantHoldsNoController)
{
    for (const char * name : {"dcm-k.qft", "maglev-lower.qft"}) {
        const std::string file = example(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        ProjectController project;
        project.load(file);
        LtiSystem * structure = project.controllerStructure();
        ASSERT_NE(structure, nullptr);
        LtiSystem * plant = project.plant();
        FamilyStabilityChecker family(plant, structure, project.sweepGrids());
        ASSERT_TRUE(family.usable());

        const std::optional<LtiSystem::Polynomials> nominal = plant->polynomialsAt(
                    nominalValues(plant->numerator()), nominalValues(plant->denominator()), plant->gain().nominal());
        ASSERT_TRUE(nominal.has_value());
        std::unique_ptr<LtiSystem> controller = structure->clone();
        const auto stableAtNominal = [&](const PointController & point) {
            const std::optional<LtiSystem::Polynomials> loop = controller->polynomialsAt(point.zeros, point.poles, point.gain);
            return math::isHurwitz(math::polynomialSum(math::polynomialProduct(nominal->numerator, loop->numerator),
                                                       math::polynomialProduct(nominal->denominator, loop->denominator)));
        };

        std::mt19937 generator(23);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        const auto logDraw = [&](const Parameter & parameter) {
            const Range r = parameter.range();
            return std::exp(std::log(r.min) + unit(generator) * (std::log(r.max) - std::log(r.min)));
        };
        const auto around = [&](double centre, double relativeWidth, const Range & within) {
            return Range(std::max(within.min, centre / (1.0 + relativeWidth)), std::min(within.max, centre * (1.0 + relativeWidth)));
        };

        std::size_t proven = 0, sampled = 0, holdingAStableOne = 0, provenOfThose = 0;
        for (int trial = 0; trial < 1500; ++trial) {
            const double width = std::pow(10.0, -4.0 + 4.0 * unit(generator));
            std::vector<Parameter> zeros, poles;
            for (const Parameter & z : structure->numerator()) {
                zeros.emplace_back(z.name(), around(logDraw(z), width, z.range()), z.range().min);
            }
            for (const Parameter & q : structure->denominator()) {
                poles.emplace_back(q.name(), around(logDraw(q), width, q.range()), q.range().min);
            }
            const Range gain = around(logDraw(structure->gain()), width, structure->gain().range());
            std::unique_ptr<LtiSystem> box = structure->create("box", zeros, poles,
                                                               Parameter(std::string("k"), gain, gain.min), Parameter(0.0));
            const bool isProven = family.isBoxUnstableAtNominal(box.get());
            proven += isProven ? 1 : 0;

            bool anyStable = false;
            for (int point = 0; point < 20; ++point) {
                PointController inside;
                for (const Parameter & z : box->numerator()) inside.zeros.push_back(z.range().min + unit(generator) * (z.range().max - z.range().min));
                for (const Parameter & q : box->denominator()) inside.poles.push_back(q.range().min + unit(generator) * (q.range().max - q.range().min));
                inside.gain = gain.min + unit(generator) * (gain.max - gain.min);
                const bool stable = stableAtNominal(inside);
                anyStable = anyStable || stable;
                if (isProven) {
                    EXPECT_FALSE(stable) << name << " trial " << trial << ": a controller inside a box proven unstable at the nominal plant is stable";
                    ++sampled;
                }
            }
            if (anyStable) {
                ++holdingAStableOne;
                provenOfThose += isProven ? 1 : 0;
            }
        }
        std::printf("NOMINAL-BOX %-16s %zu of 1500 random boxes proven unstable at the nominal plant, %zu controllers sampled inside; "
                    "%zu boxes hold a stable one, %zu of them proven\n", name, proven, sampled, holdingAStableOne, provenOfThose);
        EXPECT_GT(proven, 0u) << name;
        EXPECT_GT(holdingAStableOne, 0u) << name;
        EXPECT_EQ(provenOfThose, 0u) << name;
        EXPECT_EQ(family.statistics().nominalBoxVerdicts, 1500u) << name;
        EXPECT_EQ(family.statistics().nominalBoxPrunes, proven) << name;
    }
}

TEST(FamilyStabilityGate, TheSearchNoLongerReturnsTheUnstableMaglevDesign)
{
    const std::string file = example("maglev-lower.qft");
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    ASSERT_NE(project.boundaries(), nullptr);

    const ParameterGrids sweep = gridsOf(*project.plant(), 11);
    const SpecificationSet specifications = toSpecificationSet(*project.specifications());

    const auto designWith = [&](bool gated) {
        AlgorithmMc3 search;
        search.setProblem(project.plant(), project.controllerStructure(),
                          project.omega()->values(), project.boundaries(), 0.5);
        if (gated) {
            search.setPlantFamily(sweep);
        }
        std::unique_ptr<LtiSystem> designed;
        if (search.solve()) {
            designed = search.controllerStructure();
        }
        return designed;
    };

    const auto familyOf = [&](LtiSystem * designed) {
        return checkAgainstSpecifications(*designed, *project.plant(), *project.omega()->values(),
                                          project.templates(), specifications, &sweep).family;
    };

    std::unique_ptr<LtiSystem> ungated = designWith(false);
    ASSERT_NE(ungated, nullptr);
    const FamilyStability before = familyOf(ungated.get());
    EXPECT_GT(before.unstableMembers, 0u)
        << "without the gate the search returns a design the bounds accept and the family does not";

    std::unique_ptr<LtiSystem> gated = designWith(true);
    if (gated != nullptr) {
        EXPECT_EQ(familyOf(gated.get()).unstableMembers, 0u)
            << "what the gate lets through leaves every plant of the sweep stable";
        EXPECT_GE(gated->gain().range().min, ungated->gain().range().min)
            << "a design that stabilises the family costs at least as much gain";
    }
}
