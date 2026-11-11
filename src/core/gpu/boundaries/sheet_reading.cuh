/**
 * @file
 * @brief Reading a sheet at a specification's height, as a thread does it,
 * on the card or on the processor.
 *
 * A column is read from the lowest magnitude up, as
 * BoundaryColumns::fromSheet reads it: a cell is allowed when its value is
 * under the height, and each run of allowed cells is an interval whose ends
 * are interpolated where the sheet crosses the height.
 *
 * The traces are ContourTracer's. Which cells are at or above the height,
 * which of them could start a trace and which a trace has passed are bits,
 * one per cell column after column, the order in which the processor scans
 * them. A cell starts a trace only when it is inside the frame and one of
 * its inside neighbours is under the height: from any other the processor
 * gets a trace of one point and drops it. The scan goes on from the cell
 * after the last start (candidates), as the processor's loop does, since a
 * start that gives one point is left unvisited. A step of the walk tries the
 * eight neighbours in the processor's order (stepTo), and a trace is the
 * nodes it passed, kept when it has more than one.
 *
 * A trace's label says whether the side below it is allowed, judged as
 * BoundaryEngine judges it: one decibel under the highest point of the
 * trace, the first of them along it counting the extra point the tracer
 * puts before the first, against the specification's magnitude over the
 * whole template, with no guard.
 */

#ifndef QFTBX_GPU_SHEET_READING_CUH
#define QFTBX_GPU_SHEET_READING_CUH

#include <cmath>
#include <cstdint>

#include "src/core/gpu/boundaries/sheet_bounds.cuh"
#include "src/core/gpu/common/host_device.h"
#include "src/core/math/constants.h"

