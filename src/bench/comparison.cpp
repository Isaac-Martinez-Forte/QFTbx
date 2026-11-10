/**
 * @file
 * @brief The measures of how far apart the two processors' phases are.
 *
 * Every frequency the two have in common is compared, and one the other
 * lacks counts as differing whole. Contour points are compared as exact
 * values, sorted by real then imaginary part. An end of a column that is
 * infinite in one and finite in the other makes the column differ.
 */

#include "src/bench/comparison.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <iterator>
#include <map>

namespace qftbx::bench {

namespace {

double largest(double current, double value)
{
    return std::isnan(current) || value > current ? value : current;
}

bool lexicographic(const std::complex<double> & a, const std::complex<double> & b)
{
    return a.real() != b.real() ? a.real() < b.real() : a.imag() < b.imag();
}

std::size_t pointsInOneOnly(ComplexCloud a, ComplexCloud b)
{
    std::sort(a.begin(), a.end(), lexicographic);
    a.erase(std::unique(a.begin(), a.end()), a.end());
    std::sort(b.begin(), b.end(), lexicographic);
    b.erase(std::unique(b.begin(), b.end()), b.end());
    std::vector<std::complex<double>> differ;
    std::set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(differ), lexicographic);
    return differ.size();
}

double directedHausdorff(const ComplexCloud & from, const ComplexCloud & to)
{
    double worst = 0.0;
    for (const std::complex<double> & p : from) {
        double nearest = std::numeric_limits<double>::infinity();
        for (const std::complex<double> & q : to) {
            nearest = std::min(nearest, std::abs(p - q));
        }
        worst = std::max(worst, nearest);
    }
    return worst;
}

bool sameReport(const ContourReport & a, const ContourReport & b)
{
    return a.cloudPoints == b.cloudPoints && a.contourPoints == b.contourPoints && a.relaxed == b.relaxed
            && a.truncated == b.truncated && a.wholeCloud == b.wholeCloud && a.components == b.components
            && a.componentStarts == b.componentStarts && a.unbounded == b.unbounded
            && a.resonanceFrom == b.resonanceFrom && a.resonanceTo == b.resonanceTo;
}

void compareColumns(const BoundaryColumns & a, const BoundaryColumns & b, Comparison & into)
{
    if (a.columnCount() != b.columnCount()) {
        into.columnsDiffer += static_cast<std::size_t>(std::max(a.columnCount(), b.columnCount()));
        return;
    }
    for (std::int32_t column = 0; column < a.columnCount(); ++column) {
        const BoundaryColumns::Intervals x = a.intervals(column);
        const BoundaryColumns::Intervals y = b.intervals(column);
        if (x.count != y.count) {
            ++into.columnsDiffer;
            continue;
        }
        bool differs = false;
        for (std::int32_t i = 0; i < x.count; ++i) {
            for (const auto & [p, q] : {std::make_pair(x.lo[i], y.lo[i]), std::make_pair(x.hi[i], y.hi[i])}) {
                if (std::isfinite(p) && std::isfinite(q)) {
                    into.columnLargestDb = largest(into.columnLargestDb, std::abs(p - q));
                } else if (p != q) {
                    differs = true;
                }
            }
        }
        if (differs) {
            ++into.columnsDiffer;
        }
    }
}

void compareBoundaries(const BoundaryData & a, const BoundaryData & b, Comparison & into)
{
    into.columnLargestDb = 0.0;
    const ColumnSet & columnsA = a.specificationColumns();
    const ColumnSet & columnsB = b.specificationColumns();
    for (std::size_t f = 0; f < std::max(columnsA.size(), columnsB.size()); ++f) {
        std::map<std::string, const BoundaryColumns *> left;
        std::map<std::string, const BoundaryColumns *> right;
        if (f < columnsA.size()) {
            for (const auto & [name, columns] : columnsA[f]) {
                left[name] = &columns;
            }
        }
        if (f < columnsB.size()) {
            for (const auto & [name, columns] : columnsB[f]) {
                right[name] = &columns;
            }
        }
        for (const auto & [name, columns] : left) {
            const auto other = right.find(name);
            if (other == right.end()) {
                into.columnsDiffer += static_cast<std::size_t>(columns->columnCount());
            } else {
                compareColumns(*columns, *other->second, into);
            }
        }
        for (const auto & [name, columns] : right) {
            if (left.find(name) == left.end()) {
                into.columnsDiffer += static_cast<std::size_t>(columns->columnCount());
            }
        }
    }

    const BoundarySet & tracesA = a.boundaries();
    const BoundarySet & tracesB = b.boundaries();
    for (std::size_t f = 0; f < std::max(tracesA.size(), tracesB.size()); ++f) {
        if (f >= tracesA.size() || f >= tracesB.size()) {
            into.traceSetsDiffer += std::max(f < tracesA.size() ? tracesA[f].size() : 0,
                                             f < tracesB.size() ? tracesB[f].size() : 0);
            continue;
        }
        for (const auto & [name, traces] : tracesA[f]) {
            const auto other = tracesB[f].find(name);
            if (other == tracesB[f].end() || other->second != traces) {
                ++into.traceSetsDiffer;
            }
        }
        for (const auto & [name, traces] : tracesB[f]) {
            if (tracesA[f].find(name) == tracesA[f].end()) {
                ++into.traceSetsDiffer;
            }
        }
    }

    const UnionTraces & unionA = a.unionBoundaries();
    const UnionTraces & unionB = b.unionBoundaries();
    for (std::size_t f = 0; f < std::max(unionA.size(), unionB.size()); ++f) {
        if (f >= unionA.size() || f >= unionB.size() || unionA[f] != unionB[f]) {
            ++into.unionsDiffer;
        }
    }
}

}

