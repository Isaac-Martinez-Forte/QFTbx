#include "src/core/boundaries/boundary_columns.h"

#include <algorithm>
#include <cmath>

namespace qftbx {

namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();

using Span = BoundaryColumns::Span;

//A border cell of one traced curve in one column.
struct Cell {
    double magnitude;
    std::size_t trace;
};

//The intersection of two ascending lists of disjoint intervals.
std::vector<Span> intersect(const std::vector<Span> & a, const std::vector<Span> & b)
{
    std::vector<Span> out;
    std::size_t i = 0, j = 0;

    while (i < a.size() && j < b.size()) {
        const double lo = std::max(a[i].lo, b[j].lo);
        const double hi = std::min(a[i].hi, b[j].hi);
        if (lo <= hi) {
            out.push_back({lo, hi});
        }
        if (a[i].hi < b[j].hi) {
            ++i;
        } else {
            ++j;
        }
    }

    return out;
}

//The allowed intervals of one specification in one column, from the border
//cells of its curves (see BoundaryColumns::fromTraces).
std::vector<Span> spansFromCells(std::vector<Cell> & cells, double magnitudeStep,
                                 const TraceLabels & labels, const std::vector<bool> & closed)
{
    if (cells.empty()) {
        return {{-kInfinity, kInfinity}};
    }

    std::sort(cells.begin(), cells.end(),
              [](const Cell & a, const Cell & b) { return a.magnitude < b.magnitude; });

    //Runs, bottom to top: [first, last], the curve of the run's top cell,
    //and whether the run is a fragment of a closed curve a cell thick.
    struct Run {
        double bottom;
        double top;
        std::size_t trace;
        bool band;
    };
    std::vector<Run> runs;
    const double gap = magnitudeStep * 1.01;

    for (std::size_t i = 0; i < cells.size();) {
        std::size_t j = i;
        while (j + 1 < cells.size() && cells[j + 1].magnitude - cells[j].magnitude <= gap) {
            ++j;
        }
        runs.push_back({cells[i].magnitude, cells[j].magnitude, cells[j].trace, false});
        i = j + 1;
    }

    //A closed curve with a single run in the column enters and leaves the
    //column in that run: a band, not a crossing.
    std::vector<int> runsOfTrace(labels.size(), 0);
    for (const Run & run : runs) {
        ++runsOfTrace[run.trace];
    }
    for (Run & run : runs) {
        run.band = closed[run.trace] && runsOfTrace[run.trace] == 1;
    }

    //Walk down from the top. The state above the topmost curve is the
    //opposite of its label; a crossing sits at the end of its run on the
    //allowed side, so the run's own cells are forbidden; a band leaves the
    //state as it was.
    std::vector<Span> spans;
    bool allowedAbove = !labels[runs.back().trace];
    double ceiling = kInfinity;

    for (std::size_t r = runs.size(); r-- > 0;) {
        const Run & run = runs[r];
        if (allowedAbove) {
            spans.push_back({run.top, ceiling});
        }
        ceiling = run.bottom;
        if (!run.band) {
            allowedAbove = !allowedAbove;
        }
    }

    if (allowedAbove) {
        spans.push_back({-kInfinity, ceiling});
    }

    std::reverse(spans.begin(), spans.end());
    return spans;
}

bool reachesBothEnds(const Trace & trace, Range phaseRange, double step)
{
    double lowest = kInfinity, highest = -kInfinity;
    for (const NicholsPoint & point : trace) {
        lowest = std::min(lowest, point.phase);
        highest = std::max(highest, point.phase);
    }
    return lowest <= phaseRange.min + step && highest >= phaseRange.max - step;
}

bool allowsBelowWhenOpen(const std::string & specification)
{
    return specification == "Stability" || specification == "SensorNoise" ||
           specification == "ControlEffort";
}

} // namespace

BoundaryColumns::BoundaryColumns(std::int32_t phaseCount, Range phaseRange)
{
    setGrid(phaseCount, phaseRange);
    flatten(std::vector<std::vector<Span>>(static_cast<std::size_t>(m_columns), {{-kInfinity, kInfinity}}));
}

BoundaryColumns::BoundaryColumns(std::vector<std::vector<Span>> columns, std::int32_t phaseCount, Range phaseRange)
{
    setGrid(phaseCount, phaseRange);
    columns.resize(static_cast<std::size_t>(m_columns), {{-kInfinity, kInfinity}});
    flatten(columns);
}

