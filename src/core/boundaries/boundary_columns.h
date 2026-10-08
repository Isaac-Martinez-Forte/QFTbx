/**
 * @file
 * @brief The boundaries of one design frequency as the search reads them.
 *
 * Per phase column of the Nichols grid, the magnitude intervals every
 * specification allows the nominal loop, and their intersection over the
 * specifications; the search intersects them over frequencies and takes
 * the smallest gain left. A specification's sheet holds its worst-case
 * closed-loop magnitude D in dB at every grid node; a node is allowed
 * exactly when D is strictly under the bound, as the contour tracer takes
 * it, and a NaN violates. The intervals of a column are its runs of allowed
 * nodes, each end interpolated linearly where D crosses the bound, and the
 * state of the top and bottom nodes extends beyond the grid. Intervals are
 * ascending and disjoint, ends may be infinite, and a finite end is a
 * crossing of the binding boundary: their minimum and maximum over a phase
 * span are the B_min and B_max of the cutting equations (Tharewal 2005,
 * fig. 5.1). They lie in flat arrays, so a query allocates nothing.
 * intersectWith needs both operands over the same grid.
 *
 * columnOf is the column whose node is nearest the phase, the binning of
 * the traces; the search must not judge a point by it, as it is permissive
 * where the boundary is steep in phase. The columns from
 * firstColumnCovering of a phase interval's lower end to lastColumnCovering
 * of its upper end bracket it, and a verdict they all agree on holds
 * wherever the boundary lies between the nodes. Phases outside the window
 * fall in the end columns.
 *
 * fromTraces rebuilds the columns, a cell permissive at every crossing,
 * from traced curves for a file that holds only those: border cells one
 * step apart form a run, the state above the topmost run is the curve's
 * label and each run flips it going down, except the single run of a
 * closed curve in a column, a forbidden band. By deriveLabels a closed
 * curve forbids its inside, and an open one allows the side above for
 * tracking, output and input disturbance and the side below for stability,
 * sensor noise and control effort. ColumnSet keys the columns of each
 * design frequency by specification name.
 */

#ifndef QFTBX_BOUNDARY_COLUMNS_H
#define QFTBX_BOUNDARY_COLUMNS_H

#include <cstdint>
#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include "src/core/boundaries/boundary_types.h"
#include "src/core/math/range.h"

namespace qftbx {

class BoundaryColumns
{
public:
    struct Span {
        double lo;
        double hi;
    };

    BoundaryColumns() = default;

    BoundaryColumns(std::int32_t phaseCount, Range phaseRange);

    BoundaryColumns(std::vector<std::vector<Span>> columns, std::int32_t phaseCount, Range phaseRange);

    template <class Cell>
    static BoundaryColumns fromSheet(Cell cell, double thresholdDb,
                                     std::int32_t phaseCount, Range phaseRange,
                                     std::int32_t magnitudeCount, Range magnitudeRange);

    static BoundaryColumns fromTraces(const std::string & specification, const TraceSet & traces,
                                      std::int32_t phaseCount, Range phaseRange,
                                      std::int32_t magnitudeCount, Range magnitudeRange);

    void intersectWith(const BoundaryColumns & other);

    std::int32_t columnCount() const noexcept { return m_columns; }

    std::int32_t columnOf(double phaseDegrees) const noexcept
    {
        const double x = (phaseDegrees - m_phaseMin) * m_inverseStep + 0.5;
        if (x <= 0.0) {
            return 0;
        }
        if (x >= static_cast<double>(m_columns)) {
            return m_columns - 1;
        }
        return static_cast<std::int32_t>(x);
    }

    std::int32_t firstColumnCovering(double phaseDegrees) const noexcept
    {
        const double x = std::floor((phaseDegrees - m_phaseMin) * m_inverseStep);
        if (x <= 0.0) {
            return 0;
        }
        if (x >= static_cast<double>(m_columns - 1)) {
            return m_columns - 1;
        }
        return static_cast<std::int32_t>(x);
    }

