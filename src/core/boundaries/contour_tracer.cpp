#include <cstdint>
#include "contour_tracer.h"

namespace qftbx {

ContourTracer::ContourTracer(double thresholdDb, const BoundarySheet & sheet){
    m_thresholdDb = thresholdDb;
    m_sheet = &sheet;
}

#ifdef CUDA_AVAILABLE
ContourTracer::ContourTracer(double thresholdDb, const float *sheet){
    m_thresholdDb = thresholdDb;
    m_cudaSheet = sheet;
}
#endif


//Grid-index to Nichols-coordinate conversion: each axis maps index ->
//bottom + index * span / cells. The historical formulas subtracted the
//magnitude TOP (index * span - top) and the phase span, which only equals
//the bottom on grids symmetric around zero / ending at zero: any other
//grid produced boundaries shifted by (top - |bottom|).
TraceSet ContourTracer::trace(double phaseSpan, double magnitudeSpan,
                                                     double phaseBottom, double magnitudeBottom)
{

    std::int32_t width = static_cast<std::int32_t>(m_sheet->at(0).size());
    std::int32_t height = static_cast<std::int32_t>(m_sheet->size());
    double threshold = m_thresholdDb;

    std::int32_t phaseCells = width - 1;
    std::int32_t magnitudeCells = height - 1;

    QVector <bool> visited ((width + 1) * (height + 1), false);

    TraceSet traces;

    // We look for pixels contained in a connected set (gray-level value >= threshold) in the image
    for (std::int32_t column = 1; column < width-1; column++){
        for (std::int32_t row = 1; row < height-1; row++)
        {

            if ((m_sheet->at(static_cast<std::size_t>(row)).at(static_cast<std::size_t>(column)) >= threshold) && (!visited.at(row * width + column))){

                Trace trace;


                std::int32_t currentX = column;
                std::int32_t currentY = row;

                std::int32_t advanced = 0;

                std::int32_t x,y;

                while (true){

                    advanced = 0;

                    for (std::int32_t i = 15; i>7; i--)
                    {
                        x =  currentX + kNeighbourX[i % 8];
                        y =  currentY + kNeighbourY[i % 8];

                        if ((x > 0) && (x < width-1) && (y > 0) && (y < height-1) && (m_sheet->at(static_cast<std::size_t>(y)).at(static_cast<std::size_t>(x)) < threshold))
                        {
                            x =  currentX + kNeighbourX[(i - 1) % 8];
                            y =  currentY + kNeighbourY[(i - 1) % 8];

                            if ((m_sheet->at(static_cast<std::size_t>(y)).at(static_cast<std::size_t>(x)) >= threshold) && (!visited.at(y * width + x))){


                                trace.push_back(QPointF(((currentX * phaseSpan) / phaseCells) + phaseBottom, ((currentY *
                                                      magnitudeSpan) / magnitudeCells) + magnitudeBottom));

                                visited.replace(currentY * width + currentX, true);
                                currentX = x;
                                currentY = y;
                                advanced++;
                                break;
                            }
                        }
                    }


                    if (advanced == 0){

                        trace.push_back(QPointF(((currentX * phaseSpan) / phaseCells) + phaseBottom, ((currentY *
                                              magnitudeSpan) / magnitudeCells) + magnitudeBottom));

                        break;
                    }
                }

                //Degenerate traces (<= 1 point) are discarded.
                if (trace.size() <= 1){
                } else {

                    trace.insert(trace.begin(), QPointF(trace.front().x()-(phaseSpan / phaseCells), trace.front().y()));
                    trace.push_back(QPointF(trace.back().x()+(phaseSpan / phaseCells), trace.back().y()));

                    traces.push_back(std::move(trace));
                }
            }
        }
    }

    return traces;
}


#ifdef CUDA_AVAILABLE
TraceSet ContourTracer::trace(double phaseSpan, double phaseCount, double magnitudeSpan,
                                                     double magnitudeCount, double phaseBottom, double magnitudeBottom){

    std::int32_t width = phaseCount;
    std::int32_t height = magnitudeCount;
    double threshold = m_thresholdDb;
    phaseCount--;
    magnitudeCount--;


    QVector <bool> visited ((width + 1) * (height + 1), false);

    TraceSet traces;

    // We look for pixels contained in a connected set (gray-level value >= threshold) in the image
    for (std::int32_t column = 1; column < width-1; column++){
        for (std::int32_t row = 1; row < height-1; row++)
        {

            if ((m_cudaSheet[column * height + row] >= threshold) && (!visited.at(row * width + column))){

                Trace trace;


                std::int32_t currentX = column;
                std::int32_t currentY = row;

                std::int32_t advanced = 0;

                std::int32_t x,y;

                while (true){

                    advanced = 0;

                    for (std::int32_t i = 15; i>7; i--)
                    {
                        x =  currentX + kNeighbourX[i % 8];
                        y =  currentY + kNeighbourY[i % 8];

                        if ((x > 0) && (x < width-1) && (y > 0) && (y < height-1) &&
                                (m_cudaSheet[x * height + y] <= threshold))
                        {
                            x =  currentX + kNeighbourX[(i - 1) % 8];
                            y =  currentY + kNeighbourY[(i - 1) % 8];

                            if ((m_cudaSheet[x * height + y] > threshold) && (!visited.at(y * width + x))){
                                trace.push_back(QPointF(((currentX * phaseSpan) / phaseCount) + phaseBottom, ((currentY * magnitudeSpan) / magnitudeCount) + magnitudeBottom));
                                visited.replace(currentY * width + currentX, true);
                                currentX = x;
                                currentY = y;
                                advanced++;
                                break;
                            }
                        }
                    }


                    if (advanced == 0){
                        trace.push_back(QPointF(((currentX * phaseSpan) / phaseCount) + phaseBottom, ((currentY * magnitudeSpan) / magnitudeCount) + magnitudeBottom));
                        break;
                    }
                }

                if (trace.size() <= 1){
                    //Degenerate: dropped.
                }else if (trace.size() == 2){
                    //Two points is the signature of a region the first walk
                    //could not follow; it is retraced from scratch.
                    Trace retrace;


                    std::int32_t retraceX = column;
                    std::int32_t retraceY = row;

                    std::int32_t readvanced = 0;

                    while (true){

                        readvanced = 0;

                        for (std::int32_t i = 0; i<8; i++)
                        {
                            x =  retraceX + kNeighbourX[i];
                            y =  retraceY + kNeighbourY[i];

                            if ((x > 0) && (x < width-1) && (y > 0) && (y < height-1) &&
                                    (m_cudaSheet[x * height + y] <= threshold))
                            {
                                x =  retraceX + kNeighbourX[(i + 1) % 8];
                                y =  retraceY + kNeighbourY[(i + 1) % 8];

                                if ((m_cudaSheet[x * height + y] > threshold) && (!visited.at(y * width + x))){
                                    retrace.push_back(QPointF(((retraceX * phaseSpan) / phaseCount) + phaseBottom, ((retraceY *
                                                          magnitudeSpan) / magnitudeCount) + magnitudeBottom));
                                    visited.replace(retraceY * width + retraceX, true);
                                    retraceX = x;
                                    retraceY = y;
                                    readvanced++;
                                    break;
                                }
                            }
                        }


                        if (readvanced == 0){
                            retrace.push_back(QPointF(((retraceX * phaseSpan) / phaseCount) + phaseBottom, ((retraceY *
                                                  magnitudeSpan) / magnitudeCount) + magnitudeBottom));
                            break;
                        }
                    }

                    if (retrace.size() > 1){
                        traces.push_back(std::move(retrace));
                    }
                } else {
                    traces.push_back(std::move(trace));
                }
            }
        }
    }

    return traces;

}
#endif

} // namespace qftbx
