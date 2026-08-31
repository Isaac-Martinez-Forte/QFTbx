#include "boundary_union_1d.h"

namespace qftbx {

using std::abs;

BoundaryUnion1D::BoundaryUnion1D()
{

}

BoundaryUnion1D::~BoundaryUnion1D()
{

}

qint32 BoundaryUnion1D::bucketIndex(qreal x, qreal totalPhase)
{
    qreal res = totalPhase-(abs(x)*(totalPhase/(qreal)kPhaseDegrees));
    if(res<0) res=0;
    return (qint32) res;
}

void BoundaryUnion1D::insertSorted(QVector<QVector<QPointF> *> * layerBucketsRow, QVector<QPointF>::iterator it, qint32 index, QPointF point, qreal totalPhase)
{
    bool duplicated = false;
    for(qint32 i=0;i<layerBucketsRow->at(bucketIndex(point.x(), totalPhase))->size();i++)
    {
        QPointF bucketPoint = layerBucketsRow->at(bucketIndex(point.x(), totalPhase))->at(i);
        if(point.y()==bucketPoint.y()) duplicated = true;
    }
    if(!duplicated) layerBucketsRow->at(bucketIndex(point.x(), totalPhase))->insert(it+index,point);
}

QVector< QVector< QVector<QPointF> * > * > * BoundaryUnion1D::buildLayerBuckets(QVector< QVector<QPointF> * > * &chosenCurves, qreal totalPhase, bool open, bool upper)
{
    QVector< QVector< QVector<QPointF> * > * > * layerBuckets = new QVector< QVector< QVector<QPointF> * > * >();
    for(qint32 i=0;i<kLayerCount;i++)
    {
        layerBuckets->append(new QVector< QVector<QPointF> * >());
        for(qint32 j=0;j<=(qint32)totalPhase;j++)
        {
            layerBuckets->at(i)->append(new QVector<QPointF>());
        }
        for(qint32 j=0;j<(qint32)chosenCurves->at(i)->size();j++)
        {
            QPointF point = chosenCurves->at(i)->at(j);
            qint32 index = 0;
            QVector<QPointF>::iterator it = layerBuckets->at(i)->at(bucketIndex(point.x(), totalPhase))->begin();
            for(qint32 k=0;k<(qint32)layerBuckets->at(i)->at(bucketIndex(point.x(), totalPhase))->size();k++)
            {
                if(upper)
                {
                    if(point.y()<layerBuckets->at(i)->at(bucketIndex(point.x(), totalPhase))->at(k).y()) index++;
                }
                else if(point.y()>layerBuckets->at(i)->at(bucketIndex(point.x(), totalPhase))->at(k).y()) index++;
            }
            if(!open||(open&&(layerBuckets->at(i)->at(bucketIndex(point.x(), totalPhase))->size()==0))) insertSorted(layerBuckets->at(i), it, index, point, totalPhase);
        }

    }
    return layerBuckets;
}

QVector<QPointF> * BoundaryUnion1D::drawFirstLayer(QVector< QVector<QPointF> * > * &chosenCurves, QVector< QVector< QVector<QPointF> * > * > * &layerBuckets, qreal totalPhase, bool open1, bool open2)
{
    QVector<QPointF> * layer1 = new QVector<QPointF>();
    if(open1)
    {
        for(qint32 i=0;i<(qint32)chosenCurves->at(1)->size();i++)
        {
            qint32 bucketSize = (qint32) layerBuckets->at(0)->at(bucketIndex(chosenCurves->at(1)->at(i).x(), totalPhase))->size();
            QPointF curvePoint = chosenCurves->at(1)->at(i);
            QVector<QPointF> * bucketPoints = layerBuckets->at(0)->at(bucketIndex(chosenCurves->at(1)->at(i).x(), totalPhase));
            if(bucketSize==0) layer1->push_back(curvePoint);
            if(bucketSize==1)
            {
                if(curvePoint.y() >= bucketPoints->at(0).y()) layer1->push_back(curvePoint);
            }
            else if(bucketSize==2)
            {
                if((curvePoint.y() >= bucketPoints->at(0).y())||
                        (curvePoint.y() <= bucketPoints->at(1).y())) layer1->push_back(curvePoint);
            }
            else
            {
                for(qint32 j=0;j<(qint32)bucketPoints->size();j+=2)
                {
                    if(curvePoint.y() >= bucketPoints->at(0).y()) layer1->push_back(curvePoint);
                    if(bucketSize-j>1)
                    {
                        if((curvePoint.y() <= bucketPoints->at(j+1).y())&&
                                (curvePoint.y() >= bucketPoints->at(j+2).y())) layer1->push_back(curvePoint);
                    }
                    else
                    {
                        if((curvePoint.y() >= bucketPoints->at(j).y())&&
                                (curvePoint.y() <= bucketPoints->at(j-1).y())) layer1->push_back(curvePoint);
                    }
                }
            }
        }
    }
    else
    {
        for(qint32 i=0;i<(qint32)chosenCurves->at(1)->size();i++)
        {
            qint32 bucketSize = (qint32) layerBuckets->at(0)->at(bucketIndex(chosenCurves->at(1)->at(i).x(), totalPhase))->size();
            QPointF curvePoint = chosenCurves->at(1)->at(i);
            QVector<QPointF> * bucketPoints = layerBuckets->at(0)->at(bucketIndex(chosenCurves->at(1)->at(i).x(), totalPhase));
            if(bucketSize>1&&bucketSize%2==0)
            {
                bool outside = true;
                if(open2&&curvePoint.y() <= bucketPoints->at(0).y())
                {
                    outside = false;
                }
                else
                {
                    for(qint32 j=0;j<bucketSize;j+=2)
                    {
                        //cout << bucketPoints[j].y() << " ... " << bucketPoints[j+1].y() << endl;
                        if((curvePoint.y() <= bucketPoints->at(j).y())&&
                                (curvePoint.y() >= bucketPoints->at(j+1).y())) outside = false;
                    }
                }
                if(outside) layer1->push_back(curvePoint);
            }
            else if(bucketSize%2==1&&curvePoint.y() > bucketPoints->at(0).y())
            {
                layer1->push_back(curvePoint);
            }
            else if(bucketSize==0) layer1->push_back(curvePoint);
        }
    }
    return layer1;
}

QVector<QPointF> * BoundaryUnion1D::drawSecondLayer(QVector< QVector<QPointF> * > * &chosenCurves, QVector< QVector< QVector<QPointF> * > * > * &layerBuckets, qreal totalPhase, bool open1, bool open2)
{
    QVector<QPointF> * layer2 = new QVector<QPointF>();
    if(open2)
    {
        for(qint32 i=0;i<(qint32)chosenCurves->at(0)->size();i++)
        {
            qint32 bucketSize = (qint32) layerBuckets->at(1)->at(bucketIndex(chosenCurves->at(0)->at(i).x(), totalPhase))->size();
            QPointF curvePoint = chosenCurves->at(0)->at(i);
            QVector<QPointF> * bucketPoints = layerBuckets->at(1)->at(bucketIndex(chosenCurves->at(0)->at(i).x(), totalPhase));
            if(bucketSize==0) layer2->push_back(curvePoint);
            if(bucketSize==1)
            {
                if(curvePoint.y() >= bucketPoints->at(0).y()) layer2->push_back(curvePoint);
            }
            else if(bucketSize==2)
            {
                if((curvePoint.y() >= bucketPoints->at(0).y())||
                        (curvePoint.y() <= bucketPoints->at(1).y())) layer2->push_back(curvePoint);
            }
            else
            {
                for(qint32 j=0;j<(qint32)bucketPoints->size();j+=2)
                {
                    if(curvePoint.y() >= bucketPoints->at(0).y()) layer2->push_back(curvePoint);
                    if(bucketSize-j>1)
                    {
                        if((curvePoint.y() <= bucketPoints->at(j+1).y())&&
                                (curvePoint.y() >= bucketPoints->at(j+2).y())) layer2->push_back(curvePoint);
                    }
                    else
                    {
                        if((curvePoint.y() >= bucketPoints->at(j).y())&&
                                (curvePoint.y() <= bucketPoints->at(j-1).y())) layer2->push_back(curvePoint);
                    }
                }
            }
        }
    }
    else
    {
        for(qint32 i=0;i<(qint32)chosenCurves->at(0)->size();i++)
        {
            qint32 bucketSize = layerBuckets->at(1)->at(bucketIndex(chosenCurves->at(0)->at(i).x(), totalPhase))->size();
            QPointF curvePoint = chosenCurves->at(0)->at(i);
            QVector<QPointF> * bucketPoints = layerBuckets->at(1)->at(bucketIndex(chosenCurves->at(0)->at(i).x(), totalPhase));
            if(bucketSize>1&&bucketSize%2==0)
            {
                bool outside = true;
                if(open1&&curvePoint.y() <= bucketPoints->at(0).y())
                {
                    outside = false;
                }
                else
                {
                    for(qint32 j=0;j<bucketSize;j+=2)
                    {
                        if((curvePoint.y() < bucketPoints->at(j).y())&&
                                (curvePoint.y() > bucketPoints->at(j+1).y())) outside = false;
                    }
                }
                if(outside) layer2->push_back(curvePoint);
            }
            else if(bucketSize%2==1&&curvePoint.y() > bucketPoints->at(0).y())
            {
                layer2->push_back(curvePoint);
            }
            else if(bucketSize==0) layer2->push_back(curvePoint);
        }
    }
    return layer2;
}

inline qint32 BoundaryUnion1D::bucketIndex(qreal x, qreal totalPhase, qint32 phaseCount)
{
    double res = (abs(x)*((qreal)phaseCount/totalPhase));
    if(res<0) res=0;
    //The synthetic border point (|x| == totalPhase) used to yield bucket
    //phaseCount out of phaseCount buckets: out of range.
    if(res > phaseCount - 1) res = phaseCount - 1;
    return (qint32) res;
}

QVector< QVector<QPointF> * > * BoundaryUnion1D::buildUnionBuckets(QVector<QPointF> * &unionPoints, qreal totalPhase, qint32 pointCount)
{
    QVector< QVector<QPointF> * > * unionBucketsRow = new QVector< QVector<QPointF> * > ();


    for(qint32 i=0;i<pointCount;i++)
    {
        unionBucketsRow->append(new QVector<QPointF> ());
    }

    QPointF point;

    //From i=0 (the first point used to be skipped), inserted sorted by
    //magnitude and deduplicated, like the layer buckets (and like the
    //historical file format).
    for (qint32 i=0;i < unionPoints->size(); i++) {
        point = unionPoints->at(i);
        QVector<QPointF> * bucket = unionBucketsRow->at(bucketIndex(point.x(), totalPhase, pointCount));

        qint32 pos = 0;
        bool duplicated = false;
        for (; pos < bucket->size(); pos++){
            if (bucket->at(pos).y() == point.y()){
                duplicated = true;
                break;
            }
            if (bucket->at(pos).y() > point.y()){
                break;
            }
        }
        if (!duplicated){
            bucket->insert(pos, point);
        }
    }

    return unionBucketsRow;
}

QVector<QPointF> * BoundaryUnion1D::mergeLayers(QVector<QPointF> * &layer1, QVector<QPointF> * &layer2)
{
    QVector<QPointF> * mergedLayers = new QVector<QPointF>(*layer1+*layer2);
    return mergedLayers;
}

void BoundaryUnion1D::run(BoundaryData * boundaries,
                                                      QVector <QMap <QString, QVector <QPoint> * > * > * traceMetadata)
{
    m_unionBuckets = new QVector< QVector < QVector<QPointF> * > * > ();
    m_unionVectors = new QVector< QVector<QPointF> * > ();
    m_openFlags = new QVector<bool>();
    m_upperFlags = new QVector<bool>();


    //One map per design frequency; each map holds, per specification, a
    //parametric curve of (phase, magnitude) points spanning -360 to 0
    //degrees.
    QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * boundariesPerFrequency = boundaries->boundaries();

    qreal totalPhase = -boundaries->phaseRange().x();
    qint32 phasePointCount = boundaries->phaseCount();

    for(qint32 i=0;i<boundariesPerFrequency->size();i++)
    {

        QMap <QString, QVector <QVector <QPointF> * > *> * map = boundariesPerFrequency->at(i);

        QMap <QString, QVector <QPoint> * > * metadataMap = traceMetadata->at(i);

        QVector<QPointF> * unionPoints = new QVector <QPointF> ();
        bool open1 = false, upper = false, open2 = false;
        m_openFlags->append(false);
        m_upperFlags->append(false);

        bool firstSpecification = true;
        for (auto it = map->constBegin(); it != map->constEnd(); ++it)
        {
            QVector <QVector <QPointF> * > * specificationTraces = it.value();

            //Metadata of THE SAME specification (a fresh iterator used to
            //be opened here, always reading the first key of the map).
            QVector <QPoint> * curveMetadata = metadataMap->value(it.key());
            if (curveMetadata != NULL && !curveMetadata->isEmpty())
            {
                //An x of 0 marks the allowed side as above, 1 as below; the
                //first point suffices, the whole trace shares one label.
                upper = !curveMetadata->at(0).x();
            }
            if(firstSpecification)
            {
                //La primera especificación se une en intersección
                for(qint32 j=0;j<(qint32)specificationTraces->size();j++)
                {
                    *unionPoints += *specificationTraces->at(j);

                }
                firstSpecification = false;
                continue;
            }

            //Every further specification is flattened into one curve.
            QVector <QPointF> * currentCurve = new QVector <QPointF>();
            for(qint32 j=0;j<(qint32)specificationTraces->size();j++)
            {
                *currentCurve += *specificationTraces->at(j);
            }


            if(unionPoints->size()>=totalPhase) open1 = true;
            if(currentCurve->size()>=totalPhase) open2 = true;

            QVector< QVector<QPointF> * > * chosenCurves = new QVector< QVector<QPointF> * >();
            chosenCurves->append(unionPoints);
            chosenCurves->append(currentCurve);
            QVector< QVector< QVector<QPointF> * > * > * layerBuckets = buildLayerBuckets(chosenCurves, totalPhase, open1||open2, upper);

            QVector<QPointF> * layer1 = drawFirstLayer(chosenCurves,layerBuckets,totalPhase,open1,open2);
            QVector<QPointF> * layer2 = drawSecondLayer(chosenCurves,layerBuckets,totalPhase,open1,open2);

            //The running union and the freshly merged curve now live in the
            //two layers: free them along with the scratch buckets (each
            //extra specification used to abandon them whole).
            delete unionPoints;
            unionPoints = mergeLayers(layer1, layer2);
            delete layer1;
            delete layer2;
            delete currentCurve;

            foreach (QVector <QVector <QPointF> * > * layer, *layerBuckets){
                foreach (QVector <QPointF> * bucket, *layer){
                    delete bucket;
                }
                delete layer;
            }
            delete layerBuckets;

            if((open1||open2)&&!m_openFlags->at(i))
            {
                m_openFlags->replace(i, open1||open2);
                m_upperFlags->replace(i, upper);
            }
            delete chosenCurves;
        }

        m_unionVectors->append(unionPoints);



        QVector< QVector<QPointF> * > * unionBucketsRow = buildUnionBuckets(unionPoints, totalPhase, phasePointCount);

        m_unionBuckets->append(unionBucketsRow);
    }





    QVector<QVector<QPointF> *> * dos = new QVector<QVector<QPointF> *> ();

    qint32 c = 0;
    foreach (QVector<QPointF> * b, *m_unionVectors) {
        if (m_openFlags->at(c)){
            dos->append(sortByProximity(b));
            delete b;
        } else {
            dos->append(b);
        }
        c++;
    }

    delete m_unionVectors;
    m_unionVectors = dos;
}

QVector<QVector<QVector<QPointF> *> *> *BoundaryUnion1D::unionBuckets()
{
    return m_unionBuckets;
}

QVector<QVector<QPointF> *> * BoundaryUnion1D::unionVectors()
{
    return m_unionVectors;
}

QVector <bool> * BoundaryUnion1D::openFlags()
{
    return m_openFlags;
}

QVector <bool> * BoundaryUnion1D::upperFlags()
{
    return m_upperFlags;
}


QVector<QPointF> * BoundaryUnion1D::sortByProximity(QVector<QPointF> * array) {

    QVector<QPointF> * v = new QVector<QPointF> ();
    v->reserve(array->size());

    QPointF tmp = QPointF(10000, 1);

    foreach (QPointF p, *array) {
        if (p.x() < tmp.x()){
            tmp = QPointF (p);
        }
    }

    v->append(tmp);

    array->removeOne(tmp);
    qint32 lon = array->size();

    qreal dis, dis2;
    for (qint32 i = 0; i < lon; i++){
        QPointF uno(tmp);
        dis = 10000;
        for (qint32 j = 0; j < array->size(); j++){
            QPointF dos = array->at(j);
            dis2 = sqrt(pow(uno.x() - dos.x(), 2) + pow(uno.y() - dos.y(), 2));
            if (dis2 < dis){
                dis = dis2;
                tmp = QPointF (dos);
            }
        }

        v->append(tmp);
        array->removeOne(tmp);
    }
    array->clear();
    return v;
}

} // namespace qftbx