    std::int32_t lastColumnCovering(double phaseDegrees) const noexcept
    {
        const double x = std::ceil((phaseDegrees - m_phaseMin) * m_inverseStep);
        if (x <= 0.0) {
            return 0;
        }
        if (x >= static_cast<double>(m_columns - 1)) {
            return m_columns - 1;
        }
        return static_cast<std::int32_t>(x);
    }

    double phaseOf(std::int32_t column) const noexcept { return m_phaseMin + column * m_step; }

    struct Intervals {
        const double * lo;
        const double * hi;
        std::int32_t count;
    };

    Intervals intervals(std::int32_t column) const noexcept
    {
        const std::int32_t begin = m_begin[static_cast<std::size_t>(column)];
        const std::int32_t end = m_begin[static_cast<std::size_t>(column) + 1];
        return {m_lo.data() + begin, m_hi.data() + begin, end - begin};
    }

    std::vector<Span> spans(std::int32_t column) const;

    bool allows(std::int32_t column, double magnitudeDb) const noexcept
    {
        const Intervals spans = intervals(column);
        for (std::int32_t i = 0; i < spans.count; ++i) {
            if (magnitudeDb < spans.lo[i]) {
                return false;
            }
            if (magnitudeDb <= spans.hi[i]) {
                return true;
            }
        }
        return false;
    }

    bool operator==(const BoundaryColumns & other) const;
    bool operator!=(const BoundaryColumns & other) const { return !(*this == other); }

    static TraceLabels deriveLabels(const std::string & specification, const TraceSet & traces,
                                    Range phaseRange, std::int32_t phaseCount);

private:
    void setGrid(std::int32_t phaseCount, Range phaseRange);
    void flatten(const std::vector<std::vector<Span>> & columns);

    std::vector<std::int32_t> m_begin;
    std::vector<double> m_lo;
    std::vector<double> m_hi;
    double m_phaseMin = 0.0;
    double m_step = 1.0;
    double m_inverseStep = 1.0;
    std::int32_t m_columns = 0;
};

using ColumnSet = std::vector<std::map<std::string, BoundaryColumns>>;

template <class Cell>
BoundaryColumns BoundaryColumns::fromSheet(Cell cell, double thresholdDb,
                                           std::int32_t phaseCount, Range phaseRange,
                                           std::int32_t magnitudeCount, Range magnitudeRange)
{
    constexpr double kInfinity = std::numeric_limits<double>::infinity();

    const std::int32_t rows = magnitudeCount;
    const double magnitudeStep = rows > 1 ? magnitudeRange.width() / (rows - 1) : 0.0;
    const auto magnitudeAt = [&](std::int32_t row) { return magnitudeRange.min + row * magnitudeStep; };

    const auto crossing = [&](std::int32_t allowedRow, double allowedD, std::int32_t violatingRow, double violatingD) {
        const double ma = magnitudeAt(allowedRow), mv = magnitudeAt(violatingRow);
        if (!(allowedD > -kInfinity)) {
            return mv;
        }
        if (!(violatingD < kInfinity)) {
            return ma;
        }
        return ma + (mv - ma) * (thresholdDb - allowedD) / (violatingD - allowedD);
    };

    std::vector<std::vector<Span>> columns(static_cast<std::size_t>(phaseCount > 0 ? phaseCount : 1));

    for (std::int32_t c = 0; c < phaseCount; ++c) {
        std::vector<Span> & spans = columns[static_cast<std::size_t>(c)];
        bool inside = false;
        double lo = -kInfinity;
        double previous = 0.0;

        for (std::int32_t r = 0; r < rows; ++r) {
            const double d = static_cast<double>(cell(c, r));
            const bool allowed = d < thresholdDb;
            if (allowed && !inside) {
                lo = r == 0 ? -kInfinity : crossing(r, d, r - 1, previous);
                inside = true;
            } else if (!allowed && inside) {
                spans.push_back({lo, crossing(r - 1, previous, r, d)});
                inside = false;
            }
            previous = d;
        }
        if (inside) {
            spans.push_back({lo, kInfinity});
        }
    }

    BoundaryColumns result;
    result.setGrid(phaseCount, phaseRange);
    result.flatten(columns);
    return result;
}

}

#endif
