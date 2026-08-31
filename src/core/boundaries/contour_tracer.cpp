#include "contour_tracer.h"

namespace qftbx {

ContourTracer::ContourTracer(qreal thresholdDb, QVector<QVector<qreal> *> *sheet){
    m_thresholdDb = thresholdDb;
    m_sheet = sheet;
}

#ifdef CUDA_AVAILABLE
ContourTracer::ContourTracer(qreal thresholdDb, float *sheet){
    m_thresholdDb = thresholdDb;
    m_cudaSheet = sheet;
}
#endif


QVector <QVector <QPointF> *> * ContourTracer::trace(qreal phasePoints, qreal magnitudePoints, qreal magnitudeShift)
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


                                trace->append(QPointF(((currentX * phasePoints) / phaseCells) - phasePoints, ((currentY *
                                                      magnitudePoints) / magnitudeCells) - magnitudeShift));

                                visited.replace(currentY * width + currentX, true);
                                currentX = x;
                                currentY = y;
                                advanced++;
                                break;
                            }
                        }
                    }


                    if (advanced == 0){

                        trace->append(QPointF(((currentX * phasePoints) / phaseCells) - phasePoints, ((currentY *
                                              magnitudePoints) / magnitudeCells) - magnitudeShift));

                        break;
                    }
                }

                //Degenerate traces (<= 1 point) are discarded.
                if (trace->size() <= 1){
                    delete trace;
                } else {

                    trace->prepend(QPointF(trace->first().x()-(phasePoints / phaseCells), trace->first().y()));
                    trace->append(QPointF(trace->last().x()+(phasePoints / phaseCells), trace->last().y()));

                    traces->append(trace);
                }
            }
        }
    }

    return traces;
}


#ifdef CUDA_AVAILABLE
QVector <QVector <QPointF> *> * ContourTracer::trace(qreal phasePoints, qreal phaseCount, qreal magnitudePoints,
                                                     qreal magnitudeCount, qreal magnitudeShift){

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
                                trace->append(QPointF(((currentX * phasePoints) / phaseCount) - phasePoints, ((currentY *
                                                      magnitudePoints) / magnitudeCount) - magnitudeShift));
                                visited.replace(currentY * width + currentX, true);
                                currentX = x;
                                currentY = y;
                                advanced++;
                                break;
                            }
                        }
                    }


                    if (advanced == 0){
                        trace->append(QPointF(((currentX * phasePoints) / phaseCount) - phasePoints, ((currentY *
                                              magnitudePoints) / magnitudeCount) - magnitudeShift));
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
                                    retrace->append(QPointF(((retraceX * phasePoints) / phaseCount) - phasePoints, ((retraceY *
                                                          magnitudePoints) / magnitudeCount) - magnitudeShift));
                                    visited.replace(retraceY * width + retraceX, true);
                                    retraceX = x;
                                    retraceY = y;
                                    readvanced++;
                                    break;
                                }
                            }
                        }


                        if (readvanced == 0){
                            retrace->append(QPointF(((retraceX * phasePoints) / phaseCount) - phasePoints, ((retraceY *
                                                  magnitudePoints) / magnitudeCount) - magnitudeShift));
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
