#include "src/core/loopshaping/boundary_violation_detector.h"

using namespace tools;
using namespace cxsc;

BoundaryViolationDetector::BoundaryViolationDetector() {
}

BoundaryViolationDetector::~BoundaryViolationDetector() {
}

inline qint32 BoundaryViolationDetector::phaseBucket(qreal x, qreal totalFase, qint32 numeroFases)
{
    qreal res = (std::abs(x)*((qreal) totalFase / numeroFases));
    if(res<0) res=0;
    return (qint32) res;
}

inline BoxFlag BoundaryViolationDetector::pointVerdict(QPointF punto, QVector< QVector<QPointF> * > * interseccionHash, qint32 totalFase, bool abierta, bool arriba, qint32 numeroFases) {
    bool violacion = false;

    qint32 contArriba = 0, contAbajo = 0;
    QVector<QPointF> * puntosHash = interseccionHash->at(phaseBucket(punto.x(), totalFase, numeroFases));
    qint32 tamCubeta = puntosHash->size();

    for (qint32 j = 0; j < tamCubeta; j++) {
        QPointF puntoH = puntosHash->at(j);
        if (punto.y() > puntoH.y()){
            contArriba++;
        } else {
            contAbajo++;
        }
    }

    //Open boundary: 'arriba' comes from the union metadata and means the
    //ALLOWED side is above. An even number of boundary layers below the
    //point leaves it UNDER the union, an odd number over it. The
    //historical branch had the two verdicts swapped, so every open
    //boundary (tracking, disturbance rejection) accepted exactly the
    //loops that violated it and rejected the compliant ones.
    if (abierta) {
        if (contArriba % 2 == 0){
            violacion = arriba;   //under the union
        } else {
            violacion = !arriba;  //over it
        }
    } else {
        if (contArriba % 2 == 0){
            if (arriba){
                violacion = true;
            } else {
                violacion = false;
            }
        } else {
            if (arriba){
                violacion = false;
            } else {
                violacion = true;
            }
        }
    }


    if (violacion) return infeasible;

    return feasible;
}

//Feasibility of a Nichols box against the boundary union at one design
//frequency (Tharewal 2005, sec. 3.3.4): feasible when the box lies
//entirely on the allowed side, infeasible when entirely on the forbidden
//side, ambiguous when boundary points fall inside it. The returned
//minimums/maximums are B_min and B_max, the extreme boundary magnitudes
//over the box's PHASE interval (Tharewal 2005, fig. 5.1), in dB/degrees,
//which drive the gain cutting, plus the boundary's phase extremes over
//the same span, which drive the phase cutting of algorithm MC. The
//historical version computed B_min and B_max only from the boundary
//points INSIDE the box: when the boundary left the box within its phase
//span the cut could remove feasible gains.
BoxClassification *BoundaryViolationDetector::classifyBox(cinterval box, BoundaryData *boundaries, qint32 contador) {

    QVector< QVector<QPointF> * > * interseccionHash = boundaries->unionBuckets()->at(contador);
    qint32 totalFase = boundaries->phaseCount() - 1;
    bool abierta = boundaries->openFlags()->at(contador);
    bool arriba = boundaries->upperFlags()->at(contador);


    qreal minFasBound = std::numeric_limits<qreal>::max(), maxFasBound = std::numeric_limits<qreal>::lowest(),
            minMagBound = std::numeric_limits<qreal>::max(), maxMagBound = std::numeric_limits<qreal>::lowest();

    bool ambiguo = false;

    qreal numeroFases = boundaries->phaseRange().y() - boundaries->phaseRange().x();

    //Degrees per bucket of the phase-bucketed union (the historical
    //formula was inverted, which only worked on the standard 1-degree grid).
    qreal salto = numeroFases / totalFase;


    qreal minFas = _double(InfIm(box)), maxFas = _double(SupIm(box)), minMag = _double(InfRe(box)), maxMag = _double(SupRe(box));

    for (qreal f = minFas; f <= maxFas + salto; f += salto) {

        foreach(auto puntoDecibelios, *interseccionHash->value(phaseBucket(std::min(f, maxFas), totalFase, numeroFases))) {

            //Only boundary points within the box's phase span take part.
            if (puntoDecibelios.x() < minFas || puntoDecibelios.x() > maxFas) {
                continue;
            }

            //B_min / B_max over the phase interval, regardless of the
            //box's magnitude range.
            if (puntoDecibelios.x() > maxFasBound) {
                maxFasBound = puntoDecibelios.x();
            }

            if (puntoDecibelios.x() < minFasBound) {
                minFasBound = puntoDecibelios.x();
            }

            if (puntoDecibelios.y() > maxMagBound) {
                maxMagBound = puntoDecibelios.y();
            }

            if (puntoDecibelios.y() < minMagBound) {
                minMagBound = puntoDecibelios.y();
            }

            //A boundary point inside the box makes it ambiguous.
            if (puntoDecibelios.y() >= minMag && puntoDecibelios.y() <= maxMag) {
                ambiguo = true;
            }
        }
    }



    BoxClassification * datos = new BoxClassification();

    QVector <qreal> * m_extremes = new QVector <qreal> ();


    m_extremes->append(minMagBound);
    m_extremes->append(maxMagBound);
    m_extremes->append(minFasBound);
    m_extremes->append(maxFasBound);

    datos->setExtremes(m_extremes);

    //Corner classifications for the cutting strips: every boundary point
    //whose phase lies in the box's span was scanned above, so the box
    //regions below/left of (B_min, phase_min) and above/right of
    //(B_max, phase_max) are boundary-free and uniformly classified by the
    //corner they contain. The bottom-left corner drives the gain cutting
    //(NT/NK/MC/thesis-MC, bottom and left strips); the top-right corner
    //drives the top and right strips (MC/thesis-MC).
    BoxFlag f = pointVerdict(QPointF(minFas, minMag), interseccionHash, totalFase,
                                     abierta, arriba, numeroFases);
    datos->setBottomLeftForbidden(f == infeasible);

    BoxFlag f2 = pointVerdict(QPointF(maxFas, maxMag), interseccionHash, totalFase,
                                      abierta, arriba, numeroFases);
    datos->setTopRightForbidden(f2 == infeasible);

    if (ambiguo) {
        datos->setFlag(ambiguous);
    } else {
        datos->setFlag(f);
    }

    return datos;
}

//Classification of a single Nichols point (phase in degrees, magnitude in
//dB) against the boundary union at one design frequency, with the same
//parity test the box classification uses. It certifies the zone gates of
//the gain cutting and splitting (Tharewal 2005, ch. 5).
tools::BoxFlag BoundaryViolationDetector::classifyPoint(QPointF punto, BoundaryData * boundaries, qint32 contador) {

    QVector< QVector<QPointF> * > * interseccionHash = boundaries->unionBuckets()->at(contador);
    qint32 totalFase = boundaries->phaseCount() - 1;
    bool abierta = boundaries->openFlags()->at(contador);
    bool arriba = boundaries->upperFlags()->at(contador);
    qreal numeroFases = boundaries->phaseRange().y() - boundaries->phaseRange().x();

    return pointVerdict(punto, interseccionHash, totalFase, abierta, arriba, numeroFases);
}
