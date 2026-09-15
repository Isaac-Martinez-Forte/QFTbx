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
#include "src/core/common/exception.h"
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

//The same facts on the other fixtures that carry templates and a usable
//epsilon. ACC'90 is the one that truncates: its template is a curve (one
//uncertain parameter, 80 points) and the walk of a curve with this epsilon
//either traces it out and back or fails to close; at three of its twelve
//frequencies neither walk closes, and the fixture's own stored contours show
//what that used to produce - 3 points out of 80. The full cloud now stands
//in at those frequencies. multivaluados.qft is left out: it stores an
//epsilon of zero at one frequency, on which no walk was ever possible.
TEST(ContourReport, WhichFixturesTruncateIsPinned)
{
    struct Fixture { const char * file; std::size_t truncated; };
    for (const Fixture f : {Fixture{"planta1.qft", 0}, Fixture{"planta2.qft", 0}, Fixture{"acc90.qft", 3}}) {
        const char * file = f.file;
        ProjectController controller;
        controller.load(std::string(QFTBX_TEST_DATA_DIR) + "/" + file);
        const std::size_t frequencies = controller.omega()->values()->size();
        ASSERT_GT(frequencies, 0u) << file;
        ASSERT_EQ(controller.templates().size(), frequencies) << file;

        TemplateEngine engine;
        engine.setClouds(controller.templates());
        ASSERT_TRUE(engine.computeContours(std::vector<double>(frequencies, 10.0))) << file;

        std::size_t relaxed = 0, truncated = 0, split = 0;
        for (std::size_t i = 0; i < frequencies; ++i) {
            const TemplateEngine::ContourReport & r = engine.contourReports()[i];
            relaxed += r.relaxed ? 1 : 0;
            truncated += r.truncated ? 1 : 0;
            split += r.components > 1 ? 1 : 0;
            if (r.truncated) {
                //The cloud stands in: the contour IS the cloud there.
                EXPECT_EQ(r.contourPoints, r.cloudPoints) << file << " index " << i;
                EXPECT_EQ(engine.contours()[i].size(), controller.templates()[i].size()) << file << " index " << i;
            }
        }
        std::printf("CONTOUR %-14s frequencies=%zu relaxed=%zu truncated=%zu split=%zu\n",
                    file, frequencies, relaxed, truncated, split);
        EXPECT_EQ(truncated, f.truncated) << file;
    }
    std::fflush(stdout);
}

//A contour that does not close is the user's decision, not the engine's: by
//default the whole cloud stands in for it at that frequency and the report
//says so; asked not to, the engine stops with an error naming the frequency.
//The cloud is the one whose hull is null at this epsilon (EHull.TinyEpsilon).
TEST(ContourReport, AContourThatDoesNotCloseIsTheWholeCloudOrAnError)
{
    ComplexCloud square{{0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {0.0, 1.0}};

    TemplateEngine engine;
    engine.setClouds({square});

    ASSERT_TRUE(engine.wholeCloudStandsIn()) << "the safe choice is the default";
    ASSERT_TRUE(engine.computeContours(std::vector<double>(1, 0.1)));
    ASSERT_EQ(engine.contourReports().size(), 1u);
    EXPECT_TRUE(engine.contourReports()[0].wholeCloud);
    EXPECT_EQ(engine.contours()[0].size(), square.size()) << "the whole cloud stands in";

    engine.setWholeCloudStandsIn(false);
    EXPECT_THROW(engine.computeContours(std::vector<double>(1, 0.1)), ComputationError);

    //And with an epsilon that closes, neither path marks anything.
    engine.setWholeCloudStandsIn(true);
    ASSERT_TRUE(engine.computeContours(std::vector<double>(1, 2.5)));
    EXPECT_FALSE(engine.contourReports()[0].wholeCloud);
    EXPECT_LE(engine.contours()[0].size(), 8u);
}
