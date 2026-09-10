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

/**
 * @brief The boundaries of one design frequency as the search reads them:
 * per phase column of the Nichols grid, the intervals of magnitude the
 * nominal loop is allowed to take.
 *
 * Every specification bounds a region of the Nichols plane, and the search
 * only ever asks whether a point, or a rectangle, is on the allowed side of
 * all of them. The classical way to answer is the union of the boundary
 * curves and a parity count over the curve points of one phase bucket, and
 * it breaks down where an open boundary runs under a closed one in the same
 * column (the union drops the lower curve and the count calls the inside of
 * the closed curve allowed), and again where the curve is traced as thin
 * fragments a single cell thick (each fragment counts as one crossing and
 * the parity of the fragments decides the verdict). The columns answer the
 * question from the sheet the curves were cut from instead.
 *
 * A specification's sheet holds its worst-case closed-loop magnitude D at
 * every grid node, and a node is allowed exactly when D is under the bound
 * (the contour tracer walks the border of the nodes where it is not). The
 * allowed intervals of a column are the runs of allowed nodes, each end
 * placed where D crosses the bound by linear interpolation between the last
 * allowed node and the first violating one, so the column is a whole cell
 * sharper than the traced curve, which sits on the violating node. The state
 * of the top and bottom nodes extends beyond the grid. The column of the
 * frequency is the intersection over its specifications. Open and closed
 * curves, multivalued boundaries, pockets, corridors and thin fragments all
 * come out of the sheet with no rule to apply.
 *
 * The intervals of every column lie in two flat arrays indexed by a table of
 * column starts, so a query touches a handful of contiguous doubles and
 * allocates nothing. A column's cell is centred on its grid node, and phases
 * outside the window fall in the end columns.
 *
 * A finite end of an interval is a crossing of the binding boundary at that
 * column: their minimum and maximum over a phase span are the B_min and
 * B_max the cutting equations of the algorithms read (Tharewal 2005,
 * fig. 5.1).
 *
 * Projects written before the columns were stored only carry the traced
 * curves; fromTraces() rebuilds the columns from them as well as the curves
 * allow (see there).
 */
class BoundaryColumns
{
public:
    /// An allowed interval, [lo, hi]; lo may be -infinity and hi +infinity.
    struct Span {
        double lo;
        double hi;
    };

    BoundaryColumns() = default;

    /// Everything allowed in every column of the grid.
    BoundaryColumns(std::int32_t phaseCount, Range phaseRange);

    /// The given intervals, one ascending disjoint list per column of the
    /// grid (the reader's path).
    BoundaryColumns(std::vector<std::vector<Span>> columns, std::int32_t phaseCount, Range phaseRange);

    /**
     * @brief The allowed intervals of one specification, read off its
     * sheet.
     *
     * @param cell the sheet: cell(phaseIndex, magnitudeIndex) is D in dB at
     * that grid node; a value that is not a number violates.
     * @param thresholdDb the bound the sheet is cut at; a node is allowed
     * when its D is strictly under it, as the tracer takes it.
     * @param phaseCount, phaseRange, magnitudeCount, magnitudeRange the
     * grid the sheet was sampled on.
     */
    template <class Cell>
    static BoundaryColumns fromSheet(Cell cell, double thresholdDb,
                                     std::int32_t phaseCount, Range phaseRange,
                                     std::int32_t magnitudeCount, Range magnitudeRange);

    /**
     * @brief The allowed intervals of one specification rebuilt from its
     * traced curves, for a project whose file predates the columns.
     *
     * The border cells of the curves in a column are sorted by magnitude
     * and consecutive cells one grid step apart form a run. Above the
     * topmost run the plane is allowed or not according to the label of
     * that curve (deriveLabels: from the specification family and whether
     * the curve is open or closed), and every run flips the state going
     * down, with the crossing at the end of the run on its allowed side so
     * the cells themselves stay forbidden. The one exception is a closed
     * curve that has a single run in the column, a fragment a cell thick:
     * it is a forbidden band and the state is the same on both sides. A
     * sheet says all this directly; the curves are an approximation of it,
     * a cell permissive at every crossing.
     */
    static BoundaryColumns fromTraces(const std::string & specification, const TraceSet & traces,
                                      std::int32_t phaseCount, Range phaseRange,
                                      std::int32_t magnitudeCount, Range magnitudeRange);

    /// Narrows every column to what this and the other allow. Both must be
    /// over the same grid.
    void intersectWith(const BoundaryColumns & other);

    std::int32_t columnCount() const noexcept { return m_columns; }

    /// The column whose node is NEAREST the phase; the end columns take
    /// what falls outside the window. This is the reading the traces are
    /// binned with (fromTraces). It is not the reading the search may use:
    /// a point half a cell from a node is judged by a column up to half a
    /// step away, and where the boundary is steep in phase that is a
    /// magnitude error of the first order in the step - permissive when the
    /// boundary rises towards the point. See firstColumnCovering().
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

    /// The columns whose nodes bracket the phase: the last node at or below
    /// it and the first node at or above it, the same column when the phase
    /// sits on a node. A phase interval is covered by the columns from
    /// firstColumnCovering(its lower end) to lastColumnCovering(its upper
    /// end), and a verdict that every one of those columns agrees on holds
    /// wherever the boundary between the nodes lies (the columns bound it
    /// from both sides), which the nearest-node reading cannot claim. The
    /// end columns take what falls outside the window.
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

    /// The phase of a column's grid node.
    double phaseOf(std::int32_t column) const noexcept { return m_phaseMin + column * m_step; }

    /// The allowed intervals of a column as the search reads them:
    /// ascending and disjoint, lo may be -infinity and hi +infinity.
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

    /// The allowed intervals of a column, copied (the writer's path).
    std::vector<Span> spans(std::int32_t column) const;

    /// Whether a magnitude is allowed in a column.
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

    /**
     * @brief The allowed-side label of a specification's traced curves,
     * for fromTraces().
     *
     * A closed curve (one that does not reach both ends of the phase window)
     * forbids its inside, so the plane above it is allowed. An open curve
     * follows its family: the magnitude specifications on the closed loop
     * whose bound grows with the loop gain (tracking, output and input
     * disturbance) allow the side above; those that shrink with it
     * (stability, sensor noise, control effort) allow the side below.
     */
    static TraceLabels deriveLabels(const std::string & specification, const TraceSet & traces,
                                    Range phaseRange, std::int32_t phaseCount);

private:
    void setGrid(std::int32_t phaseCount, Range phaseRange);
    void flatten(const std::vector<std::vector<Span>> & columns);

    std::vector<std::int32_t> m_begin;   //column starts into m_lo/m_hi, columnCount + 1 entries
    std::vector<double> m_lo;
    std::vector<double> m_hi;
    double m_phaseMin = 0.0;
    double m_step = 1.0;
    double m_inverseStep = 1.0;
    std::int32_t m_columns = 0;
};

/// Per design frequency, the columns of each specification, keyed by name.
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

    //Where D crosses the bound between an allowed node and a violating one,
    //by linear interpolation. An infinite violating value (the critical
    //point) puts the crossing on the allowed node; an allowed value that is
    //not finite puts it on the violating one.
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
            const bool allowed = d < thresholdDb;   //false for a NaN
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

} // namespace qftbx

#endif // QFTBX_BOUNDARY_COLUMNS_H