void comparePhases(const PhaseOutputs & cpu, const PhaseOutputs & gpu, Comparison & into)
{
    into.made = true;
    into.frequencies = std::max(cpu.clouds.size(), gpu.clouds.size());

    into.cloudLargestRelative = 0.0;
    for (std::size_t f = 0; f < into.frequencies; ++f) {
        if (f >= cpu.clouds.size() || f >= gpu.clouds.size() || cpu.clouds[f].size() != gpu.clouds[f].size()) {
            ++into.cloudSizesDiffer;
            continue;
        }
        for (std::size_t i = 0; i < cpu.clouds[f].size(); ++i) {
            const std::complex<double> a = cpu.clouds[f][i];
            const std::complex<double> b = gpu.clouds[f][i];
            const double scale = std::abs(a);
            const double gap = std::abs(a - b);
            into.cloudLargestRelative = largest(into.cloudLargestRelative,
                                                scale > 0.0 ? gap / scale : gap);
        }
    }

    into.contourLargestHausdorff = 0.0;
    for (std::size_t f = 0; f < std::max(cpu.contours.size(), gpu.contours.size()); ++f) {
        if (f >= cpu.contours.size() || f >= gpu.contours.size()) {
            into.contourPointsDiffer += f < cpu.contours.size() ? cpu.contours[f].size() : gpu.contours[f].size();
            continue;
        }
        into.contourPointsDiffer += pointsInOneOnly(cpu.contours[f], gpu.contours[f]);
        const double epsilon = f < cpu.epsilon.size() && cpu.epsilon[f] > 0.0 ? cpu.epsilon[f] : 1.0;
        const double hausdorff = std::max(directedHausdorff(cpu.contours[f], gpu.contours[f]),
                                          directedHausdorff(gpu.contours[f], cpu.contours[f]));
        into.contourLargestHausdorff = largest(into.contourLargestHausdorff, hausdorff / epsilon);
    }

    for (std::size_t f = 0; f < std::max(cpu.reports.size(), gpu.reports.size()); ++f) {
        if (f >= cpu.reports.size() || f >= gpu.reports.size() || !sameReport(cpu.reports[f], gpu.reports[f])) {
            ++into.reportsDiffer;
        }
    }
    for (std::size_t f = 0; f < std::max(cpu.epsilon.size(), gpu.epsilon.size()); ++f) {
        if (f >= cpu.epsilon.size() || f >= gpu.epsilon.size() || cpu.epsilon[f] != gpu.epsilon[f]) {
            ++into.epsilonsDiffer;
        }
    }

    if (cpu.boundaries && gpu.boundaries) {
        compareBoundaries(*cpu.boundaries, *gpu.boundaries, into);
    }
}

}
