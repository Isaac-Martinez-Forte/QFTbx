// What the epsilon-hull reports about itself, per design frequency.
//
// The walk has two ways of not being the canonical epsilon-hull. When the
// faithful walk does not close it falls back to the relaxed historical walk,
// and that walk can stop at its step limit with a PARTIAL contour. Both used
// to be a line on the error stream, which a benchmark, a test or a script
// never reads. The engine now keeps them as data, one report per frequency.
//
// On the QFT toolbox example 2 with the epsilon its file carries (10 at
// every frequency) the faithful walk fails to close at five of the six
// frequencies: the epsilon is far above what the clouds need (at w = 100 it
// is a thousand times the cloud's own diameter), and the walk degenerates.
// None of the six truncates. Those two facts are pinned here, as the state
// of the chain before the epsilon is measured in the right units.

#include <gtest/gtest.h>

#include <cstdio>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/templates/template_engine.h"

using namespace qftbx;

TEST(ContourReport, OneReportPerFrequencyAndTheSizesAdd)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));

    const std::size_t frequencies = controller.omega()->values()->size();
    ASSERT_EQ(controller.templates().size(), frequencies);

    TemplateEngine engine;
    engine.setClouds(controller.templates());
    ASSERT_TRUE(engine.computeContours(std::vector<double>(frequencies, 10.0)));

    const std::vector<TemplateEngine::ContourReport> & reports = engine.contourReports();
    ASSERT_EQ(reports.size(), frequencies);

    std::size_t relaxed = 0, truncated = 0;
    for (std::size_t i = 0; i < frequencies; ++i) {
        const TemplateEngine::ContourReport & r = reports[i];
        EXPECT_EQ(r.cloudPoints, controller.templates()[i].size());
        EXPECT_EQ(r.contourPoints, engine.contours()[i].size());
        EXPECT_GT(r.contourPoints, 0u);
        EXPECT_LE(r.contourPoints, 3 * r.cloudPoints) << "the walk visits at most three times the cloud";
        relaxed += r.relaxed ? 1 : 0;
        truncated += r.truncated ? 1 : 0;
        std::printf("CONTOUR w=%-6g cloud=%zu contour=%zu relaxed=%d truncated=%d\n",
                    controller.omega()->values()->at(i), r.cloudPoints, r.contourPoints,
                    r.relaxed ? 1 : 0, r.truncated ? 1 : 0);
    }
    std::fflush(stdout);

    //The state of the chain with this epsilon (see the header).
    EXPECT_EQ(relaxed, 5u) << "five of six frequencies fell back to the relaxed walk";
    EXPECT_EQ(truncated, 0u) << "none of them truncated";
}
