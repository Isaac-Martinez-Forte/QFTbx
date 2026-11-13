/**
 * @file
 * @brief The alpha-shape contours on the card are the processor's.
 *
 * Without a card, the code every thread runs is run on the processor, on
 * the clouds of walk_cases.h, the examples at their epsilon and half it,
 * and on small clouds: a grid, a ring whose hole is dropped,
 * points in a line walked out and back, separated blocks and isolated
 * points. It must give the template engine's alpha-shape exactly, the
 * contour and where each loop starts, and so at the connecting epsilon the
 * border sweep uses. With a card, the card's contours are compared with
 * the processor's run of the same code, and those of the GPU engine with
 * the processor's engine for the alpha-shape and the border sweep: the
 * card's arc tangent and square root are not the processor's, so on a
 * neighbour almost on a disc or at almost the same angle the two may part.
 * Such a contour must have as many loops and lie within epsilon of the
 * other on the plane of the walk; how many come out identical is printed.
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <string>
#include <vector>

#include "src/core/gpu/templates/alpha_contour.h"
#include "src/core/gpu/templates/gpu_template_engine.h"
#include "src/core/gpu/templates/walk_preparation.h"
#include "src/core/templates/template_engine.h"
#include "tests/gpu/card.h"
#include "tests/gpu/walk_cases.h"

using namespace qftbx;

namespace {

std::vector<WalkedContour> alphaOfTheEngine(const gputest::WalkCase & c)
{
    TemplateEngine engine;
    engine.setHullMetric(c.metric, 1.0);
    std::vector<WalkedContour> contours(c.clouds.size());
    for (std::size_t i = 0; i < c.clouds.size(); ++i) {
        contours[i].contour = engine.alphaShapeContour(c.clouds[i], c.epsilon[i], &contours[i].starts);
    }
    return contours;
}

gputest::WalkCase atTheConnectingEpsilon(gputest::WalkCase c)
{
    TemplateEngine engine;
    engine.setHullMetric(c.metric, 1.0);
    for (std::size_t i = 0; i < c.clouds.size(); ++i) {
        c.epsilon[i] = engine.borderSweepEpsilon(c.clouds[i]);
    }
    c.name += " at the connecting epsilon";
    return c;
}

std::vector<gputest::WalkCase> smallAlphaCases()
{
    ComplexCloud grid;
    gputest::addBlock(grid, {0.0, 0.0}, 9);
    ComplexCloud ring;
    for (const std::complex<double> & z : grid) {
        if (std::abs(z.real() - 4.0) > 1.0 || std::abs(z.imag() - 4.0) > 1.0) {
            ring.push_back(z);
        }
    }
    ComplexCloud blocks;
    gputest::addBlock(blocks, {0.0, 0.0}, 5);
    gputest::addBlock(blocks, {40.0, 0.0}, 5);
    std::vector<gputest::WalkCase> cases{
        {"a grid", {grid}, {1.2}, HullMetric::ComplexPlane},
        {"a ring", {ring}, {1.2}, HullMetric::ComplexPlane},
        {"a line", {{{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}}}, {1.5}, HullMetric::ComplexPlane},
        {"two blocks", {blocks}, {1.5}, HullMetric::ComplexPlane},
        {"isolated points", {{{0, 0}, {2, 0}, {2, 1}, {0, 1}}}, {0.5}, HullMetric::ComplexPlane},
        {"no epsilon", {grid}, {0.0}, HullMetric::ComplexPlane},
    };
    for (gputest::WalkCase & c : gputest::smallCases()) {
        cases.push_back(std::move(c));
    }
    return cases;
}

void expectSame(const std::vector<WalkedContour> & expected, const std::vector<WalkedContour> & found,
                const std::string & where)
{
    ASSERT_EQ(found.size(), expected.size()) << where;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(found[i].contour, expected[i].contour) << where << ", frequency " << i;
        EXPECT_EQ(found[i].starts, expected[i].starts) << where << ", frequency " << i;
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
        ++tally.contours;
        if (found[i].contour == expected[i].contour && found[i].starts == expected[i].starts) {
            ++tally.identical;
            continue;
        }
        EXPECT_EQ(found[i].starts.size(), expected[i].starts.size()) << c.name << ", frequency " << i;
        EXPECT_LE(gputest::walkPlaneGap(c.clouds[i], c.metric, found[i].contour, expected[i].contour),
                  c.epsilon[i] * (1.0 + 1e-9)) << c.name << ", frequency " << i;
    }
}

std::vector<WalkedContour> engineContours(const TemplateComputation & engine)
{
    std::vector<WalkedContour> contours;
    for (std::size_t i = 0; i < engine.contours().size(); ++i) {
        WalkedContour contour;
        contour.contour = engine.contours()[i];
        contour.starts = engine.contourReports()[i].componentStarts;
        contours.push_back(std::move(contour));
    }
    return contours;
}

}

TEST(AlphaOnTheCard, TheThreadsCodeContoursTheExamplesAsTheProcessor)
{
    std::size_t checked = 0;
    for (const gputest::WalkCase & c : gputest::exampleCases({1.0, 0.5})) {
        expectSame(alphaOfTheEngine(c), gpu::alphaOnProcessor(c.clouds, c.epsilon, c.metric, 1.0), c.name);
        ++checked;
    }
    EXPECT_EQ(checked, 4 * gputest::exampleFiles().size());
}

TEST(AlphaOnTheCard, TheThreadsCodeContoursAtTheConnectingEpsilonAsTheProcessor)
{
    for (const gputest::WalkCase & example : gputest::exampleCases({1.0})) {
        const gputest::WalkCase c = atTheConnectingEpsilon(example);
        expectSame(alphaOfTheEngine(c), gpu::alphaOnProcessor(c.clouds, c.epsilon, c.metric, 1.0), c.name);
    }
}

TEST(AlphaOnTheCard, TheThreadsCodeContoursSmallCloudsAsTheProcessor)
{
    for (const gputest::WalkCase & c : smallAlphaCases()) {
        expectSame(alphaOfTheEngine(c), gpu::alphaOnProcessor(c.clouds, c.epsilon, c.metric, 1.0), c.name);
    }
}

TEST(GpuTemplates, TheCardContoursTheAlphaShapeAsTheThreadsCodeOnTheProcessor)
{
    QFTBX_REQUIRE_CARD();

    std::vector<gputest::WalkCase> cases = gputest::exampleCases({1.0, 0.5});
    for (const gputest::WalkCase & example : gputest::exampleCases({1.0})) {
        cases.push_back(atTheConnectingEpsilon(example));
    }
    for (gputest::WalkCase & c : smallAlphaCases()) {
        cases.push_back(std::move(c));
    }
    Equivalence tally;
    for (const gputest::WalkCase & c : cases) {
        const auto resident = gpu::cloudsOnCard(c.clouds);
        ASSERT_NE(resident, nullptr) << c.name;
        const gpu::AlphaOnCard shaped = gpu::alphaOnCard(*resident, c.epsilon, c.metric, 1.0, nullptr);
        EXPECT_FALSE(shaped.cancelled) << c.name;
        expectEquivalent(c, gpu::alphaOnProcessor(c.clouds, c.epsilon, c.metric, 1.0), shaped.contours, tally);
    }
    std::printf("alpha-shapes on the card identical to the threads' code on the processor: %zu of %zu\n",
                tally.identical, tally.contours);
}

TEST(GpuTemplates, TheEngineOfTheCardContoursTheAlphaShapeAsTheProcessor)
{
    QFTBX_REQUIRE_CARD();

    Equivalence tally;
    for (const gputest::WalkCase & c : gputest::exampleCases({1.0, 0.5})) {
        TemplateEngine processor;
        processor.setHullMetric(c.metric, 1.0);
        processor.setAlphaShapeContour(true);
        processor.setClouds(c.clouds);
        processor.computeContours(c.epsilon);

        gpu::GpuTemplateEngine card;
        card.setHullMetric(c.metric, 1.0);
        card.setAlphaShapeContour(true);
        card.setClouds(c.clouds);
        card.computeContours(c.epsilon);

        expectEquivalent(c, engineContours(processor), engineContours(card), tally);
    }
    std::printf("alpha-shapes of the GPU engine identical to the processor's: %zu of %zu\n", tally.identical,
                tally.contours);
}
