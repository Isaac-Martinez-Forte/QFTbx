#include "contour_tracer.h"

namespace qftbx {

ContourTracer::ContourTracer(qreal thresholdDb, QVector<QVector<qreal> *> *sheet){
    m_thresholdDb = thresholdDb;
    m_sheet = sheet;
}

#ifdef CUDA_AVAILABLE
ContourTracer::ContourTracer(qreal thresholdDb, const float *sheet){
    m_thresholdDb = thresholdDb;
    m_cudaSheet = sheet;
}
#endif


//Grid-index to Nichols-coordinate conversion: each axis maps index ->
//bottom + index * span / cells. The historical formulas subtracted the
//magnitude TOP (index * span - top) and the phase span, which only equals
//the bottom on grids symmetric around zero / ending at zero: any other
//grid produced boundaries shifted by (top - |bottom|).
QVector <QVector <QPointF> *> * ContourTracer::trace(qreal phaseSpan, qreal magnitudeSpan,
                                                     qreal phaseBottom, qreal magnitudeBottom)
{

    qint32 width = m_sheet->at(0)->size();
    qint32 height = m_sheet->size();
    qreal threshold = m_thresholdDb;

    qint32 phaseCells = width - 1;
    qint32 magnitudeCells = height - 1;

    QVector <bool> visited ((width + 1) * (height + 1), false);

    QVector <QVector <QPointF> *> * traces = new QVector <QVector <QPointF> *> ();

    // We look for pixels contained in a connected set (gray-level value >= threshold) in the image
    for (qint32 column = 1; column < width-1; column++){
        for (qint32 row = 1; row < height-1; row++)
        {

            if ((m_sheet->at(row)->at(column) >= threshold) && (!visited.at(row * width + column))){

                QVector <QPointF> * trace = new QVector <QPointF> ();


                qint32 currentX = column;
                qint32 currentY = row;

                qint32 advanced = 0;

                qint32 x,y;

                while (true){

                    advanced = 0;

                    for (qint32 i = 15; i>7; i--)
                    {
                        x =  currentX + kNeighbourX[i % 8];
                        y =  currentY + kNeighbourY[i % 8];

                        if ((x > 0) && (x < width-1) && (y > 0) && (y < height-1) && (m_sheet->at(y)->at(x) < threshold))
                        {
                            x =  currentX + kNeighbourX[(i - 1) % 8];
                            y =  currentY + kNeighbourY[(i - 1) % 8];

                            if ((m_sheet->at(y)->at(x) >= threshold) && (!visited.at(y * width + x))){


                                trace->append(QPointF(((currentX * phaseSpan) / phaseCells) + phaseBottom, ((currentY *
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

                        trace->append(QPointF(((currentX * phaseSpan) / phaseCells) + phaseBottom, ((currentY *
                                              magnitudeSpan) / magnitudeCells) + magnitudeBottom));

                        break;
                    }
                }

                //Degenerate traces (<= 1 point) are discarded.
                if (trace->size() <= 1){
                    delete trace;
                } else {

                    trace->prepend(QPointF(trace->first().x()-(phaseSpan / phaseCells), trace->first().y()));
                    trace->append(QPointF(trace->last().x()+(phaseSpan / phaseCells), trace->last().y()));

                    traces->append(trace);
                }
            }
        }
    }

    return traces;
}


#ifdef CUDA_AVAILABLE
QVector <QVector <QPointF> *> * ContourTracer::trace(qreal phaseSpan, qreal phaseCount, qreal magnitudeSpan,
                                                     qreal magnitudeCount, qreal phaseBottom, qreal magnitudeBottom){

    qint32 width = phaseCount;
    qint32 height = magnitudeCount;
    qreal threshold = m_thresholdDb;
    phaseCount--;
    magnitudeCount--;


    QVector <bool> visited ((width + 1) * (height + 1), false);

    QVector <QVector <QPointF> *> * traces = new QVector <QVector <QPointF> *> ();

    // We look for pixels contained in a connected set (gray-level value >= threshold) in the image
    for (qint32 column = 1; column < width-1; column++){
        for (qint32 row = 1; row < height-1; row++)
        {

            if ((m_cudaSheet[column * height + row] >= threshold) && (!visited.at(row * width + column))){

                QVector <QPointF> * trace = new QVector <QPointF> ();


                qint32 currentX = column;
                qint32 currentY = row;

                qint32 advanced = 0;

                qint32 x,y;

                while (true){

                    advanced = 0;

                    for (qint32 i = 15; i>7; i--)
                    {
                        x =  currentX + kNeighbourX[i % 8];
                        y =  currentY + kNeighbourY[i % 8];

                        if ((x > 0) && (x < width-1) && (y > 0) && (y < height-1) &&
                                (m_cudaSheet[x * height + y] <= threshold))
                        {
                            x =  currentX + kNeighbourX[(i - 1) % 8];
                            y =  currentY + kNeighbourY[(i - 1) % 8];

                            if ((m_cudaSheet[x * height + y] > threshold) && (!visited.at(y * width + x))){
                                trace->append(QPointF(((currentX * phaseSpan) / phaseCount) + phaseBottom, ((currentY * magnitudeSpan) / magnitudeCount) + magnitudeBottom));
                                visited.replace(currentY * width + currentX, true);
                                currentX = x;
                                currentY = y;
                                advanced++;
                                break;
                            }
                        }
                    }


                    if (advanced == 0){
                        trace->append(QPointF(((currentX * phaseSpan) / phaseCount) + phaseBottom, ((currentY * magnitudeSpan) / magnitudeCount) + magnitudeBottom));
                        break;
                    }
                }

                if (trace->size() <= 1){
                    delete trace;
                }else if (trace->size() == 2){
                    delete trace;

                    QVector <QPointF> * retrace = new QVector <QPointF> ();


                    qint32 retraceX = column;
                    qint32 retraceY = row;

                    qint32 readvanced = 0;

                    while (true){

                        readvanced = 0;

                        for (qint32 i = 0; i<8; i++)
                        {
                            x =  retraceX + kNeighbourX[i];
                            y =  retraceY + kNeighbourY[i];

                            if ((x > 0) && (x < width-1) && (y > 0) && (y < height-1) &&
                                    (m_cudaSheet[x * height + y] <= threshold))
                            {
                                x =  retraceX + kNeighbourX[(i + 1) % 8];
                                y =  retraceY + kNeighbourY[(i + 1) % 8];

                                if ((m_cudaSheet[x * height + y] > threshold) && (!visited.at(y * width + x))){
                                    retrace->append(QPointF(((retraceX * phaseSpan) / phaseCount) + phaseBottom, ((retraceY *
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
                            retrace->append(QPointF(((retraceX * phaseSpan) / phaseCount) + phaseBottom, ((retraceY *
                                                  magnitudeSpan) / magnitudeCount) + magnitudeBottom));
                            break;
                        }
                    }

                    if (retrace->size() <= 1){
                        delete retrace;
                    } else{
                        traces->append(retrace);
                    }
                } else {
                    traces->append(trace);
                }
            }
        }
    }

    return traces;

}
#endif

} // namespace qftbx