namespace qftbx::gpu {

struct ColumnScale
{
    double magnitudeMin;
    double magnitudeStep;
};

struct NodeScale
{
    double phaseSpan;
    double phaseBottom;
    double phaseCells;
    double magnitudeSpan;
    double magnitudeBottom;
    double magnitudeCells;
};

struct SheetBits
{
    const std::uint32_t * high;
    const std::uint32_t * border;
    std::uint32_t * visited;
    std::int32_t columns;
    std::int32_t rows;
};

struct TraceOutput
{
    std::int32_t * x;
    std::int32_t * y;
    std::int32_t * start;
    std::int32_t capacity;
    std::int32_t points;
    std::int32_t traces;
    bool overflow;
};

QFTBX_HD inline double crossingAt(const ColumnScale & scale, double height, std::int32_t allowedRow, double allowedD,
                                  std::int32_t violatingRow, double violatingD)
{
    const double ma = scale.magnitudeMin + allowedRow * scale.magnitudeStep;
    const double mv = scale.magnitudeMin + violatingRow * scale.magnitudeStep;
    if (!(allowedD > -kInfinity)) {
        return mv;
    }
    if (!(violatingD < kInfinity)) {
        return ma;
    }
    return ma + (mv - ma) * (height - allowedD) / (violatingD - allowedD);
}

template <bool Fill>
QFTBX_HD inline std::int32_t readColumn(const double * sheet, std::int32_t columns, std::int32_t rows,
                                        std::int32_t column, double height, const ColumnScale & scale, double * lo,
                                        double * hi)
{
    std::int32_t count = 0;
    bool inside = false;
    [[maybe_unused]] double start = -kInfinity;
    double previous = 0.0;
    for (std::int32_t r = 0; r < rows; ++r) {
        const double d = sheet[static_cast<std::int64_t>(r) * columns + column];
        const bool allowed = d < height;
        if (allowed && !inside) {
            start = r == 0 ? -kInfinity : crossingAt(scale, height, r, d, r - 1, previous);
            inside = true;
        } else if (!allowed && inside) {
            if constexpr (Fill) {
                lo[count] = start;
                hi[count] = crossingAt(scale, height, r - 1, previous, r, d);
            }
            ++count;
            inside = false;
        }
        previous = d;
    }
    if (inside) {
        if constexpr (Fill) {
            lo[count] = start;
            hi[count] = kInfinity;
        }
        ++count;
    }
    return count;
}

QFTBX_HD inline std::int64_t cellIndex(std::int32_t x, std::int32_t y, std::int32_t rows)
{
    return static_cast<std::int64_t>(x) * rows + y;
}

QFTBX_HD inline bool bitAt(const std::uint32_t * words, std::int64_t index)
{
    return ((words[index >> 5] >> (index & 31)) & 1u) != 0;
}

QFTBX_HD inline std::int32_t neighbourX(std::int32_t d)
{
    return (d >= 1 && d <= 3) ? 1 : (d >= 5 ? -1 : 0);
}

QFTBX_HD inline std::int32_t neighbourY(std::int32_t d)
{
    return (d <= 1 || d == 7) ? -1 : ((d >= 3 && d <= 5) ? 1 : 0);
}

QFTBX_HD inline bool insideFrame(std::int32_t x, std::int32_t y, std::int32_t columns, std::int32_t rows)
{
    return x > 0 && x < columns - 1 && y > 0 && y < rows - 1;
}

QFTBX_HD inline bool stepTo(const SheetBits & bits, std::int32_t x, std::int32_t y, std::int32_t i, std::int32_t & nextX,
                            std::int32_t & nextY)
{
    const std::int32_t outward = i % 8;
    const std::int32_t along = (i - 1) % 8;
    const std::int32_t ox = x + neighbourX(outward);
    const std::int32_t oy = y + neighbourY(outward);
    nextX = x + neighbourX(along);
    nextY = y + neighbourY(along);
    if (!insideFrame(ox, oy, bits.columns, bits.rows) || bitAt(bits.high, cellIndex(ox, oy, bits.rows))) {
        return false;
    }
    const std::int64_t next = cellIndex(nextX, nextY, bits.rows);
    return bitAt(bits.high, next) && !bitAt(bits.visited, next);
}

QFTBX_HD inline bool startsTrace(const std::uint32_t * high, std::int32_t x, std::int32_t y, std::int32_t columns,
                                 std::int32_t rows)
{
    if (!insideFrame(x, y, columns, rows) || !bitAt(high, cellIndex(x, y, rows))) {
        return false;
    }
    bool low = false;
    for (std::int32_t d = 0; d < 8; ++d) {
        const std::int32_t ox = x + neighbourX(d);
        const std::int32_t oy = y + neighbourY(d);
        low = low || (insideFrame(ox, oy, columns, rows) && !bitAt(high, cellIndex(ox, oy, rows)));
    }
    return low;
}

QFTBX_HD inline std::uint32_t candidates(const SheetBits & bits, std::int64_t word, std::int64_t cursor)
{
    const std::uint32_t free = bits.border[word] & ~bits.visited[word];
    if (word < (cursor >> 5)) {
        return 0u;
    }
    return word == (cursor >> 5) ? free & (~0u << (cursor & 31)) : free;
}

QFTBX_HD inline void markVisited(std::uint32_t * visited, std::int64_t index)
{
    visited[index >> 5] |= 1u << (index & 31);
}

QFTBX_HD inline void emitNode(TraceOutput & out, std::int32_t x, std::int32_t y, bool write = true)
{
    if (out.points >= out.capacity) {
        out.overflow = true;
    } else if (write) {
        out.x[out.points] = x;
        out.y[out.points] = y;
    }
    ++out.points;
}

QFTBX_HD inline void closeTrace(TraceOutput & out, std::int32_t first, bool write = true)
{
    if (out.points - first > 1) {
        if (write && out.traces < out.capacity) {
            out.start[out.traces] = first;
        }
        ++out.traces;
    } else {
        out.points = first;
    }
}

QFTBX_HD inline void walkOnProcessor(SheetBits & bits, std::int32_t x, std::int32_t y, TraceOutput & out)
{
    const std::int32_t first = out.points;
    while (true) {
        std::int32_t nextX = 0;
        std::int32_t nextY = 0;
        bool advanced = false;
        for (std::int32_t i = 15; i > 7 && !advanced; --i) {
            advanced = stepTo(bits, x, y, i, nextX, nextY);
        }
        emitNode(out, x, y);
        if (!advanced) {
            break;
        }
        markVisited(bits.visited, cellIndex(x, y, bits.rows));
        x = nextX;
        y = nextY;
    }
    closeTrace(out, first);
}

QFTBX_HD inline double nodePhase(const NodeScale & s, std::int32_t x)
{
    return ((x * s.phaseSpan) / s.phaseCells) + s.phaseBottom;
}

QFTBX_HD inline double nodeMagnitude(const NodeScale & s, std::int32_t y)
{
    return ((y * s.magnitudeSpan) / s.magnitudeCells) + s.magnitudeBottom;
}

QFTBX_HD inline DeviceComplex probeLoop(const NodeScale & s, std::int32_t x, std::int32_t y, bool beforeFirst)
{
    const double phase = beforeFirst ? nodePhase(s, x) - (s.phaseSpan / s.phaseCells) : nodePhase(s, x);
    const double magnitude = nodeMagnitude(s, y) - 1;
    const double r = ::pow(10.0, magnitude / 20.0);
    const double theta = phase * qftbx::math::kPi / 180.0;
    return DeviceComplex(r * ::cos(theta), r * ::sin(theta));
}

QFTBX_HD inline bool allowedBelow(std::size_t sheet, double height, DeviceComplex L, double plant, double nearest,
                                  double farthest, double ratio)
{
    const double loop = ::hypot(L.real(), L.imag());
    const double stability = 20 * ::log10(loop / nearest);
    double value = stability;
    if (sheet == 1) {
        value = stability - 20 * ::log10(loop / farthest);
    } else if (sheet == 2) {
        value = 20 * ::log10(ratio);
    } else if (sheet == 3) {
        value = 20 * ::log10(plant / nearest);
    } else if (sheet == 4) {
        value = 20 * ::log10(loop / plant * ratio);
    }
    return !(value > height);
}

}

#endif
