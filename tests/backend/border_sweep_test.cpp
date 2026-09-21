/**
 * @file
 * @brief Tests of the border sweep of a two-parameter template.
 *
 * With two uncertain parameters the template is sampled round the border of
 * the parameter box instead of over its interior grid: the border of the
 * image of a rectangle lies in the image of its border plus isolated
 * critical values, and the worst closed-loop magnitude over a template is on
 * its border when the pole lies outside. The tests check the point count and
 * order round the box, that one or three parameters still sweep the interior,
 * and on QFT toolbox example 2 that the denser border leads NT to the same
 * gain as the interior sweep (557.0721774) while its check finds a slightly
 * worse plant, and that a border cloud's contour never exceeds the cloud.
 */

#include <gtest/gtest.h>

#include <complex>
#include <cstdio>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/project/settings.h"
#include "src/core/math/range.h"
#include "src/core/math/sequences.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/templates/template_engine.h"

using namespace qftbx;

namespace {

ParameterGrids gridsOf(LtiSystem * plant, int points)
{
    ParameterGrids g;
    const auto add = [&](const Parameter & p) {
        if (p.isUncertain()) g[p.name()] = qftbx::math::linspace(p.range().min, p.range().max, points);
    };
    for (const Parameter & p : plant->numerator()) add(p);
    for (const Parameter & p : plant->denominator()) add(p);
    add(plant->gain());
    add(plant->delay());
    return g;
}

}

TEST(BorderSweep, TwoParametersSpendTheBudgetOnTheFourEdgesInOrder)
{
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter("a", Range(1.0, 2.0), 1.5), Parameter(1.0)};
    PolynomialForm plant("border", numerator, denominator, Parameter("k", Range(1.0, 2.0), 1.5), Parameter(0.0));
    std::vector<double> omega{1.0};

    TemplateEngine engine;
    engine.setGrids(gridsOf(&plant, 5));
    engine.setBorderSweep(true);
    const CloudSet clouds = engine.computeClouds(&plant, &omega);

    ASSERT_TRUE(engine.borderSweepApplied());
    ASSERT_EQ(clouds.size(), 1u);
    ASSERT_EQ(clouds[0].size(), 24u);

    double lo = 1e300, hi = 0.0;
    for (const std::complex<double> & z : clouds[0]) {
        lo = std::min(lo, std::abs(z));
        hi = std::max(hi, std::abs(z));
    }
    EXPECT_NEAR(lo, 1.0 / std::abs(std::complex<double>(2.0, 1.0)), 1e-12);
    EXPECT_NEAR(hi, 2.0 / std::abs(std::complex<double>(1.0, 1.0)), 1e-12);
    for (std::size_t i = 0; i < 24; ++i) {
        EXPECT_LT(std::abs(clouds[0][(i + 1) % 24] - clouds[0][i]), 0.2) << "step " << i;
    }

    engine.setBorderSweep(false);
    EXPECT_EQ(engine.computeClouds(&plant, &omega)[0].size(), 25u);
    EXPECT_FALSE(engine.borderSweepApplied());
}

TEST(BorderSweep, OneOrThreeParametersSweepTheInteriorAsUsual)
{
    std::vector<double> omega{1.0};
    {
        std::vector<Parameter> numerator{Parameter(1.0)};
        std::vector<Parameter> denominator{Parameter("a", Range(1.0, 2.0), 1.5), Parameter(1.0)};
        PolynomialForm plant("one", numerator, denominator, Parameter(1.0), Parameter(0.0));
        TemplateEngine engine;
        engine.setGrids(gridsOf(&plant, 7));
        engine.setBorderSweep(true);
        EXPECT_EQ(engine.computeClouds(&plant, &omega)[0].size(), 7u);
        EXPECT_FALSE(engine.borderSweepApplied());
    }
    {
        std::vector<Parameter> numerator{Parameter("b", Range(1.0, 2.0), 1.5)};
        std::vector<Parameter> denominator{Parameter("a", Range(1.0, 2.0), 1.5), Parameter(1.0)};
        PolynomialForm plant("three", numerator, denominator, Parameter("k", Range(1.0, 2.0), 1.5), Parameter(0.0));
        TemplateEngine engine;
        engine.setGrids(gridsOf(&plant, 3));
        engine.setBorderSweep(true);
        EXPECT_EQ(engine.computeClouds(&plant, &omega)[0].size(), 27u);
        EXPECT_FALSE(engine.borderSweepApplied());
    }
}

TEST(BorderSweep, OnExampleTwoTheBorderIsDenserAndTheBoundariesAgree)
{
    ProjectController controller;
    {
        Settings published;
        published.algorithms.conservativeBoundaryColumns = false;
        controller.applySettings(published);
    }
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    const std::size_t f = controller.omega()->values()->size();

    controller.setEpsilonMetric({HullMetric::Nichols, 1.0});
    controller.setBorderSweep(true);
    const std::vector<TemplateEngine::EpsilonProposal> proposals =
        controller.proposeEpsilon(gridsOf(controller.plant(), 25), {HullMetric::Nichols, 1.0});
    ASSERT_EQ(proposals.size(), f);
    std::vector<double> epsilon;
    for (const auto & p : proposals) {
        EXPECT_TRUE(p.closes);
        epsilon.push_back(p.epsilon);
    }

    ASSERT_TRUE(controller.computeTemplates(epsilon, gridsOf(controller.plant(), 25), false));
    ASSERT_EQ(controller.templates()[0].size(), 624u);
    for (std::size_t i = 0; i < f; ++i) {
        EXPECT_GT(controller.contour()[i].size(), 4u);
    }

    ASSERT_TRUE(controller.computeBoundaries(Range(-360.0, 0.0), 361, Range(-60.0, 160.0), 441,
                                             1.0e6, true, false));
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt, Range(1e-9, 10.0), 100));
    const double k = controller.loopShapingResult()->controller()->gain().nominal();
    const double excess = controller.loopShapingResult()->check()->worstExcessDb;
    std::printf("BORDER ex2 NT: k=%.7f excess=%+.4f dB (interior sweep 557.0721774, +0.0514)\n", k, excess);
    std::fflush(stdout);
    EXPECT_NEAR(k, 557.0721774, 1e-6);
    EXPECT_LE(k, 568.911);
    EXPECT_GT(excess, 0.0514) << "the denser border finds a slightly worse plant";
}

TEST(BorderSweep, TheContourOfABorderCloudIgnoresAnOversizedEpsilon)
{
    ProjectController controller;
    {
        Settings published;
        published.algorithms.conservativeBoundaryColumns = false;
        controller.applySettings(published);
    }
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    const std::size_t f = controller.omega()->values()->size();

    controller.setBorderSweep(true);
    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(f, 10.0), gridsOf(controller.plant(), 25), false));
    for (std::size_t i = 0; i < f; ++i) {
        EXPECT_LE(controller.contour()[i].size(), controller.templates()[i].size() + 1) << "frequency " << i;
        EXPECT_GT(controller.contour()[i].size(), 4u) << "frequency " << i;
    }
}
