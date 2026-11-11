/**
 * @file
 * @brief The boundaries read on the card are the processor's.
 *
 * Without a card, the threads' own code reads sheets on the processor. On
 * sheets of random noise, with infinite cells and cells right at the
 * height, where traces wind, start on cells that give one point and touch
 * the frame, its traces are ContourTracer's to the bit and its columns
 * BoundaryColumns::fromSheet's. On the sheets of examples, from the contour
 * and from the cloud, BoundaryEngine gives the same boundaries from what it
 * reads and from its own reading of the same sheets: the same traces and
 * labels, so the same union, and the same column ends to the last bits
 * that fusing products and sums can move. With a card, what the card
 * reads off its sheets is what the threads' code reads off the same sheets
 * on the processor.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <filesystem>
#include <limits>
#include <random>
#include <string>
#include <vector>

#include "src/core/boundaries/boundary_columns.h"
#include "src/core/boundaries/boundary_engine.h"
#include "src/core/boundaries/contour_tracer.h"
#include "src/core/gpu/boundaries/boundary_reading.h"
#include "src/core/gpu/boundaries/gpu_boundary_engine.h"
#include "tests/gpu/boundary_checks.h"
#include "tests/gpu/card.h"

using namespace qftbx;
using namespace qftbx::gputest;
using Complex = std::complex<double>;

namespace {

constexpr double kRelative = 1e-12;
constexpr double kInfinity = std::numeric_limits<double>::infinity();

bool closeEnough(double a, double b)
{
    if (std::isinf(a) || std::isinf(b)) {
        return a == b;
    }
    return std::abs(a - b) <= kRelative * std::max(1.0, std::abs(a));
}

bool sameColumns(const BoundaryColumns & a, const BoundaryColumns & b)
{
    if (a.columnCount() != b.columnCount()) {
        return false;
    }
    for (std::int32_t c = 0; c < a.columnCount(); ++c) {
        const std::vector<BoundaryColumns::Span> x = a.spans(c);
        const std::vector<BoundaryColumns::Span> y = b.spans(c);
        if (x.size() != y.size()) {
            return false;
        }
        for (std::size_t i = 0; i < x.size(); ++i) {
            if (!closeEnough(x[i].lo, y[i].lo) || !closeEnough(x[i].hi, y[i].hi)) {
                return false;
            }
        }
    }
    return true;
}

BoundarySheet noise(std::mt19937 & random, std::int32_t columns, std::int32_t rows, double height)
{
    std::uniform_real_distribution<double> value(-1.0, 1.0);
    std::uniform_int_distribution<int> special(0, 40);
    const double tilt = value(random);
    BoundarySheet sheet(static_cast<std::size_t>(rows), std::vector<double>(static_cast<std::size_t>(columns)));
    for (std::int32_t r = 0; r < rows; ++r) {
        for (std::int32_t c = 0; c < columns; ++c) {
            double v = value(random) + tilt * std::sin(0.4 * c + 0.3 * r);
            const int roll = special(random);
            v = roll == 0 ? kInfinity : (roll == 1 ? height : v);
            sheet[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = v;
        }
    }
    return sheet;
}

struct Readings
{
    std::vector<std::vector<SheetReading>> named;
    std::vector<std::vector<gpu::ReadingRequest>> requests;
};

Readings readingsOf(const BoundaryPlan & plan, const std::vector<double> & omega)
{
    Readings readings;
    for (std::size_t u = 0; u < omega.size(); ++u) {
        readings.named.push_back(plan.readings(u, omega[u]));
        readings.requests.push_back(gpu::requestsOf(readings.named.back()));
    }
    return readings;
}

void expectSameReading(const gpu::ReadingOnCard & card, const gpu::ReadingOnCard & processor, const std::string & where)
{
    EXPECT_EQ(card.traceStart, processor.traceStart) << where;
    EXPECT_EQ(card.x, processor.x) << where;
    EXPECT_EQ(card.y, processor.y) << where;
    EXPECT_EQ(card.allowedBelow, processor.allowedBelow) << where;
    EXPECT_EQ(card.columnBegin, processor.columnBegin) << where;
    ASSERT_EQ(card.lo.size(), processor.lo.size()) << where;
    for (std::size_t i = 0; i < card.lo.size(); ++i) {
        EXPECT_TRUE(closeEnough(card.lo[i], processor.lo[i]) && closeEnough(card.hi[i], processor.hi[i]))
                << where << " interval " << i;
    }
}

}

TEST(BoundaryReading, TheTracesAndColumnsOfRandomSheetsAreTheProcessorsOnes)
{
    std::mt19937 random(20261111);
    const Range phaseRange(-360.0, 0.0);
    const Range magnitudeRange(-30.0, 40.0);
    std::size_t traces = 0;
    for (int trial = 0; trial < 60; ++trial) {
        const std::int32_t columns = 3 + (trial * 5) % 31;
        const std::int32_t rows = 3 + (trial * 7) % 37;
        const double height = 0.1 * (trial % 7) - 0.3;
        const BoundarySheet sheet = noise(random, columns, rows, height);

        const std::vector<ComplexCloud> templates{{Complex(1.0, 0.0), Complex(0.8, 0.3), Complex(1.2, -0.2)}};
        gpu::SheetProblem problem;
        problem.templates = &templates;
        problem.nominal = {Complex(1.0, 0.0)};
        problem.sheets = {{true, true, true, true, true}};
        problem.rowMagnitudes.assign(static_cast<std::size_t>(rows), 1.0);
        problem.cosines.assign(static_cast<std::size_t>(columns), 1.0);
        problem.sines.assign(static_cast<std::size_t>(columns), 0.0);
        std::vector<BoundarySheets> sheets(1);
        sheets[0][0] = sheet;

        const std::vector<SheetReading> named{{"Stability", 0, height}};
        const auto read = gpu::readingsOnProcessor(problem, sheets, {gpu::requestsOf(named)}, phaseRange, magnitudeRange);
        const FrequencyBoundaries mine = gpu::boundariesOf(named, read[0], phaseRange, columns, magnitudeRange, rows);

        ContourTracer tracer(height, sheet);
        const TraceSet expected = tracer.trace(phaseRange.width(), magnitudeRange.width(), phaseRange.min,
                                               magnitudeRange.min);
        EXPECT_EQ(mine.traces.at("Stability"), expected) << "trial " << trial;
        traces += expected.size();

        const auto cell = [&sheet](std::int32_t c, std::int32_t r) {
            return sheet[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
        };
        EXPECT_TRUE(sameColumns(mine.columns.at("Stability"),
                                BoundaryColumns::fromSheet(cell, height, columns, phaseRange, rows, magnitudeRange)))
                << "trial " << trial;
    }
    EXPECT_GT(traces, 100u);
}

TEST(BoundaryReading, TheEngineGivesTheSameBoundariesFromTheThreadsReadingAsFromItsOwn)
{
    for (const char * name : {"toolbox-1.qft", "dcm-T33.qft", "msf.qft", "unstable.qft"}) {
        for (const bool fromContour : {true, false}) {
            Loaded loaded;
            ASSERT_TRUE(load(loaded, std::filesystem::path(QFTBX_EXAMPLES_DIR) / name)) << name;
            const Grid g{loaded.grid.phaseRange, 91, loaded.grid.magnitudeRange, 81};
            const CloudSet & templates = fromContour ? loaded.project.contour() : loaded.project.templates();
            BoundaryPlan plan = planBoundaries(loaded.omega, *loaded.project.specifications(), g.phaseRange,
                                               g.phaseCount, g.magnitudeRange, g.magnitudeCount);
            const gpu::SheetProblem problem = gpu::sheetProblemOf(loaded.omega, *loaded.project.plant(), templates,
                                                                  fromContour, true, plan, g.phaseRange, g.phaseCount,
                                                                  g.magnitudeRange, g.magnitudeCount);
            std::vector<BoundarySheets> sheets = sheetsOnProcessor(problem, plan);
            const Readings readings = readingsOf(plan, loaded.omega);
            const auto read = gpu::readingsOnProcessor(problem, sheets, readings.requests, g.phaseRange,
                                                       g.magnitudeRange);
            std::vector<FrequencyBoundaries> boundaries;
            for (std::size_t u = 0; u < loaded.omega.size(); ++u) {
                boundaries.push_back(gpu::boundariesOf(readings.named[u], read[u], g.phaseRange, g.phaseCount,
                                                       g.magnitudeRange, g.magnitudeCount));
            }

            std::vector<double> omega = loaded.omega;
            BoundaryEngine own;
            own.adoptSheets(&omega, loaded.project.plant(), templates, fromContour,
                            planBoundaries(loaded.omega, *loaded.project.specifications(), g.phaseRange,
                                           g.phaseCount, g.magnitudeRange, g.magnitudeCount),
                            g.phaseRange, g.phaseCount, g.magnitudeRange, g.magnitudeCount, std::move(sheets));
            BoundaryEngine threads;
            threads.adoptBoundaries(&omega, loaded.project.plant(), templates, fromContour, std::move(plan),
                                    g.phaseRange, g.phaseCount, g.magnitudeRange, g.magnitudeCount,
                                    std::move(boundaries));

            const Gap gap = compareBoundaries(own.boundaryData(), threads.boundaryData());
            const std::string where = std::string(name) + (fromContour ? " contour" : " cloud");
            EXPECT_EQ(gap.differ, 0u) << where;
            EXPECT_EQ(gap.traces, 0.0) << where;
            EXPECT_LE(gap.columns, 1e-12) << where;
        }
    }
}

TEST(GpuBoundaries, TheReadingsOfTheCardAreTheThreadsCodeOnTheProcessor)
{
    QFTBX_REQUIRE_CARD();

    for (const char * name : {"toolbox-1.qft", "dcm-T33.qft", "acc90.qft", "maglev-upper.qft", "msf.qft"}) {
        for (const bool fromContour : {true, false}) {
            Loaded loaded;
            ASSERT_TRUE(load(loaded, std::filesystem::path(QFTBX_EXAMPLES_DIR) / name)) << name;
            const Grid g{loaded.grid.phaseRange, 91, loaded.grid.magnitudeRange, 81};
            const CloudSet & templates = fromContour ? loaded.project.contour() : loaded.project.templates();
            const BoundaryPlan plan = planBoundaries(loaded.omega, *loaded.project.specifications(), g.phaseRange,
                                                     g.phaseCount, g.magnitudeRange, g.magnitudeCount);
            const gpu::SheetProblem problem = gpu::sheetProblemOf(loaded.omega, *loaded.project.plant(), templates,
                                                                  fromContour, true, plan, g.phaseRange, g.phaseCount,
                                                                  g.magnitudeRange, g.magnitudeCount);
            const Readings readings = readingsOf(plan, loaded.omega);

            const gpu::SheetsOnCard sheets = gpu::sheetsOnCard(problem, nullptr);
            const auto expected = gpu::readingsOnProcessor(problem, sheets.sheets, readings.requests, g.phaseRange,
                                                           g.magnitudeRange);
            const gpu::BoundariesOnCard card = gpu::boundariesOnCard(problem, readings.requests, g.phaseRange,
                                                                     g.magnitudeRange, nullptr);
            ASSERT_FALSE(card.cancelled);
            ASSERT_EQ(card.readings.size(), expected.size()) << name;
            for (std::size_t u = 0; u < expected.size(); ++u) {
                ASSERT_EQ(card.readings[u].size(), expected[u].size()) << name;
                for (std::size_t k = 0; k < expected[u].size(); ++k) {
                    expectSameReading(card.readings[u][k], expected[u][k],
                                      std::string(name) + (fromContour ? " contour" : " cloud") + " frequency "
                                              + std::to_string(u) + " " + readings.named[u][k].name);
                }
            }
        }
    }
}
