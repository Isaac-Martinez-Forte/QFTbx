/**
 * @file
 * @brief The contours walked on the card are the processor's.
 *
 * Without a card, the code every thread runs is run on the processor, on
 * the clouds of walk_cases.h, the examples at their epsilon, half and
 * twice it, and on the lattices whose walk falls back to the relaxed walk
 * or runs out of steps in it, as one part and as several, which the
 * template engine is first checked to do. It must give the engine's walks
 * exactly: the contour, whether it fell back or ran out of steps, and where
 * each part starts. With a card, the card's walks are compared with the
 * processor's run of the same code, and the contours of the GPU engine,
 * given clouds from a file, with those of the processor's engine: the card
 * computes its angles with its own arc tangent and arc cosine, so a walk
 * may take the other of two candidates the processor finds almost equally
 * good. Such a contour must fall back and run out of steps as the other,
 * have as many parts, and lie within epsilon of it on the plane of the
 * walk; how many come out identical is printed.
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <string>
#include <vector>

#include "src/core/gpu/templates/contour_walk.h"
#include "src/core/gpu/templates/gpu_template_engine.h"
#include "src/core/gpu/templates/walk_preparation.h"
#include "src/core/templates/template_engine.h"
#include "tests/gpu/card.h"
#include "tests/gpu/walk_cases.h"

using namespace qftbx;

namespace {

void expectSame(const std::vector<WalkedContour> & expected, const std::vector<WalkedContour> & found,
                const std::string & where)
{
    ASSERT_EQ(found.size(), expected.size()) << where;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const std::string at = where + ", frequency " + std::to_string(i);
        EXPECT_EQ(found[i].contour, expected[i].contour) << at;
        EXPECT_EQ(found[i].fellBack, expected[i].fellBack) << at;
        EXPECT_EQ(found[i].truncated, expected[i].truncated) << at;
        EXPECT_EQ(found[i].starts, expected[i].starts) << at;
    }
}

struct Equivalence
{
    std::size_t contours = 0;
    std::size_t identical = 0;
};

void expectEquivalent(const gputest::WalkCase & c, const std::vector<WalkedContour> & expected,
                      const std::vector<WalkedContour> & found, Equivalence & tally)
{
    ASSERT_EQ(found.size(), expected.size()) << c.name;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const std::string at = c.name + ", frequency " + std::to_string(i);
        ++tally.contours;
        EXPECT_EQ(found[i].fellBack, expected[i].fellBack) << at;
        EXPECT_EQ(found[i].truncated, expected[i].truncated) << at;
        EXPECT_EQ(found[i].starts.size(), expected[i].starts.size()) << at;
        if (found[i].contour == expected[i].contour) {
            ++tally.identical;
            continue;
        }
        EXPECT_LE(gputest::walkPlaneGap(c.clouds[i], c.metric, found[i].contour, expected[i].contour),
                  c.epsilon[i] * (1.0 + 1e-9)) << at;
    }
}

std::vector<WalkedContour> engineWalks(const TemplateComputation & engine)
{
    std::vector<WalkedContour> walks;
    for (std::size_t i = 0; i < engine.contours().size(); ++i) {
        WalkedContour walk;
        walk.contour = engine.contours()[i];
        walk.fellBack = engine.contourReports()[i].relaxed;
        walk.truncated = engine.contourReports()[i].truncated;
        walk.starts = engine.contourReports()[i].componentStarts;
        walks.push_back(std::move(walk));
    }
    return walks;
}

}

TEST(WalkOnTheCard, TheThreadsCodeWalksTheExamplesAsTheProcessor)
{
    std::size_t checked = 0;
    for (const gputest::WalkCase & c : gputest::exampleCases({1.0, 0.5, 2.0})) {
        expectSame(gputest::walksOnTheProcessor(c), gpu::walkedOnProcessor(c.clouds, c.epsilon, c.metric, 1.0),
                   c.name);
        ++checked;
    }
    EXPECT_EQ(checked, 6 * gputest::exampleFiles().size());
}

TEST(WalkOnTheCard, TheThreadsCodeFallsBackAndRunsOutOfStepsAsTheProcessor)
{
    const std::vector<gputest::WalkCase> fallbacks = gputest::fallbackCases();
    ASSERT_EQ(fallbacks.size(), 4u);
    const std::vector<std::vector<WalkedContour>> engine = {
        gputest::walksOnTheProcessor(fallbacks[0]), gputest::walksOnTheProcessor(fallbacks[1]),
        gputest::walksOnTheProcessor(fallbacks[2]), gputest::walksOnTheProcessor(fallbacks[3])};
    EXPECT_TRUE(engine[0][0].fellBack && !engine[0][0].truncated && engine[0][0].starts.size() == 1);
    EXPECT_TRUE(engine[1][0].fellBack && !engine[1][0].truncated && engine[1][0].starts.size() > 1);
    EXPECT_TRUE(engine[2][0].fellBack && engine[2][0].truncated && engine[2][0].starts.size() == 1);
    EXPECT_TRUE(engine[3][0].fellBack && engine[3][0].truncated && engine[3][0].starts.empty());

    for (std::size_t k = 0; k < fallbacks.size(); ++k) {
        const gputest::WalkCase & c = fallbacks[k];
        expectSame(engine[k], gpu::walkedOnProcessor(c.clouds, c.epsilon, c.metric, 1.0), c.name);
    }
    for (const gputest::WalkCase & c : gputest::smallCases()) {
        expectSame(gputest::walksOnTheProcessor(c), gpu::walkedOnProcessor(c.clouds, c.epsilon, c.metric, 1.0),
                   c.name);
    }
}

TEST(GpuTemplates, TheCardWalksAsTheThreadsCodeOnTheProcessor)
{
    QFTBX_REQUIRE_CARD();

    std::vector<gputest::WalkCase> cases = gputest::exampleCases({1.0, 0.5, 2.0});
    for (std::vector<gputest::WalkCase> more : {gputest::smallCases(), gputest::fallbackCases()}) {
        for (gputest::WalkCase & c : more) {
            cases.push_back(std::move(c));
        }
    }
    Equivalence tally;
    for (const gputest::WalkCase & c : cases) {
        const auto resident = gpu::cloudsOnCard(c.clouds);
        ASSERT_NE(resident, nullptr) << c.name;
        const gpu::WalkedOnCard walked = gpu::walkedOnCard(*resident, c.epsilon, c.metric, 1.0, nullptr);
        EXPECT_FALSE(walked.cancelled) << c.name;
        expectEquivalent(c, gpu::walkedOnProcessor(c.clouds, c.epsilon, c.metric, 1.0), walked.walks, tally);
    }
    std::printf("walks on the card identical to the threads' code on the processor: %zu of %zu\n", tally.identical,
                tally.contours);
}

TEST(GpuTemplates, TheEngineOfTheCardContoursCloudsFromAFileAsTheProcessor)
{
    QFTBX_REQUIRE_CARD();

    Equivalence tally;
    for (const gputest::WalkCase & c : gputest::exampleCases({1.0, 0.5})) {
        TemplateEngine processor;
        processor.setHullMetric(c.metric, 1.0);
        processor.setClouds(c.clouds);
        processor.computeContours(c.epsilon);

        gpu::GpuTemplateEngine card;
        card.setHullMetric(c.metric, 1.0);
        card.setClouds(c.clouds);
        card.computeContours(c.epsilon);

        expectEquivalent(c, engineWalks(processor), engineWalks(card), tally);
        EXPECT_NE(card.resident(), nullptr) << c.name;
    }
    std::printf("contours of the GPU engine identical to the processor's: %zu of %zu\n", tally.identical,
                tally.contours);
}
