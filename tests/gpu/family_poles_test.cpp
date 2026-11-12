/**
 * @file
 * @brief The check of the family's poles on the card gives the processor's.
 *
 * Without a card, the code every warp runs is run on the processor: on the
 * 27 examples, with their grids and twice as dense, with and without the
 * border sweep, and on families in each of the four forms that are stable,
 * that have poles on the imaginary axis, that change their number of right
 * half-plane poles, and that have a member whose poles cannot be placed, it
 * must find the processor's first count, the same first member that stops
 * the check, and the same bands within 1e-8, relatively: recovering a free
 * form of degree nine with a triple root, the processor's transform is
 * already 2e-9 off the exact frequency, and the warps' 3e-10. A family
 * whose denominator does not fit a warp is refused. With a card, the card's
 * check must be the processor's run of the same code, the bands within
 * 1e-12.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/frequencies/omega.h"
#include "src/core/gpu/templates/family_poles.h"
#include "src/core/gpu/templates/gpu_template_engine.h"
#include "src/core/math/range.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/system/free_form.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/templates/sweep_plan.h"
#include "tests/gpu/card.h"

using namespace qftbx;

namespace {

constexpr double kProcessorTolerance = 1e-8;
constexpr double kCardTolerance = 1e-12;

struct Family
{
    std::string name;
    std::unique_ptr<LtiSystem> plant;
    ParameterGrids grids;
    bool border = false;
};

std::vector<std::filesystem::path> examples()
{
    std::vector<std::filesystem::path> files;
    for (const auto & entry : std::filesystem::directory_iterator(QFTBX_EXAMPLES_DIR)) {
        if (entry.path().extension() == ".qft") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

ParameterGrids denser(const ParameterGrids & grids, std::size_t factor)
{
    ParameterGrids out;
    for (const auto & [name, grid] : grids) {
        const auto [low, high] = std::minmax_element(grid.begin(), grid.end());
        const std::size_t size = grid.size() <= 1 ? grid.size() : (grid.size() - 1) * factor + 1;
        out[name] = size <= 1 ? grid : linspace(*low, *high, static_cast<int>(size));
    }
    return out;
}

Parameter uncertain(const std::string & name, double nominal = 1.0)
{
    return Parameter(name, Range(0.5, 2.5), nominal);
}

std::unique_ptr<LtiSystem> freeForm(const std::string & denominator, std::vector<Parameter> parameters)
{
    return std::make_unique<FreeForm>("P", std::vector<Parameter>{}, std::move(parameters), Parameter(1.0),
                                      Parameter(0.0), "1", denominator);
}

std::vector<Family> families()
{
    std::vector<Family> out;
    const auto add = [&out](std::string name, std::unique_ptr<LtiSystem> plant, ParameterGrids grids, bool border = false) {
        out.push_back({std::move(name), std::move(plant), std::move(grids), border});
    };

    add("polynomial, stable", std::make_unique<PolynomialForm>("P", std::vector<Parameter>{Parameter(1.0)},
            std::vector<Parameter>{Parameter(1.0), uncertain("a"), uncertain("b")}, Parameter(1.0), Parameter(0.0)),
        {{"a", {0.5, 1.0, 2.0}}, {"b", {1.0, 4.0}}});
    add("polynomial, on the axis", std::make_unique<PolynomialForm>("P", std::vector<Parameter>{Parameter(1.0)},
            std::vector<Parameter>{Parameter(1.0), Parameter(0.0), uncertain("b")}, Parameter(1.0), Parameter(0.0)),
        {{"b", {1.0, 2.25, 4.0}}});
    add("polynomial, crossing", std::make_unique<PolynomialForm>("P", std::vector<Parameter>{Parameter(1.0)},
            std::vector<Parameter>{Parameter(1.0), uncertain("a"), Parameter(1.0), Parameter(0.5)}, Parameter(1.0),
            Parameter(0.0)),
        {{"a", {2.0, 1.0, -1.0}}});
    add("zeros and poles, crossing after a zero", std::make_unique<ZeroPoleGain>("P",
            std::vector<Parameter>{uncertain("z")}, std::vector<Parameter>{uncertain("p"), Parameter(3.0)},
            Parameter(1.0), Parameter(0.0)),
        {{"z", {1.0, 2.0, 3.0}}, {"p", {1.0, -0.5}}});
    add("zeros and poles, stable", std::make_unique<ZeroPoleGain>("P", std::vector<Parameter>{},
            std::vector<Parameter>{uncertain("p"), uncertain("q")}, Parameter(1.0), Parameter(0.0)),
        {{"p", {1.0, 2.0}}, {"q", {0.5, 3.0}}});
    add("time constants, unplaceable", std::make_unique<TimeConstantGain>("P", std::vector<Parameter>{},
            std::vector<Parameter>{uncertain("c")}, Parameter(1.0), Parameter(0.0)),
        {{"c", {1.0, 0.0, -1.0}}});
    add("time constants, crossing", std::make_unique<TimeConstantGain>("P", std::vector<Parameter>{},
            std::vector<Parameter>{uncertain("c")}, Parameter(1.0), Parameter(0.0)),
        {{"c", {1.0, -1.0, 0.0}}});
    add("free form, crossing", freeForm("s^2-a", {uncertain("a")}), {{"a", {-1.0, -4.0, 0.5, 2.0}}});
    add("free form, unplaceable", freeForm("s+a+(1-a^2)*sqrt(s)", {uncertain("a")}), {{"a", {1.0, 0.0, -1.0}}});
    add("free form, on the axis", freeForm("s^2+a", {uncertain("a")}), {{"a", {1.0, 2.25, 4.0}}});
    add("free form, two bands", freeForm("(s^2+a)*(s^2+0.1*s+4*a)*(s+3-a)", {uncertain("a")}),
        {{"a", {1.0, 1.5, 2.0, 2.5}}});
    add("free form, degree nine", freeForm("(s^2+a)*(s^2+2*a)*(s+1)^3*(s+2)*(s+b)", {uncertain("a"), uncertain("b")}),
        {{"a", {1.0, 2.0}}, {"b", {0.5, 1.5, 2.5}}});
    add("free form, border", freeForm("(s+a)*(s^2+b*s+1)", {uncertain("a"), uncertain("b")}),
        {{"a", {0.5, 1.0, 1.5, 2.0}}, {"b", {0.5, 1.0, 2.0}}}, true);
    return out;
}

void expectSame(const FamilyPoles & expected, const gpu::FamilyOnCard & found, double tolerance,
                const std::string & where)
{
    EXPECT_EQ(found.firstCount, expected.firstCount) << where;
    EXPECT_EQ(found.unplaceableAt.has_value(), expected.unplaceableAt.has_value()) << where;
    if (found.unplaceableAt.has_value() && expected.unplaceableAt.has_value()) {
        EXPECT_EQ(*found.unplaceableAt, *expected.unplaceableAt) << where;
    }
    ASSERT_EQ(found.otherCountAt.has_value(), expected.otherCountAt.has_value()) << where;
    if (expected.otherCountAt.has_value()) {
        EXPECT_EQ(*found.otherCountAt, *expected.otherCountAt) << where;
        EXPECT_EQ(found.otherCount, expected.otherCount) << where;
        return;
    }
    ASSERT_EQ(found.bands.size(), expected.bands.size()) << where;
    for (std::size_t p = 0; p < expected.bands.size(); ++p) {
        for (int end = 0; end < 2; ++end) {
            const double gap = std::abs(found.bands[p][end] - expected.bands[p][end]) / std::abs(expected.bands[p][end]);
            EXPECT_LE(gap, tolerance) << where << ", band " << p << ", end " << end;
        }
    }
}

FamilyPoles asFamilyPoles(const gpu::FamilyOnCard & family)
{
    FamilyPoles poles;
    poles.firstCount = family.firstCount;
    if (family.unplaceableAt.has_value()) {
        poles.unplaceableAt = static_cast<std::size_t>(*family.unplaceableAt);
    }
    if (family.otherCountAt.has_value()) {
        poles.otherCountAt = static_cast<std::size_t>(*family.otherCountAt);
    }
    poles.otherCount = family.otherCount;
    poles.bands = family.bands;
    return poles;
}

template <typename Check>
void forEveryExample(Check && check)
{
    for (const std::filesystem::path & file : examples()) {
        ProjectController project;
        project.load(file.string());
        LtiSystem & plant = *project.plant();
        const std::vector<double> frequencies = *project.omega()->values();
        for (const std::size_t factor : {1u, 2u}) {
            for (const bool border : {false, true}) {
                const SweepPlan plan = planCombinations(plant, frequencies, denser(project.sweepGrids(), factor), border);
                check(plant, plan, file.stem().string() + " x" + std::to_string(factor) + (border ? " border" : ""));
            }
        }
    }
}

}

TEST(FamilyOnTheCard, TheThreadsCodeChecksTheExamplesAsTheProcessor)
{
    std::size_t checked = 0;
    forEveryExample([&](LtiSystem & plant, const SweepPlan & plan, const std::string & where) {
        const gpu::FamilyOnCard found = gpu::familyOnProcessor(gpu::modelOf(plant, plan), gpu::gridOf(plan));
        expectSame(checkFamily(plant, plan), found, kProcessorTolerance, where);
        EXPECT_GT(found.denominators, 0u) << where;
        ++checked;
    });
    EXPECT_EQ(checked, 4 * examples().size());
}

TEST(FamilyOnTheCard, TheFourFormsAreCheckedAsTheProcessorChecksThem)
{
    for (Family & family : families()) {
        const SweepPlan plan = planCombinations(*family.plant, {0.3}, family.grids, family.border);
        ASSERT_EQ(plan.borderSweepApplied(), family.border) << family.name;
        const gpu::FamilyOnCard found = gpu::familyOnProcessor(gpu::modelOf(*family.plant, plan), gpu::gridOf(plan));
        expectSame(checkFamily(*family.plant, plan), found, kProcessorTolerance, family.name);
    }
}

TEST(FamilyOnTheCard, TheFamiliesStopWhereTheProcessorStops)
{
    std::vector<Family> all = families();
    const auto check = [&](const std::string & name) {
        for (Family & family : all) {
            if (family.name == name) {
                const SweepPlan plan = planCombinations(*family.plant, {0.3}, family.grids, family.border);
                return gpu::familyOnProcessor(gpu::modelOf(*family.plant, plan), gpu::gridOf(plan));
            }
        }
        return gpu::FamilyOnCard{};
    };

    const gpu::FamilyOnCard afterAZero = check("zeros and poles, crossing after a zero");
    EXPECT_EQ(afterAZero.otherCountAt, std::uint64_t{3});
    EXPECT_EQ(afterAZero.otherCount, 1);
    EXPECT_EQ(afterAZero.denominators, 2u);

    const gpu::FamilyOnCard unplaceable = check("time constants, unplaceable");
    EXPECT_EQ(unplaceable.firstCount, 0);
    EXPECT_EQ(unplaceable.unplaceableAt, std::uint64_t{1});
    EXPECT_FALSE(unplaceable.otherCountAt.has_value());

    const gpu::FamilyOnCard band = check("free form, on the axis");
    ASSERT_EQ(band.bands.size(), 1u);
    EXPECT_NEAR(band.bands[0][0], 1.0, 1e-12);
    EXPECT_NEAR(band.bands[0][1], 2.0, 1e-12);
}

TEST(FamilyOnTheCard, ADenominatorThatDoesNotFitAWarpIsRefused)
{
    std::vector<Parameter> denominator(gpu::kMaxDenominator + 1, Parameter(1.0));
    PolynomialForm plant("P", {Parameter(1.0)}, denominator, Parameter(1.0), Parameter(0.0));
    const SweepPlan plan = planCombinations(plant, {0.3}, {}, false);
    EXPECT_THROW(gpu::modelOf(plant, plan), ComputationError);
}

TEST(GpuTemplates, TheCardChecksTheFamilyAsTheThreadsCodeOnTheProcessor)
{
    QFTBX_REQUIRE_CARD();

    forEveryExample([&](LtiSystem & plant, const SweepPlan & plan, const std::string & where) {
        const gpu::PlantModel model = gpu::modelOf(plant, plan);
        const gpu::SweepGrid grid = gpu::gridOf(plan);
        const gpu::FamilyOnCard found = gpu::familyOnCard(model, grid);
        expectSame(asFamilyPoles(gpu::familyOnProcessor(model, grid)), found, kCardTolerance, where);
        EXPECT_EQ(found.denominators, gpu::familyOnProcessor(model, grid).denominators) << where;
    });
    for (Family & family : families()) {
        const SweepPlan plan = planCombinations(*family.plant, {0.3}, family.grids, family.border);
        const gpu::PlantModel model = gpu::modelOf(*family.plant, plan);
        const gpu::SweepGrid grid = gpu::gridOf(plan);
        expectSame(asFamilyPoles(gpu::familyOnProcessor(model, grid)), gpu::familyOnCard(model, grid), kCardTolerance,
                   family.name);
    }
}
