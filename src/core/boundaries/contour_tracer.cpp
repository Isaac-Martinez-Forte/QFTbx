#include <limits>
#include <algorithm>
#include <vector>
#include <cstdint>
#include "src/core/boundaries/contour_tracer.h"

namespace qftbx {

ContourTracer::ContourTracer(double thresholdDb, const BoundarySheet & sheet)
    : m_thresholdDb(thresholdDb), m_sheet(&sheet)
{
}

#ifdef CUDA_AVAILABLE
ContourTracer::ContourTracer(double thresholdDb, const float *sheet)
    : m_thresholdDb(thresholdDb), m_cudaSheet(sheet)
{
}
#endif

namespace {

//Moore neighbourhood, clockwise from north:
// Direction-number        Y
//   NE-7    N-0    NW-1   |
//   E-6      *     W-2    v
//   SE-5    S-4    SW-3
// X -->
//                            N   NW   W   SW   S   SE   E   NE
constexpr std::int8_t kNeighbourX[8] = {  0,   1,  1,   1,  0,  -1, -1,  -1 };
constexpr std::int8_t kNeighbourY[8] = { -1,  -1,  0,   1,  1,   1,  0,  -1 };

//The flat index of a grid cell, multiplied in std::size_t. Every one of
//these used to read `static_cast<std::size_t>(row * width + column)`, which
//multiplies in std::int32_t and widens only the result: an overflow would
//already have happened before the cast could help. The same cast shape
//sized the visited vector, so a grid large enough to overflow would have
//produced a vector too small for the indices it is then read with - and one
//of those reads is an operator[], with no bounds check to catch it.
inline std::size_t flatIndex(std::int32_t row, std::int32_t column, std::int32_t width)
{
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(width) +
            static_cast<std::size_t>(column);
}

//Cells of a (width + 1) x (height + 1) grid, likewise widened first.
inline std::size_t cellCount(std::int32_t width, std::int32_t height)
{
    return (static_cast<std::size_t>(width) + 1) *
            (static_cast<std::size_t>(height) + 1);
}

//Grid-index to Nichols-coordinate conversion: each axis maps index ->
//bottom + index * span / cells. The historical formulas subtracted the
//magnitude TOP (index * span - top) and the phase span, which only equals
//the bottom on grids symmetric around zero / ending at zero: any other
//grid produced boundaries shifted by (top - |bottom|).
//
//The walk itself, over any sheet that answers cellAt(x, y): every pixel of a
//connected region at or above the threshold that has not been visited starts
//a Moore boundary trace; it stops where no unvisited border neighbour is
//left. Degenerate traces (one point) are dropped; the others are extended
//by one synthetic point on each side so the 1D union closes them against
//the window frame.
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

            //Degenerate traces (<= 1 point) are discarded.
            if (trace.size() > 1){
                trace.insert(trace.begin(), NicholsPoint(trace.front().phase - (phaseSpan / phaseCells), trace.front().magnitude));
                trace.push_back(NicholsPoint(trace.back().phase + (phaseSpan / phaseCells), trace.back().magnitude));

                traces.push_back(std::move(trace));
            }
        }
    }

    return traces;
}

} // namespace

TraceSet ContourTracer::trace(double phaseSpan, double magnitudeSpan,
                              double phaseBottom, double magnitudeBottom)
{
    const std::int32_t width = static_cast<std::int32_t>(m_sheet->at(0).size());
    const std::int32_t height = static_cast<std::int32_t>(m_sheet->size());

    //One row per magnitude, one column per phase.
    const auto cellAt = [this](std::int32_t x, std::int32_t y) {
        return m_sheet->at(static_cast<std::size_t>(y)).at(static_cast<std::size_t>(x));
    };

    return traceCells(width, height, m_thresholdDb, cellAt,
                      phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
}

#ifdef CUDA_AVAILABLE
TraceSet ContourTracer::trace(double phaseSpan, double phaseCount, double magnitudeSpan,
                              double magnitudeCount, double phaseBottom, double magnitudeBottom){

    //Both arrive as doubles (the CUDA overload's signature) and went into an
    //std::int32_t with nothing checked, which is undefined behaviour for a
    //value out of range. No compiler on the development machine sees this
    //branch, so it is written to be right by inspection.
    const std::int32_t width = static_cast<std::int32_t>(
                std::min(phaseCount, static_cast<double>(std::numeric_limits<std::int32_t>::max())));
    const std::int32_t height = static_cast<std::int32_t>(
                std::min(magnitudeCount, static_cast<double>(std::numeric_limits<std::int32_t>::max())));

    //Column-major: the phase index times the magnitude count, plus the
    //magnitude index.
    const auto cellAt = [this, height](std::int32_t x, std::int32_t y) {
        return static_cast<double>(m_cudaSheet[static_cast<std::size_t>(x) * static_cast<std::size_t>(height)
                                               + static_cast<std::size_t>(y)]);
    };

    return traceCells(width, height, m_thresholdDb, cellAt,
                      phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
}
#endif

} // namespace qftbx
