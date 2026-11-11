/**
 * @file
 * @brief What the suites of the GPU's boundaries share: the examples, loaded
 * with their templates computed on the processor and their Nichols grid,
 * the sheets the threads' own code gives for a whole grid, and how two
 * computations of the boundaries are told apart: the traces and the unions
 * point by point, the column ends apart, in dB.
 */

#ifndef QFTBX_TESTS_GPU_BOUNDARY_CHECKS_H
#define QFTBX_TESTS_GPU_BOUNDARY_CHECKS_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/boundaries/boundary_plan.h"
#include "src/core/gpu/boundaries/sheet_sweep.h"
#include "src/core/project/settings.h"

namespace qftbx::gputest {

constexpr double kColumnToleranceDb = 1e-6;

struct Grid
{
    Range phaseRange;
    std::int32_t phaseCount = 0;
    Range magnitudeRange;
    std::int32_t magnitudeCount = 0;
};

inline Grid gridOf(ProjectController & project)
{
    const Settings defaults;
    if (const BoundaryData * stored = project.boundaries()) {
        return {stored->phaseRange(), stored->phaseCount(), stored->magnitudeRange(), stored->magnitudeCount()};
    }
    return {Range(defaults.defaults.phaseStart, defaults.defaults.phaseEnd), defaults.defaults.phasePoints,
            Range(defaults.defaults.magnitudeStart, defaults.defaults.magnitudeEnd), defaults.defaults.magnitudePoints};
}

inline std::vector<std::filesystem::path> examples()
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

struct Loaded
{
    ProjectController project;
    Grid grid;
    std::vector<double> omega;
};

inline bool load(Loaded & loaded, const std::filesystem::path & file)
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

struct Gap
{
    std::size_t differ = 0;
    double traces = 0.0;
    double columns = 0.0;
};

inline void compareValue(double a, double b, Gap & gap, double & largest)
{
    if (std::isinf(a) || std::isinf(b)) {
        gap.differ += a == b ? 0 : 1;
        return;
    }
    largest = std::max(largest, std::abs(a - b));
}

inline void compareTraces(const TraceSet & a, const TraceSet & b, Gap & gap)
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

inline Gap compareBoundaries(const BoundaryData & a, const BoundaryData & b)
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

inline std::vector<BoundarySheets> sheetsOnProcessor(const gpu::SheetProblem & problem, const BoundaryPlan & plan)
{
    std::vector<BoundarySheets> sheets(problem.templates->size());
    for (std::size_t u = 0; u < sheets.size(); ++u) {
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
    return sheets;
}

}

#endif
