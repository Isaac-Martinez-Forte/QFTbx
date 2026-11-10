/**
 * @file
 * @brief Moore-neighbourhood boundary tracing over a sampled sheet.
 *
 * Every unvisited cell at or above the threshold starts a trace that follows
 * the region border clockwise through the eight neighbours and stops when no
 * unvisited border neighbour is left. One-point traces are dropped, and every
 * other trace is extended by a synthetic point on each side so the union can
 * close it against the window frame. Indices are widened to size_t before
 * multiplying, and axes map an index from their bottom end.
 */

#include <vector>
#include <cstdint>
#include "src/core/boundaries/contour_tracer.h"

namespace qftbx {

ContourTracer::ContourTracer(double thresholdDb, const BoundarySheet & sheet)
    : m_thresholdDb(thresholdDb), m_sheet(&sheet)
{
}

namespace {

constexpr std::int8_t kNeighbourX[8] = {  0,   1,  1,   1,  0,  -1, -1,  -1 };
constexpr std::int8_t kNeighbourY[8] = { -1,  -1,  0,   1,  1,   1,  0,  -1 };

inline std::size_t flatIndex(std::int32_t row, std::int32_t column, std::int32_t width)
{
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(width) +
            static_cast<std::size_t>(column);
}

inline std::size_t cellCount(std::int32_t width, std::int32_t height)
{
    return (static_cast<std::size_t>(width) + 1) *
            (static_cast<std::size_t>(height) + 1);
}

template <class CellAt>
TraceSet traceCells(std::int32_t width, std::int32_t height, double threshold, CellAt cellAt,
                    double phaseSpan, double magnitudeSpan, double phaseBottom, double magnitudeBottom)
{
    const std::int32_t phaseCells = width - 1;
    const std::int32_t magnitudeCells = height - 1;

    std::vector <bool> visited (cellCount(width, height), false);

    TraceSet traces;

    const auto toNichols = [&](std::int32_t x, std::int32_t y) {
        return NicholsPoint(((x * phaseSpan) / phaseCells) + phaseBottom,
                            ((y * magnitudeSpan) / magnitudeCells) + magnitudeBottom);
    };

    for (std::int32_t column = 1; column < width - 1; column++){
        for (std::int32_t row = 1; row < height - 1; row++)
        {
            if (!(cellAt(column, row) >= threshold) || visited.at(flatIndex(row, column, width))) {
                continue;
            }

            Trace trace;

            std::int32_t currentX = column;
            std::int32_t currentY = row;

            while (true){
                bool advanced = false;

                for (std::int32_t i = 15; i > 7; i--)
                {
                    std::int32_t x = currentX + kNeighbourX[i % 8];
                    std::int32_t y = currentY + kNeighbourY[i % 8];

                    if ((x > 0) && (x < width - 1) && (y > 0) && (y < height - 1) && (cellAt(x, y) < threshold))
                    {
                        x = currentX + kNeighbourX[(i - 1) % 8];
                        y = currentY + kNeighbourY[(i - 1) % 8];

                        if ((cellAt(x, y) >= threshold) && (!visited.at(flatIndex(y, x, width)))){
                            trace.push_back(toNichols(currentX, currentY));
                            visited[flatIndex(currentY, currentX, width)] = true;
                            currentX = x;
                            currentY = y;
                            advanced = true;
                            break;
                        }
                    }
                }

                if (!advanced){
                    trace.push_back(toNichols(currentX, currentY));
                    break;
                }
            }

            if (trace.size() > 1){
                trace.insert(trace.begin(), NicholsPoint(trace.front().phase - (phaseSpan / phaseCells), trace.front().magnitude));
                trace.push_back(NicholsPoint(trace.back().phase + (phaseSpan / phaseCells), trace.back().magnitude));

                traces.push_back(std::move(trace));
            }
        }
    }

    return traces;
}

}

TraceSet ContourTracer::trace(double phaseSpan, double magnitudeSpan,
                              double phaseBottom, double magnitudeBottom)
{
    const std::int32_t width = static_cast<std::int32_t>(m_sheet->at(0).size());
    const std::int32_t height = static_cast<std::int32_t>(m_sheet->size());

    const auto cellAt = [this](std::int32_t x, std::int32_t y) {
        return m_sheet->at(static_cast<std::size_t>(y)).at(static_cast<std::size_t>(x));
    };

    return traceCells(width, height, m_thresholdDb, cellAt,
                      phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
}

}
