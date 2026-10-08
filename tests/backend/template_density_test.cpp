/**
 * @file
 * @brief What a denser template does to the verifier's verdict on a design.
 *
 * The plant family the verifier evaluates is a sample, the one approximation
 * it cannot remove. The MC2 design of QFT toolbox example 2 is checked against
 * a template swept at fifty points per parameter, four times the points of the
 * design. Under the columns reading the verdict and the worst excess must be
 * the same to 1 mdB: what decides the margin there is the reading of the
 * boundary columns, not the density of the template. Under the exact reading
 * the design sits on the edge of the template it was designed with, an excess
 * of minus the tolerance, and the denser template moves that excess by a few
 * millidecibels, at most 5: the exact reading is exact with respect to the
 * sample, which is what its certificate claims and no more.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/math/range.h"
#include "src/core/math/sequences.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/templates/parameter_grids.h"

using namespace qftbx;

namespace {

CloudSet cloudsAt(const std::string & file, int points, std::size_t frequencies)
{
    ProjectController dense;
    dense.load(file);

    ParameterGrids grids;
    const auto add = [&](const Parameter & q) {
        if (q.isUncertain()) {
            grids[q.name()] = qftbx::math::linspace(q.range().min, q.range().max, points);
        }
    };
    for (const Parameter & q : dense.plant()->numerator()) add(q);
    for (const Parameter & q : dense.plant()->denominator()) add(q);
    add(dense.plant()->gain());
    add(dense.plant()->delay());

    dense.computeTemplates(std::vector<double>(frequencies, 0.5), grids, false);
    return dense.templates();
}

}

TEST(TemplateDensity, ADenserTemplateDoesNotChangeTheVerdict)
{
    const std::string file(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft");

    ProjectController controller;
    controller.load(file);
    Settings columns;
    columns.research.mc2Reading = Settings::Research::PointReading::Columns;
    columns.research.mc2.feasibleMagnitude = true;
    controller.applySettings(columns);
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100));

    const std::optional<SpecificationCheck> & asDesigned = controller.loopShapingResult()->check();
    ASSERT_TRUE(asDesigned.has_value());
    EXPECT_TRUE(asDesigned->satisfied()) << "worst excess " << asDesigned->worstExcessDb << " dB";

    LtiSystem * result = controller.loopShapingResult()->controller();
    ASSERT_NE(result, nullptr);

    const std::vector<double> & omega = *controller.omega()->values();
    const SpecificationSet specifications = toSpecificationSet(*controller.specifications());

    for (const int points : {50}) {
        const CloudSet clouds = cloudsAt(file, points, omega.size());
        ASSERT_EQ(clouds.size(), omega.size());

        const SpecificationCheck denser =
            checkAgainstSpecifications(*result, *controller.plant(), omega, clouds, specifications);

        EXPECT_TRUE(denser.satisfied())
            << points << " points per parameter, worst excess " << denser.worstExcessDb << " dB";
        EXPECT_NEAR(denser.worstExcessDb, asDesigned->worstExcessDb, 1e-3)
            << points << " points per parameter";
    }
}

TEST(TemplateDensity, UnderTheExactReadingTheDesignSitsOnTheEdgeOfItsSample)
{
    const std::string file(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft");

    ProjectController controller;
    controller.load(file);
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100));

    const std::optional<SpecificationCheck> & asDesigned = controller.loopShapingResult()->check();
    ASSERT_TRUE(asDesigned.has_value());
    EXPECT_TRUE(asDesigned->satisfied());
    EXPECT_NEAR(asDesigned->worstExcessDb, 0.0, 1e-9) << "the exact design is on the edge of its own template";

    const std::vector<double> & omega = *controller.omega()->values();
    const CloudSet clouds = cloudsAt(file, 50, omega.size());
    const SpecificationCheck denser = checkAgainstSpecifications(*controller.loopShapingResult()->controller(),
                                                                 *controller.plant(), omega, clouds,
                                                                 toSpecificationSet(*controller.specifications()));
    std::printf("TEMPLATE-DENSITY exact design: %.3g dB on its template, %.4f dB at 50 points per parameter\n",
                asDesigned->worstExcessDb, denser.worstExcessDb);
    EXPECT_LT(std::abs(denser.worstExcessDb), 5e-3) << "a denser sample moves the edge by millidecibels";
}
