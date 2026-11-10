/**
 * @file
 * @brief The sweep of the plant family on the card gives the processor's
 * plants.
 *
 * Without a card, the code every thread runs is run on the processor: on
 * the 27 examples, every plant of every frequency must come out within
 * 1e-13 of the processor's, relatively, and so must a family written in
 * each of the four forms, with and without a delay. A combination's digits
 * are the processor's, the border sweep's included, and a plant with more
 * uncertain parameters than a thread holds is refused. With a card, the
 * engine of the GPU must publish the same clouds as the processor's on
 * every example, within the same tolerance, keeping them on the card.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/gpu/templates/gpu_template_engine.h"
#include "src/core/gpu/templates/plant_sweep.h"
#include "src/core/system/free_form.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/templates/sweep_plan.h"
#include "tests/gpu/card.h"

using namespace qftbx;

namespace {

constexpr double kTolerance = 1e-13;

double relativeGap(std::complex<double> card, std::complex<double> processor)
{
    const double scale = std::abs(processor);
    return std::abs(card - processor) / (scale > 0.0 ? scale : 1.0);
}

double largestGapOnTheProcessor(LtiSystem & plant, const std::vector<double> & frequencies, const ParameterGrids & grids,
                                bool borderSweep = false)
{
    const SweepPlan plan = planSweep(plant, frequencies, grids, borderSweep);
    const CloudSet processor = sweepOnProcessor(plant, frequencies, plan, nullptr);
    const gpu::PlantModel model = gpu::modelOf(plant, plan);
    const gpu::SweepGrid grid = gpu::gridOf(plan);
    const std::size_t stride = std::max<std::size_t>(1, plan.combinationCount / 2000);

    double largest = 0.0;
    for (std::size_t u = 0; u < frequencies.size(); ++u) {
        for (std::size_t i = 0; i < plan.combinationCount; i += stride) {
            const std::complex<double> card =
                    gpu::plantOnProcessor(model, gpu::digitsOnProcessor(grid, i), frequencies[u]);
            largest = std::max(largest, relativeGap(card, processor[u][i]));
        }
    }
    return largest;
}

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

ParameterGrids gridsFor(const std::vector<std::string> & names)
{
    ParameterGrids grids;
    for (std::size_t k = 0; k < names.size(); ++k) {
        std::vector<double> values;
        for (int j = 0; j < 5; ++j) {
            values.push_back(0.5 + 0.4 * static_cast<double>(k) + 0.3 * static_cast<double>(j));
        }
        grids[names[k]] = values;
    }
    return grids;
}

Parameter uncertain(const std::string & name)
{
    return Parameter(name, Range(0.5, 2.5), 1.0);
}

}

TEST(PlantOnTheCard, TheThreadsCodeGivesTheProcessorsPlantsOnEveryExample)
{
    std::size_t checked = 0;
    for (const std::filesystem::path & file : examples()) {
        ProjectController project;
        project.load(file.string());
        ASSERT_NE(project.plant(), nullptr) << file;
        const double gap = largestGapOnTheProcessor(*project.plant(), *project.omega()->values(), project.sweepGrids());
        EXPECT_LE(gap, kTolerance) << file.filename();
        ++checked;
    }
    EXPECT_EQ(checked, 27u);
}

TEST(PlantOnTheCard, TheFourFormsGiveTheProcessorsPlantsWithAndWithoutDelay)
{
    const std::vector<double> frequencies{0.05, 0.7, 3.0, 45.0, 900.0};
    for (const bool delayed : {false, true}) {
        const Parameter delay = delayed ? uncertain("tau") : Parameter(0.0);
        std::vector<std::string> names{"a", "b", "k"};
        if (delayed) {
            names.push_back("tau");
        }
        const ParameterGrids grids = gridsFor(names);

        PolynomialForm polynomial("P", {uncertain("a"), Parameter(1.0)}, {Parameter(1.0), uncertain("b"), Parameter(2.0)},
                                  uncertain("k"), delay);
        ZeroPoleGain zeroPole("P", {uncertain("a")}, {uncertain("b"), Parameter(3.0)}, uncertain("k"), delay);
        TimeConstantGain timeConstant("P", {uncertain("a")}, {uncertain("b"), Parameter(0.2)}, uncertain("k"), delay);
        FreeForm freeForm("P", {uncertain("a")}, {uncertain("b")}, uncertain("k"), delay,
                          "s+a", "(s+b)*(s^2+0.4*s+b)");

        EXPECT_LE(largestGapOnTheProcessor(polynomial, frequencies, grids), kTolerance) << delayed;
        EXPECT_LE(largestGapOnTheProcessor(zeroPole, frequencies, grids), kTolerance) << delayed;
        EXPECT_LE(largestGapOnTheProcessor(timeConstant, frequencies, grids), kTolerance) << delayed;
        EXPECT_LE(largestGapOnTheProcessor(freeForm, frequencies, grids), kTolerance) << delayed;
    }
}

TEST(PlantOnTheCard, TheDigitsOfACombinationAreTheProcessors)
{
    ZeroPoleGain plant("P", {uncertain("a")}, {uncertain("b")}, uncertain("k"), Parameter(0.0));
    ParameterGrids grids{{"a", {1.0, 2.0}}, {"b", {3.0, 4.0, 5.0}}, {"k", {6.0, 7.0}}};
    const SweepPlan plan = planSweep(plant, {1.0}, grids, false);
    const gpu::SweepGrid grid = gpu::gridOf(plan);
    ASSERT_EQ(plan.combinationCount, 12u);
    EXPECT_EQ(gpu::digitsOnProcessor(grid, 0), (std::vector<double>{1.0, 3.0, 6.0}));
    EXPECT_EQ(gpu::digitsOnProcessor(grid, 1), (std::vector<double>{2.0, 3.0, 6.0}));
    EXPECT_EQ(gpu::digitsOnProcessor(grid, 2), (std::vector<double>{1.0, 4.0, 6.0}));
    EXPECT_EQ(gpu::digitsOnProcessor(grid, 11), (std::vector<double>{2.0, 5.0, 7.0}));

    ZeroPoleGain twoParameters("P", {uncertain("a")}, {uncertain("b")}, Parameter(1.0), Parameter(0.0));
    ParameterGrids box{{"a", {1.0, 1.5, 2.0}}, {"b", {3.0, 3.5, 4.0}}};
    const SweepPlan border = planSweep(twoParameters, {1.0}, box, true);
    ASSERT_TRUE(border.borderSweepApplied());
    const gpu::SweepGrid borderGrid = gpu::gridOf(border);
    for (std::size_t i = 0; i < border.combinationCount; ++i) {
        EXPECT_EQ(gpu::digitsOnProcessor(borderGrid, i), (std::vector<double>{border.border[i][0], border.border[i][1]}));
    }
    EXPECT_LE(largestGapOnTheProcessor(twoParameters, {0.3, 3.0}, box, true), kTolerance);
}

TEST(PlantOnTheCard, APlantThatDoesNotFitAThreadIsRefused)
{
    std::vector<Parameter> numerator;
    ParameterGrids grids;
    for (int k = 0; k < gpu::kMaxDigits + 1; ++k) {
        numerator.push_back(uncertain("c" + std::to_string(k)));
        grids["c" + std::to_string(k)] = {1.0};
    }
    PolynomialForm plant("P", numerator, {Parameter(1.0), Parameter(1.0)}, Parameter(1.0), Parameter(0.0));
    const SweepPlan plan = planSweep(plant, {1.0}, grids, false);
    EXPECT_THROW(gpu::modelOf(plant, plan), ComputationError);
}

TEST(GpuTemplates, TheSweepOnTheCardGivesTheProcessorsClouds)
{
    QFTBX_REQUIRE_CARD();

    for (const std::filesystem::path & file : examples()) {
        ProjectController processor;
        processor.load(file.string());
        const std::vector<double> epsilon = *processor.epsilon();
        const ParameterGrids grids = processor.sweepGrids();
        ASSERT_TRUE(processor.computeTemplates(epsilon, grids, Processor::Cpu)) << file;

        ProjectController card;
        card.load(file.string());
        ASSERT_TRUE(card.computeTemplates(epsilon, grids, Processor::Gpu)) << file;

        const CloudSet & expected = processor.templates();
        const CloudSet & swept = card.templates();
        ASSERT_EQ(swept.size(), expected.size()) << file;
        double largest = 0.0;
        for (std::size_t u = 0; u < expected.size(); ++u) {
            ASSERT_EQ(swept[u].size(), expected[u].size()) << file;
            for (std::size_t i = 0; i < expected[u].size(); ++i) {
                largest = std::max(largest, relativeGap(swept[u][i], expected[u][i]));
            }
        }
        EXPECT_LE(largest, kTolerance) << file.filename();
        EXPECT_FALSE(card.contour().empty()) << file;
    }
}
