/**
 * @file
 * @brief The sheets of the card are the processor's.
 *
 * Without a card, the code every thread runs is run on the processor. On
 * every example, at a spread of cells of every frequency, from the contour
 * and from the whole cloud, with the guard near the singular locus and
 * without it, the five values of a cell must be the processor's within
 * 1e-9 dB, or the same infinity. So must cells on the contour, level with
 * its vertices or just off its edges, where the winding number is taken in
 * double precision; samples single precision cannot tell apart; and a
 * template too large for single precision. A cloud's spacings are
 * SingularLocus's. The sheets of the whole grid, read by BoundaryEngine,
 * give the boundaries it computes itself. With a card, the sheets the card
 * computes are the processor's cell by cell, on a coarser grid of five
 * examples in all four ways, and the engine of the GPU gives the
 * processor's boundaries on every example.
 *
 * The card takes P0/P itself, and its last bit may differ from the
 * processor's. A cell's values are |N| over its distance d to the template,
 * so that difference grows as the template's scale over d: the tolerance
 * adds 1e-14 dB times that ratio, and within 1e-12 of the scale both sides
 * only have to violate every bound, above 150 dB or infinite. A cloud's
 * guard widens by the spacing of the nearest sample, and which of several
 * samples at the same distance to double precision is the nearest depends
 * on the last bit on either side: the tolerance adds what the widening
 * spans over the samples within 1e-14 of the nearest distance, and the
 * widening d / (d - h/2) magnifies the last bits of d and h as d nears
 * h/2, which it adds in the same way as the distance.
 *
 * The traces run through the nodes of the grid, on whichever side of its
 * bound each cell lies, so they and the unions must come out the same. The
 * ends of the columns are interpolated where a sheet crosses its bound,
 * dividing by the difference between neighbouring cells, which magnifies
 * the last bits of a sheet that is nearly flat there; they are compared
 * within 1e-6 dB.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/boundaries/boundary_engine.h"
#include "src/core/boundaries/boundary_plan.h"
#include "src/core/boundaries/closed_loop_worst_case.h"
#include "src/core/boundaries/singular_locus.h"
#include "src/core/gpu/boundaries/gpu_boundary_engine.h"
#include "src/core/gpu/boundaries/sheet_sweep.h"
#include "src/core/project/settings.h"
#include "tests/gpu/card.h"

using namespace qftbx;
using Complex = std::complex<double>;

namespace {

constexpr double kToleranceDb = 1e-9;
constexpr double kConditionDb = 1e-14;
constexpr double kOnTheTemplate = 1e-12;
constexpr double kViolatingDb = 150.0;
constexpr double kColumnToleranceDb = 1e-6;
constexpr double kInfinity = std::numeric_limits<double>::infinity();

struct Grid
{
    Range phaseRange;
    std::int32_t phaseCount = 0;
    Range magnitudeRange;
    std::int32_t magnitudeCount = 0;
};

Grid gridOf(ProjectController & project)
{
    const Settings defaults;
    if (const BoundaryData * stored = project.boundaries()) {
        return {stored->phaseRange(), stored->phaseCount(), stored->magnitudeRange(), stored->magnitudeCount()};
    }
    return {Range(defaults.defaults.phaseStart, defaults.defaults.phaseEnd), defaults.defaults.phasePoints,
            Range(defaults.defaults.magnitudeStart, defaults.defaults.magnitudeEnd), defaults.defaults.magnitudePoints};
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

double violatingDb(double db)
{
    return std::isnan(db) ? kInfinity : db;
}

struct ProcessorCell
{
    std::array<double, 5> values;
    double distance;
    double tieDb = 0.0;
};

ProcessorCell processorCell(Complex p0, Complex L, const ComplexCloud & p, const std::vector<Complex> & q,
                            const SingularLocus * locus)
{
    const WorstCase sampled = worstCaseAt(p0, L, p, q, WorstCaseMask::all());
    const WorstCase w = locus != nullptr ? locus->guard(sampled, L, p0) : sampled;
    const double stability = 20 * std::log10(w.stabilityNoise);
    double distance = sampled.nearestSample;
    if (locus != nullptr && locus->isContour()) {
        distance = std::min(distance, locus->borderDistance(-L));
    }
    double tieDb = 0.0;
    if (locus != nullptr && !locus->isContour() && distance > 0.0) {
        double scale = 0.0;
        for (const Complex & value : q) {
            scale = std::max(scale, std::abs(value));
        }
        double least = kInfinity;
        double most = 0.0;
        double condition = 0.0;
        for (std::size_t i = 0; i < q.size(); ++i) {
            if (std::abs(q[i] + L) <= distance * (1.0 + 1e-14)) {
                const double half = 0.5 * locus->spacings()[i];
                const double widen = distance > half ? distance / (distance - half) : kInfinity;
                least = std::min(least, widen);
                most = std::max(most, widen);
                if (distance > half && half > 0.0) {
                    condition = std::max(condition, half / (distance - half) * (scale / distance + scale / half));
                }
            }
        }
        tieDb = (most > least ? 2.0 * 20 * std::log10(most / least) : 0.0) + kConditionDb * condition;
    }
    return {{violatingDb(stability), violatingDb(stability - (20 * std::log10(w.trackingMin))),
             violatingDb(20 * std::log10(w.outputDisturbance)), violatingDb(20 * std::log10(w.inputDisturbance)),
             violatingDb(20 * std::log10(w.controlEffort))},
            distance, tieDb};
}

double scaleOf(const std::vector<Complex> & q)
{
    double scale = 0.0;
    for (const Complex & value : q) {
        scale = std::max(scale, std::abs(value));
    }
    return scale;
}

struct Tally
{
    std::size_t cells = 0;
    std::size_t differ = 0;
    double largest = 0.0;
    std::string first;
};

void compareCell(const ProcessorCell & processor, const std::array<double, 5> & got, double scale, Tally & tally,
                 const std::string & where)
{
    ++tally.cells;
    const std::array<double, 5> & expected = processor.values;
    const bool onTheTemplate = !(processor.distance > kOnTheTemplate * scale);
    const double tolerance = kToleranceDb + kConditionDb * scale / processor.distance + processor.tieDb;
    for (std::size_t k = 0; k < 5; ++k) {
        if (onTheTemplate) {
            if (!(expected[k] >= kViolatingDb && got[k] >= kViolatingDb) && tally.differ++ == 0) {
                tally.first = where + " sheet " + std::to_string(k) + " on the template: processor "
                        + std::to_string(expected[k]) + ", card " + std::to_string(got[k]);
            }
            continue;
        }
        const bool infinite = std::isinf(expected[k]) || std::isinf(got[k]);
        const double gap = infinite ? 0.0 : std::abs(expected[k] - got[k]);
        if ((infinite && expected[k] != got[k]) || gap > tolerance) {
            if (tally.differ++ == 0) {
                tally.first = where + " sheet " + std::to_string(k) + ": processor " + std::to_string(expected[k])
                        + ", card " + std::to_string(got[k]);
            }
        }
        tally.largest = std::max(tally.largest, gap);
    }
}

struct Loaded
{
    ProjectController project;
    Grid grid;
    std::vector<double> omega;
};

bool load(Loaded & loaded, const std::filesystem::path & file)
{
    loaded.project.load(file.string());
    loaded.grid = gridOf(loaded.project);
    const std::vector<double> epsilon = *loaded.project.epsilon();
    if (!loaded.project.computeTemplates(epsilon, loaded.project.sweepGrids(), Processor::Cpu)) {
        return false;
    }
    loaded.omega = *loaded.project.omega()->values();
    return true;
}

Tally spreadOfCells(bool fromContour, bool guard)
{
    Tally tally;
    for (const std::filesystem::path & file : examples()) {
        Loaded loaded;
        EXPECT_TRUE(load(loaded, file)) << file;
        const CloudSet & templates = fromContour ? loaded.project.contour() : loaded.project.templates();
        const Grid & g = loaded.grid;
        const BoundaryPlan plan = planBoundaries(loaded.omega, *loaded.project.specifications(), g.phaseRange,
                                                 g.phaseCount, g.magnitudeRange, g.magnitudeCount);
        const gpu::SheetProblem problem = gpu::sheetProblemOf(loaded.omega, *loaded.project.plant(), templates,
                                                              fromContour, guard, plan, g.phaseRange, g.phaseCount,
                                                              g.magnitudeRange, g.magnitudeCount);
        const std::size_t rows = problem.rowMagnitudes.size();
        const std::size_t columns = problem.cosines.size();
        const std::size_t rowStep = std::max<std::size_t>(1, rows / 17);
        const std::size_t columnStep = std::max<std::size_t>(1, columns / 16);
        for (std::size_t u = 0; u < templates.size(); ++u) {
            const gpu::SheetsOnProcessor card(problem, u);
            const Complex p0 = problem.nominal[u];
            const std::vector<Complex> q = nominalOverValueSet(p0, templates[u]);
            std::optional<SingularLocus> locus;
            if (guard) {
                locus.emplace(q, fromContour);
            }
            const double scale = scaleOf(q);
            for (std::size_t row = u % rowStep; row < rows; row += rowStep) {
                for (std::size_t column = (3 * u) % columnStep; column < columns; column += columnStep) {
                    const Complex L(problem.rowMagnitudes[row] * problem.cosines[column],
                                    problem.rowMagnitudes[row] * problem.sines[column]);
                    compareCell(processorCell(p0, L, templates[u], q, locus ? &*locus : nullptr),
                                card.cell(row, column), scale, tally,
                                file.filename().string() + " frequency " + std::to_string(u) + " cell "
                                        + std::to_string(row) + "," + std::to_string(column));
                }
            }
        }
    }
    return tally;
}

gpu::SheetProblem problemAt(const std::vector<ComplexCloud> & templates, Complex p0, const std::vector<Complex> & loops,
                            bool contours, bool guard)
{
    gpu::SheetProblem problem;
    problem.templates = &templates;
    problem.nominal = {p0};
    problem.sheets = {{true, true, true, true, true}};
    problem.contours = contours;
    problem.guard = guard;
    for (const Complex & L : loops) {
        const double r = std::abs(L);
        problem.rowMagnitudes.push_back(r);
        problem.cosines.push_back(L.real() / r);
        problem.sines.push_back(L.imag() / r);
    }
    return problem;
}

Tally diagonal(const gpu::SheetProblem & problem, const std::string & name)
{
    Tally tally;
    const ComplexCloud & p = problem.templates->front();
    const Complex p0 = problem.nominal.front();
    const std::vector<Complex> q = nominalOverValueSet(p0, p);
    std::optional<SingularLocus> locus;
    if (problem.guard) {
        locus.emplace(q, problem.contours);
    }
    const gpu::SheetsOnProcessor card(problem, 0);
    const double scale = scaleOf(q);
    for (std::size_t c = 0; c < problem.cosines.size(); ++c) {
        const Complex L(problem.rowMagnitudes[c] * problem.cosines[c], problem.rowMagnitudes[c] * problem.sines[c]);
        compareCell(processorCell(p0, L, p, q, locus ? &*locus : nullptr), card.cell(c, c), scale, tally,
                    name + " cell " + std::to_string(c));
    }
    return tally;
}

struct Gap
{
    std::size_t differ = 0;
    double traces = 0.0;
    double columns = 0.0;
};

void compareValue(double a, double b, Gap & gap, double & largest)
{
    if (std::isinf(a) || std::isinf(b)) {
        gap.differ += a == b ? 0 : 1;
        return;
    }
    largest = std::max(largest, std::abs(a - b));
}

void compareTraces(const TraceSet & a, const TraceSet & b, Gap & gap)
{
    if (a.size() != b.size()) {
        ++gap.differ;
        return;
    }
    for (std::size_t t = 0; t < a.size(); ++t) {
        if (a[t].size() != b[t].size()) {
            ++gap.differ;
            continue;
        }
        for (std::size_t i = 0; i < a[t].size(); ++i) {
            compareValue(a[t][i].phase, b[t][i].phase, gap, gap.traces);
            compareValue(a[t][i].magnitude, b[t][i].magnitude, gap, gap.traces);
        }
    }
}

Gap compareBoundaries(const BoundaryData & a, const BoundaryData & b)
{
    Gap gap;
    const BoundarySet & boundsA = a.boundaries();
    const BoundarySet & boundsB = b.boundaries();
    if (boundsA.size() != boundsB.size()) {
        ++gap.differ;
        return gap;
    }
    for (std::size_t f = 0; f < boundsA.size(); ++f) {
        if (boundsA[f].size() != boundsB[f].size()) {
            ++gap.differ;
            continue;
        }
        for (const auto & [name, traces] : boundsA[f]) {
            const auto other = boundsB[f].find(name);
            if (other == boundsB[f].end()) {
                ++gap.differ;
                continue;
            }
            compareTraces(traces, other->second, gap);
        }
    }
    compareTraces(a.unionBoundaries(), b.unionBoundaries(), gap);
    gap.differ += a.openFlags() == b.openFlags() ? 0 : 1;
    gap.differ += a.upperFlags() == b.upperFlags() ? 0 : 1;

    const ColumnSet & columnsA = a.specificationColumns();
    const ColumnSet & columnsB = b.specificationColumns();
    if (columnsA.size() != columnsB.size()) {
        ++gap.differ;
        return gap;
    }
    for (std::size_t f = 0; f < columnsA.size(); ++f) {
        for (const auto & [name, columns] : columnsA[f]) {
            const auto other = columnsB[f].find(name);
            if (other == columnsB[f].end() || other->second.columnCount() != columns.columnCount()) {
                ++gap.differ;
                continue;
            }
            for (std::int32_t c = 0; c < columns.columnCount(); ++c) {
                const std::vector<BoundaryColumns::Span> spansA = columns.spans(c);
                const std::vector<BoundaryColumns::Span> spansB = other->second.spans(c);
                if (spansA.size() != spansB.size()) {
                    ++gap.differ;
                    continue;
                }
                for (std::size_t s = 0; s < spansA.size(); ++s) {
                    compareValue(spansA[s].lo, spansB[s].lo, gap, gap.columns);
                    compareValue(spansA[s].hi, spansB[s].hi, gap, gap.columns);
                }
            }
        }
    }
    return gap;
}

}

TEST(SheetsOnTheCard, TheThreadsCodeGivesTheProcessorsCellsFromTheContourWithTheGuard)
{
    const Tally tally = spreadOfCells(true, true);
    EXPECT_EQ(tally.differ, 0u) << tally.first << " (largest gap " << tally.largest << " dB)";
    EXPECT_GT(tally.cells, 10000u);
}

TEST(SheetsOnTheCard, TheThreadsCodeGivesTheProcessorsCellsFromTheContourWithoutTheGuard)
{
    const Tally tally = spreadOfCells(true, false);
    EXPECT_EQ(tally.differ, 0u) << tally.first << " (largest gap " << tally.largest << " dB)";
}

TEST(SheetsOnTheCard, TheThreadsCodeGivesTheProcessorsCellsFromTheCloudWithTheGuard)
{
    const Tally tally = spreadOfCells(false, true);
    EXPECT_EQ(tally.differ, 0u) << tally.first << " (largest gap " << tally.largest << " dB)";
}

TEST(SheetsOnTheCard, TheThreadsCodeGivesTheProcessorsCellsFromTheCloudWithoutTheGuard)
{
    const Tally tally = spreadOfCells(false, false);
    EXPECT_EQ(tally.differ, 0u) << tally.first << " (largest gap " << tally.largest << " dB)";
}

TEST(SheetsOnTheCard, CellsOnTheContourLevelWithItsVerticesOrJustOffItsEdgesComeOutTheSame)
{
    Loaded loaded;
    ASSERT_TRUE(load(loaded, std::filesystem::path(QFTBX_EXAMPLES_DIR) / "toolbox-1.qft"));
    const CloudSet & contours = loaded.project.contour();
    for (std::size_t u = 0; u < contours.size(); ++u) {
        const std::vector<ComplexCloud> one{contours[u]};
        const Complex p0 = loaded.project.plant()->evaluate(loaded.omega[u]);
        const std::vector<Complex> q = nominalOverValueSet(p0, contours[u]);
        double scale = 0.0;
        for (const Complex & value : q) {
            scale = std::max(scale, std::abs(value));
        }
        std::vector<Complex> loops;
        for (std::size_t j = 0; j + 1 < q.size(); j += std::max<std::size_t>(1, q.size() / 40)) {
            const Complex a = q[j];
            const Complex b = q[j + 1];
            const Complex middle = 0.5 * (a + b);
            const Complex normal = b == a ? Complex(0.0, 1.0) : Complex(0.0, 1.0) * (b - a) / std::abs(b - a);
            for (const Complex & z : {a, middle, middle + 1e-13 * scale * normal, middle - 1e-13 * scale * normal,
                                      a + Complex(1e-3 * scale, 0.0), a - Complex(0.5 * scale, 0.0),
                                      a + Complex(2.0 * scale, 0.0)}) {
                if (std::abs(z) > 0.0) {
                    loops.push_back(-z);
                }
            }
        }
        const Tally tally = diagonal(problemAt(one, p0, loops, true, true), "frequency " + std::to_string(u));
        EXPECT_EQ(tally.differ, 0u) << tally.first << " (largest gap " << tally.largest << " dB)";
    }
}

TEST(SheetsOnTheCard, SamplesSinglePrecisionCannotTellApartAreDecidedInDouble)
{
    const Complex p0(0.8, -0.3);
    std::vector<ComplexCloud> templates(1);
    for (int i = 0; i < 300; ++i) {
        const double t = 1e-9 * static_cast<double>(i % 37) + 3e-10 * static_cast<double>(i / 37);
        templates[0].push_back(p0 * Complex(1.0 + t, 1e-9 * std::sin(static_cast<double>(i))));
    }
    std::vector<Complex> loops;
    for (int k = 0; k < 60; ++k) {
        const double angle = 0.1 * static_cast<double>(k);
        for (const double r : {1e-4, 0.5, 0.999999, 1.0, 1.000001, 3.0, 1e5}) {
            loops.push_back(std::polar(r, angle));
        }
    }
    for (const bool contours : {true, false}) {
        for (const bool guard : {true, false}) {
            const Tally tally = diagonal(problemAt(templates, p0, loops, contours, guard), "cluster");
            EXPECT_EQ(tally.differ, 0u) << contours << guard << " " << tally.first << " (largest gap " << tally.largest << " dB)";
        }
    }
}

TEST(SheetsOnTheCard, ATemplateTooLargeForSinglePrecisionIsDecidedInDouble)
{
    const Complex p0(1.0, 0.5);
    std::vector<ComplexCloud> templates(1);
    for (int i = 0; i < 64; ++i) {
        templates[0].push_back(std::polar(1e-21 * (1.0 + 0.01 * static_cast<double>(i)), 0.05 * static_cast<double>(i)));
    }
    std::vector<Complex> loops;
    for (int k = 0; k < 40; ++k) {
        loops.push_back(std::polar(std::pow(10.0, 0.5 * static_cast<double>(k)), 0.3 * static_cast<double>(k)));
    }
    for (const bool contours : {true, false}) {
        for (const bool guard : {true, false}) {
            const Tally tally = diagonal(problemAt(templates, p0, loops, contours, guard), "large");
            EXPECT_EQ(tally.differ, 0u) << contours << guard << " " << tally.first << " (largest gap " << tally.largest << " dB)";
        }
    }
}

TEST(SheetsOnTheCard, TheSpacingsOfACloudAreTheLocusSpacings)
{
    for (const char * name : {"toolbox-1.qft", "dcm-T33.qft", "acc90.qft"}) {
        Loaded loaded;
        ASSERT_TRUE(load(loaded, std::filesystem::path(QFTBX_EXAMPLES_DIR) / name));
        const CloudSet & clouds = loaded.project.templates();
        const Grid & g = loaded.grid;
        const BoundaryPlan plan = planBoundaries(loaded.omega, *loaded.project.specifications(), g.phaseRange,
                                                 g.phaseCount, g.magnitudeRange, g.magnitudeCount);
        const gpu::SheetProblem problem = gpu::sheetProblemOf(loaded.omega, *loaded.project.plant(), clouds, false,
                                                              true, plan, g.phaseRange, g.phaseCount,
                                                              g.magnitudeRange, g.magnitudeCount);
        for (std::size_t u = 0; u < clouds.size(); ++u) {
            const SingularLocus locus(nominalOverValueSet(problem.nominal[u], clouds[u]), false);
            const std::vector<double> & expected = locus.spacings();
            const gpu::SheetsOnProcessor card(problem, u);
            const std::vector<double> & got = card.spacings();
            ASSERT_EQ(got.size(), expected.size()) << name;
            for (std::size_t i = 0; i < expected.size(); ++i) {
                EXPECT_NEAR(got[i], expected[i], 1e-12 * std::max(1.0, expected[i])) << name << " " << u << " " << i;
            }
        }
    }
}

TEST(SheetsOnTheCard, TheSheetsOfTheWholeGridReadByTheEngineGiveItsBoundaries)
{
    for (const char * name : {"toolbox-1.qft", "dcm-T33.qft", "msf.qft", "unstable.qft"}) {
        Loaded loaded;
        ASSERT_TRUE(load(loaded, std::filesystem::path(QFTBX_EXAMPLES_DIR) / name));
        const Grid g{loaded.grid.phaseRange, 73, loaded.grid.magnitudeRange, 61};
        const CloudSet & contours = loaded.project.contour();
        BoundaryPlan plan = planBoundaries(loaded.omega, *loaded.project.specifications(), g.phaseRange,
                                           g.phaseCount, g.magnitudeRange, g.magnitudeCount);
        const gpu::SheetProblem problem = gpu::sheetProblemOf(loaded.omega, *loaded.project.plant(), contours, true,
                                                              true, plan, g.phaseRange, g.phaseCount,
                                                              g.magnitudeRange, g.magnitudeCount);
        std::vector<BoundarySheets> sheets(contours.size());
        for (std::size_t u = 0; u < contours.size(); ++u) {
            const gpu::SheetsOnProcessor card(problem, u);
            const std::array<bool, 5> used = plan.sheetsUsed(u);
            for (std::size_t k = 0; k < 5; ++k) {
                if (used[k]) {
                    sheets[u][k].assign(problem.rowMagnitudes.size(), std::vector<double>(problem.cosines.size()));
                }
            }
            for (std::size_t row = 0; row < problem.rowMagnitudes.size(); ++row) {
                for (std::size_t column = 0; column < problem.cosines.size(); ++column) {
                    const std::array<double, 5> values = card.cell(row, column);
                    for (std::size_t k = 0; k < 5; ++k) {
                        if (used[k]) {
                            sheets[u][k][row][column] = values[k];
                        }
                    }
                }
            }
        }

        std::vector<double> omega = loaded.omega;
        BoundaryEngine reference;
        reference.compute(&omega, loaded.project.plant(), contours, true, loaded.project.specifications(),
                          g.phaseRange, g.phaseCount, g.magnitudeRange, g.magnitudeCount, -1.0);
        BoundaryEngine adopted;
        adopted.adoptSheets(&omega, loaded.project.plant(), contours, true, std::move(plan), g.phaseRange,
                            g.phaseCount, g.magnitudeRange, g.magnitudeCount, std::move(sheets));
        const Gap gap = compareBoundaries(reference.boundaryData(), adopted.boundaryData());
        EXPECT_EQ(gap.differ, 0u) << name;
        EXPECT_EQ(gap.traces, 0.0) << name;
        EXPECT_LE(gap.columns, kColumnToleranceDb) << name;
    }
}

TEST(GpuBoundaries, TheSheetsOfTheCardAreTheProcessorsCellByCell)
{
    QFTBX_REQUIRE_CARD();

    std::size_t compared = 0;
    for (const char * name : {"toolbox-1.qft", "dcm-T33.qft", "acc90.qft", "maglev-upper.qft", "msf.qft"}) {
        for (const bool fromContour : {true, false}) {
            for (const bool guard : {true, false}) {
                Loaded loaded;
                ASSERT_TRUE(load(loaded, std::filesystem::path(QFTBX_EXAMPLES_DIR) / name)) << name;
                const Grid g{loaded.grid.phaseRange, 91, loaded.grid.magnitudeRange, 81};
                const CloudSet & templates = fromContour ? loaded.project.contour() : loaded.project.templates();
                const BoundaryPlan plan = planBoundaries(loaded.omega, *loaded.project.specifications(), g.phaseRange,
                                                         g.phaseCount, g.magnitudeRange, g.magnitudeCount);
                const gpu::SheetProblem problem = gpu::sheetProblemOf(loaded.omega, *loaded.project.plant(), templates,
                                                                      fromContour, guard, plan, g.phaseRange,
                                                                      g.phaseCount, g.magnitudeRange, g.magnitudeCount);
                const gpu::SheetsOnCard card = gpu::sheetsOnCard(problem, nullptr);
                ASSERT_FALSE(card.cancelled);
                ASSERT_EQ(card.sheets.size(), templates.size()) << name;

                Tally tally;
                for (std::size_t u = 0; u < templates.size(); ++u) {
                    const Complex p0 = problem.nominal[u];
                    const std::vector<Complex> q = nominalOverValueSet(p0, templates[u]);
                    std::optional<SingularLocus> locus;
                    if (guard) {
                        locus.emplace(q, fromContour);
                    }
                    const double scale = scaleOf(q);
                    const std::array<bool, 5> used = plan.sheetsUsed(u);
                    for (std::size_t row = 0; row < problem.rowMagnitudes.size(); ++row) {
                        for (std::size_t column = 0; column < problem.cosines.size(); ++column) {
                            const Complex L(problem.rowMagnitudes[row] * problem.cosines[column],
                                            problem.rowMagnitudes[row] * problem.sines[column]);
                            ProcessorCell expected = processorCell(p0, L, templates[u], q, locus ? &*locus : nullptr);
                            std::array<double, 5> got = expected.values;
                            for (std::size_t k = 0; k < 5; ++k) {
                                if (used[k]) {
                                    ASSERT_EQ(card.sheets[u][k].size(), problem.rowMagnitudes.size()) << name;
                                    got[k] = card.sheets[u][k][row][column];
                                }
                            }
                            compareCell(expected, got, scale, tally,
                                        std::string(name) + (fromContour ? " contour" : " cloud")
                                                + (guard ? " guarded" : "") + " frequency " + std::to_string(u)
                                                + " cell " + std::to_string(row) + "," + std::to_string(column));
                        }
                    }
                }
                EXPECT_EQ(tally.differ, 0u) << tally.first << " (largest gap " << tally.largest << " dB)";
                compared += tally.cells;
            }
        }
    }
    EXPECT_GT(compared, 100000u);
}

TEST(GpuBoundaries, TheEngineOfTheCardGivesTheProcessorsBoundaries)
{
    QFTBX_REQUIRE_CARD();

    for (const std::filesystem::path & file : examples()) {
        for (const bool fromContour : {true, false}) {
            Loaded processor;
            ASSERT_TRUE(load(processor, file)) << file;
            const Grid & g = processor.grid;
            ASSERT_TRUE(processor.project.computeBoundaries(g.phaseRange, g.phaseCount, g.magnitudeRange,
                                                            g.magnitudeCount, -1.0, fromContour, Processor::Cpu))
                    << file;

            Loaded card;
            ASSERT_TRUE(load(card, file)) << file;
            ASSERT_TRUE(card.project.computeBoundaries(g.phaseRange, g.phaseCount, g.magnitudeRange,
                                                       g.magnitudeCount, -1.0, fromContour, Processor::Gpu))
                    << file;

            const Gap gap = compareBoundaries(*processor.project.boundaries(), *card.project.boundaries());
            const std::string where = file.filename().string() + (fromContour ? " contour" : " cloud");
            EXPECT_EQ(gap.differ, 0u) << where;
            EXPECT_EQ(gap.traces, 0.0) << where;
            EXPECT_LE(gap.columns, kColumnToleranceDb) << where;
        }
    }
}