void BoundaryColumns::setGrid(std::int32_t phaseCount, Range phaseRange)
{
    m_columns = std::max<std::int32_t>(phaseCount, 1);
    m_phaseMin = phaseRange.min;
    m_step = m_columns > 1 ? phaseRange.width() / (m_columns - 1) : 1.0;
    if (!(m_step > 0.0)) {
        m_step = 1.0;
    }
    m_inverseStep = 1.0 / m_step;
}

void BoundaryColumns::flatten(const std::vector<std::vector<Span>> & columns)
{
    m_begin.assign(static_cast<std::size_t>(m_columns) + 1, 0);
    m_lo.clear();
    m_hi.clear();

    std::size_t total = 0;
    for (const auto & column : columns) {
        total += column.size();
    }
    m_lo.reserve(total);
    m_hi.reserve(total);

    for (std::size_t c = 0; c < static_cast<std::size_t>(m_columns); ++c) {
        m_begin[c] = static_cast<std::int32_t>(m_lo.size());
        if (c < columns.size()) {
            for (const Span & span : columns[c]) {
                m_lo.push_back(span.lo);
                m_hi.push_back(span.hi);
            }
        }
    }
    m_begin[static_cast<std::size_t>(m_columns)] = static_cast<std::int32_t>(m_lo.size());
}

std::vector<BoundaryColumns::Span> BoundaryColumns::spans(std::int32_t column) const
{
    const Intervals view = intervals(column);
    std::vector<Span> out;
    out.reserve(static_cast<std::size_t>(view.count));
    for (std::int32_t i = 0; i < view.count; ++i) {
        out.push_back({view.lo[i], view.hi[i]});
    }
    return out;
}

bool BoundaryColumns::operator==(const BoundaryColumns & other) const
{
    return m_columns == other.m_columns && m_phaseMin == other.m_phaseMin && m_step == other.m_step &&
           m_begin == other.m_begin && m_lo == other.m_lo && m_hi == other.m_hi;
}

void BoundaryColumns::intersectWith(const BoundaryColumns & other)
{
    std::vector<std::vector<Span>> columns(static_cast<std::size_t>(m_columns));
    for (std::int32_t c = 0; c < m_columns; ++c) {
        columns[static_cast<std::size_t>(c)] = intersect(spans(c), other.spans(std::min(c, other.m_columns - 1)));
    }
    flatten(columns);
}

TraceLabels BoundaryColumns::deriveLabels(const std::string & specification, const TraceSet & traces,
                                          Range phaseRange, std::int32_t phaseCount)
{
    const double step = phaseCount > 1 ? phaseRange.width() / (phaseCount - 1) : phaseRange.width();
    TraceLabels labels;
    labels.reserve(traces.size());

    for (const Trace & trace : traces) {
        const bool open = reachesBothEnds(trace, phaseRange, step);
        labels.push_back(open && allowsBelowWhenOpen(specification));
    }

    return labels;
}

BoundaryColumns BoundaryColumns::fromTraces(const std::string & specification, const TraceSet & traces,
                                            std::int32_t phaseCount, Range phaseRange,
                                            std::int32_t magnitudeCount, Range magnitudeRange)
{
    BoundaryColumns result;
    result.setGrid(phaseCount, phaseRange);

    const double magnitudeStep = magnitudeCount > 1 ? magnitudeRange.width() / (magnitudeCount - 1) : 0.0;
    const TraceLabels labels = deriveLabels(specification, traces, phaseRange, phaseCount);

    std::vector<bool> closed(traces.size(), false);
    for (std::size_t t = 0; t < traces.size(); ++t) {
        closed[t] = !reachesBothEnds(traces[t], phaseRange, result.m_step);
    }

    std::vector<std::vector<Cell>> cells(static_cast<std::size_t>(result.m_columns));
    for (std::size_t t = 0; t < traces.size(); ++t) {
        for (const NicholsPoint & point : traces[t]) {
            if (!std::isfinite(point.magnitude) || !std::isfinite(point.phase)) {
                continue;
            }
            cells[static_cast<std::size_t>(result.columnOf(point.phase))].push_back({point.magnitude, t});
        }
    }

    std::vector<std::vector<Span>> columns(static_cast<std::size_t>(result.m_columns));
    for (std::size_t c = 0; c < cells.size(); ++c) {
        columns[c] = spansFromCells(cells[c], magnitudeStep, labels, closed);
    }

    result.flatten(columns);
    return result;
}

} // namespace qftbx
