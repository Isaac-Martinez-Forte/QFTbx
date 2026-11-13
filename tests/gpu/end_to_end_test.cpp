/**
 * @file
 * @brief From the plant to the controller with the templates and the
 * boundaries on the card.
 *
 * With a card, three examples whose search is quick go the whole way twice,
 * the templates and the boundaries on the processor and on the card, and
 * MC2 on each: toolbox-2 at the file's epsilon, unstable with the
 * boundaries from the whole clouds, which the card already holds, and fopdt
 * at the epsilon proposed on each processor. The verdict, the certificate
 * and the controller must be the same, its gain, zeros and poles within
 * 1e-9 relative, as the GPU battery asks.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/loopshaping/loop_shaping_types.h"
#include "tests/gpu/boundary_checks.h"
#include "tests/gpu/card.h"

using namespace qftbx;

namespace {

struct Way
{
    const char * file;
    bool proposed;
    bool fromContour;
};

struct Design
{
    bool solved = false;
    std::string refusal;
    bool kept = false;
    bool finished = false;
    double gain = 0.0;
    std::vector<double> zeros;
    std::vector<double> poles;
};

Design designed(const Way & way, Processor processor)
{
    ProjectController project;
    project.load((std::filesystem::path(QFTBX_EXAMPLES_DIR) / way.file).string());
    const gputest::Grid g = gputest::gridOf(project);
    const std::vector<double> epsilon = way.proposed ? std::vector<double>() : *project.epsilon();
    EXPECT_TRUE(project.computeTemplates(epsilon, project.sweepGrids(), processor)) << way.file;
    EXPECT_TRUE(project.computeBoundaries(g.phaseRange, g.phaseCount, g.magnitudeRange, g.magnitudeCount, -1.0,
                                          way.fromContour, processor)) << way.file;

    Design design;
    try {
        design.solved = project.computeLoopShaping(0.5, mc2, Range(1e-9, 10.0), 100);
    } catch (const Exception & refused) {
        design.refusal = refused.what();
    }
    if (design.solved) {
        LoopShapingResult & result = *project.loopShapingResult();
        design.kept = result.statistics().certificate.kept;
        design.finished = result.statistics().certificate.finished;
        LtiSystem & controller = *result.controller();
        design.gain = controller.gain().range().min;
        for (Parameter & zero : controller.numerator()) {
            design.zeros.push_back(zero.range().min);
        }
        for (Parameter & pole : controller.denominator()) {
            design.poles.push_back(pole.range().min);
        }
    }
    return design;
}

void expectNear(const std::vector<double> & expected, const std::vector<double> & found, const std::string & what)
{
    ASSERT_EQ(found.size(), expected.size()) << what;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_LE(std::abs(found[i] - expected[i]), 1e-9 * std::abs(expected[i])) << what << " " << i;
    }
}

}

TEST(GpuEndToEnd, TheCardLeadsMc2ToTheProcessorsController)
{
    QFTBX_REQUIRE_CARD();

    for (const Way & way : {Way{"toolbox-2.qft", false, true}, Way{"unstable.qft", false, false},
                            Way{"fopdt.qft", true, true}}) {
        const Design processor = designed(way, Processor::Cpu);
        const Design card = designed(way, Processor::Gpu);
        ASSERT_TRUE(processor.solved) << way.file << ": " << processor.refusal;
        ASSERT_TRUE(card.solved) << way.file << ": " << card.refusal;
        EXPECT_EQ(card.kept, processor.kept) << way.file;
        EXPECT_EQ(card.finished, processor.finished) << way.file;
        expectNear({processor.gain}, {card.gain}, std::string(way.file) + " gain");
        expectNear(processor.zeros, card.zeros, std::string(way.file) + " zeros");
        expectNear(processor.poles, card.poles, std::string(way.file) + " poles");
    }
}
