#include "deteccionviolacionboundaries.h"

using namespace tools;
using namespace cxsc;

#define PI 3.1415926535897936

DeteccionViolacionBoundaries::DeteccionViolacionBoundaries() {
}

DeteccionViolacionBoundaries::~DeteccionViolacionBoundaries() {
}

inline qint32 DeteccionViolacionBoundaries::funcionHash(qreal x, qreal totalFase, qint32 numeroFases)
{
    qreal res = (std::abs(x)*((qreal) totalFase / numeroFases));
    if(res<0) res=0;
    return (qint32) res;
}

inline qint32 DeteccionViolacionBoundaries::funcionHashNy(qreal x, qreal totalFase, qint32 numeroFases, qreal minFas)
{
    qreal res = (x + std::abs(minFas)*(totalFase/numeroFases));
    if(res<0) res=0;
    if (res >= totalFase) res = totalFase - 1;
    return (qint32) res;
}


inline flags_box DeteccionViolacionBoundaries::deteccionViolacion(QPointF punto, QVector< QVector<QPointF> * > * interseccionHash, qint32 totalFase, bool abierta, bool arriba, qint32 numeroFases) {
    bool violacion = false;

    qint32 contArriba = 0, contAbajo = 0;
    QVector<QPointF> * puntosHash = interseccionHash->at(funcionHash(punto.x(), totalFase, numeroFases));
    qint32 tamCubeta = puntosHash->size();

    for (qint32 j = 0; j < tamCubeta; j++) {
        QPointF puntoH = puntosHash->at(j);
        if (punto.y() > puntoH.y()){
            contArriba++;
        } else {
            contAbajo++;
        }
    }

    if (abierta) {
        if (contArriba % 2 == 0){
            if (arriba) {
                violacion = false;
            } else {
                violacion = true;
            }
        } else {
            if (arriba) {
                violacion = true;
            } else {
                violacion = false;
            }
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

inline qreal DeteccionViolacionBoundaries::_arg(std::complex <qreal> c){

    qreal a =  std::arg(c);

    if (a > 0){
        a -= 2 * PI;
    }

    return a;
}

inline interval DeteccionViolacionBoundaries::_arg(cinterval z)
{

    qreal
            r0 = _double(InfRe(z)),
            r1 = _double(SupRe(z)),

            i0 = _double(InfIm(z)),
            i1 = _double(SupIm(z));


    qreal dospi = 2 * M_PI;

    if (r0 >= 0 && r1 >= 0 && i0 >= 0 && i1 >= 0){ //1
        return interval  (std::atan2(i0,r1) - dospi, std::atan2(i1,r0) - dospi);
    } else if (r0 >= 0 && r1 >= 0 && i0 <= 0 && i1 >= 0){ // 2
        return interval  (std::atan2(i1, r0) - dospi, std::atan2(i0, r0));
    } else if (i0 <= 0 && i1 <= 0 && r0 >= 0 && r1 >= 0){ //3
        return interval  (std::atan2(i0,r0), std::atan2(i1,r1));
    } else if (i0 <= 0 && i1 <= 0 && r0 <= 0 && r1 >= 0){ // 4
        return interval  (std::atan2(i1, r0), std::atan2(i1, r1));
    } else if (i0 <= 0 && i1 <= 0 && r0 <= 0 && r1 <= 0){ //5
        return interval  (std::atan2(i1,r0), std::atan2(i0,r1));
    } else if(r0 <= 0 && r1 <= 0 && i0 <= 0 && i1 >= 0){ // 6
        return interval  (std::atan2(i1,r1) - dospi, std::atan2(i0, r1));
    } else if (r0 <= 0 && r1 <= 0 && i0 >= 0 && i1 >= 0){ //7
        return interval  (std::atan2(i1,r1) - dospi, std::atan2(i0,r0) - dospi);
    } else if(i0 >= 0 && i1 >= 0 && r0 <= 0 && r1 >= 0){ // 8
        return interval  (std::atan2(i0,r1) - dospi, std::atan2(i0,r0) - dospi);
    } else{
        return interval  (-dospi, 0);
    }

}


inline bool DeteccionViolacionBoundaries::seg_intersection(std::complex <qreal> u1, std::complex <qreal> u2,
                                                           std::complex <qreal> v1, std::complex <qreal> v2)
{
    if (side_p_to_seg(u1,u2,v1) == 2 ||
            side_p_to_seg(u1,u2,v2) == 2 ||
            side_p_to_seg(v1,v2,u1) == 2 ||
            side_p_to_seg(v1,v2,u2) == 2){
        return false;
    } else if (((side_p_to_seg(u1,u2,v1) == 0 &&
                 side_p_to_seg(u1,u2,v2) == 1) ||
                (side_p_to_seg(u1,u2,v1) == 1 &&
                 side_p_to_seg(u1,u2,v2) == 0))&&
               ((side_p_to_seg(v1,v2,u1) == 1 &&
                 side_p_to_seg(v1,v2,u2) == 0) ||
                (side_p_to_seg(v1,v2,u1) == 0 &&
                 side_p_to_seg(v1,v2,u2) == 1))){
        return true;
    } else{
        return false;
    }
}

inline qint32 DeteccionViolacionBoundaries::side_p_to_seg(std::complex<qreal> v1, std::complex<qreal> v2,
                                                          std::complex <qreal> p){
    qreal area = (v2.real()-v1.real())*(p.imag()-v1.imag())-(p.real()-v1.real())*(v2.imag()-v1.imag());
    qint32 lado;
    if (area > 0) {
        lado = 0; //izq
    } else if (area < 0) {
        lado = 1; //der
    } else{
        lado = 2; //col
    }
    return lado;
}

data_box * DeteccionViolacionBoundaries::deteccionViolacionCajaNyNi(cinterval box, DatosBound * boundaries, qint32 contador)
{

    QVector< QVector<QPointF> * > * interseccionHash = boundaries->getBoundariesReunHash()->at(contador);
    qreal totalFase = boundaries->getTamFas() - 1;
    QPointF fases = boundaries->getDatosFasBoundLin()->at(contador);
    QPointF magitudes = boundaries->getDatosMagBoundLin()->at(contador);

    if ( InfRe(box) < fases.x() && SupRe(box) > fases.y() && InfIm(box) < magitudes.x() && SupIm(box) > magitudes.y()){


        data_box * datos = new data_box();

        QVector <qreal> * minimosMaximos = new QVector <qreal> ();


        datos->setFlag(ambiguous);

        minimosMaximos->append(pow(10, boundaries->getDatosMagBound()->at(contador).x() / 20));
        minimosMaximos->append(pow(10, boundaries->getDatosMagBound()->at(contador).y() / 20));
        minimosMaximos->append(boundaries->getDatosFasBound()->at(contador).x() * PI / 180);
        minimosMaximos->append(boundaries->getDatosFasBound()->at(contador).y() * PI / 180);

        datos->setMinimoxMaximos(minimosMaximos);

        datos->setCompleto(true);

        datos->setRecAbajo(true);
        datos->setRecArriba(true);
        datos->setRecDerecha(true);
        datos->setRecIzquierda(true);
        datos->setUniAbajo(true);
        datos->setUniArriba(true);
        datos->setUniDerecha(true);
        datos->setUniIzquierda(true);

        return datos;
    }


    bool ambiguo = false;

    qreal minMagCaja = std::numeric_limits<qreal>::max(), maxMagCaja = std::numeric_limits<qreal>:: lowest(),
            minFasCaja = std::numeric_limits<qreal>::max(), maxFasCaja = std::numeric_limits<qreal>::lowest();


    qreal numeroFases = fases.y() - fases.x();

    qreal salto = numeroFases / totalFase;

    interval theta = Im(boundaries->getBox());

    interval mag = Re(boundaries->getBox());

    qreal f = _double(InfRe(box));

    if (InfRe(box) < fases.x()){
        f =  fases.x();
    }

    qreal maxF = (_double(SupRe(box)) > fases.y() ? fases.y() : _double(SupRe(box)));

    //Se pasa la magnitud a db
    for (; f <= maxF; f += salto){

        qint32 a = funcionHashNy(f, totalFase, numeroFases, fases.x());

        QVector<QPointF> * vec = interseccionHash->at(a);

        foreach (QPointF pLineal, *vec) {

            std::complex <qreal> puntoLineal (pLineal.x(), pLineal.y());

            //Se pasa la magnitud a db
            QPointF puntoDecibelios (_arg(puntoLineal) * 180.0 / PI, 20 * log10 (abs(puntoLineal)));


            if ((puntoLineal.imag() >= InfIm(box) && puntoLineal.imag() <= SupIm(box) && puntoLineal.real() >= InfRe(box) && puntoLineal.real() <= SupRe(box))){

                ambiguo = true;

                if (puntoDecibelios.y() > maxMagCaja){
                    maxMagCaja = puntoDecibelios.y();
                }

                if (puntoDecibelios.y() < minMagCaja){
                    minMagCaja = puntoDecibelios.y();
                }

                if (puntoDecibelios.x() > maxFasCaja){
                    maxFasCaja = puntoDecibelios.x();
                }

                if (puntoDecibelios.x() < minFasCaja){
                    minFasCaja = puntoDecibelios.x();
                }
            }
        }
    }

    data_box * datos = new data_box();


    QVector <qreal> * minimosMaximos = new QVector <qreal> ();

    minimosMaximos->append(pow(10, minMagCaja / 20));
    minimosMaximos->append(pow(10, maxMagCaja / 20));
    minimosMaximos->append(minFasCaja * PI / 180);
    minimosMaximos->append(maxFasCaja * PI / 180);

    datos->setMinimoxMaximos(minimosMaximos);

    flags_box f1, f2;

    ////////////////////////////
    QVector<QPointF> * puntosHash = interseccionHash->at(funcionHashNy(_double(InfRe(box)), totalFase, numeroFases, fases.x()));
    qint32 tamCubeta = puntosHash->size();
    qint32 contArriba = 0, contAbajo = 0;

    for (qint32 j = 0; j < tamCubeta; j++) {
        QPointF puntoH = puntosHash->at(j);
        if (InfIm(box) > puntoH.y()){
            contAbajo++;
        } else {
            contArriba++;
        }
    }

    if (contAbajo % 2 != 0 && contArriba % 2 != 0){
        f1 = infeasible;
    } else if (contAbajo % 2 != 0 || contArriba % 2 != 0) {
        f1 = feasible;
    } else {
        f1 = feasible;
    }
    //////////////////////////////////

    puntosHash = interseccionHash->at(funcionHashNy(_double(SupRe(box)), totalFase, numeroFases, fases.x()));
    tamCubeta = puntosHash->size();
    contArriba = 0; contAbajo = 0;

    for (qint32 j = 0; j < tamCubeta; j++) {
        QPointF puntoH = puntosHash->at(j);
        if (SupIm(box) > puntoH.y()){
            contAbajo++;
        } else {
            contArriba++;
        }
    }

    if (contAbajo % 2 != 0 && contArriba % 2 != 0){
        f2 = infeasible;
    } else if (contAbajo % 2 != 0 || contArriba % 2 != 0) {
        f2 = feasible;
    } else {
        f2 = feasible;
    }

    ////////////////////////////////////////

    if (minMagCaja > (Inf(mag) + 1)){
        datos->setRecAbajo(true);
        datos->setUniAbajo(f1 != infeasible);
    }

    if (maxMagCaja < (Sup(mag) - 1)){        
        datos->setRecArriba(true);
        datos->setUniArriba(f2 != infeasible);
    }

    if (minFasCaja > (Inf(theta) + 1)){
        datos->setRecIzquierda(true);
        datos->setUniIzquierda(f1 != infeasible);
    }

    if (maxFasCaja < (Sup(theta) - 1)){
        datos->setRecDerecha(true);
        datos->setUniDerecha(f2 != infeasible);
    }



    if (ambiguo) {
        datos->setFlag(ambiguous);
    } else {
        datos->setFlag(f1);
    }

    datos->setCompleto(false);

    return datos;
}

data_box * DeteccionViolacionBoundaries::deteccionViolacionCajaNiNi(cinterval box, DatosBound *boundaries, qint32 contador, Etapas e) {


    if (e == Etapas::INICIAL &&  (InfIm(box) > boundaries->getDatosFas().x() || SupIm(box) < boundaries->getDatosFas().y())) {
       cambioEtapas = true;
    }

    return deteccionViolacionCajaNiNi(box, boundaries, contador);
}



data_box * DeteccionViolacionBoundaries::deteccionViolacionCajaNiNi(cinterval box, DatosBound *boundaries, qint32 contador) {

    c++;

    /*std::cout << "Vueltas: " << c << std::endl;

    if (c == 138) {
        qint32 a = 10;
    }*/

    QVector< QVector<QPointF> * > * interseccionHash = boundaries->getBoundariesReunHash()->at(contador);
    qreal totalFase = boundaries->getTamFas() - 1;
    bool abierta = boundaries->getMetaDatosAbierta()->at(contador);
    bool arriba = boundaries->getMetaDatosArriba()->at(contador);

    bool ambiguo = false;

    qreal numeroFases = boundaries->getDatosFas().y() - boundaries->getDatosFas().x();

    qreal salto = numeroFases / totalFase;


    qreal minFasCaja = _double(InfIm(box)), maxFasCaja = _double(SupIm(box)), minMagCaja = _double(InfRe(box)), maxMagCaja = _double(SupRe(box));



    qreal minFasBound = std::numeric_limits<qreal>::max(), maxFasBound = std::numeric_limits<qreal>::lowest(),
            minMagBound = std::numeric_limits<qreal>::max(), maxMagBound = std::numeric_limits<qreal>::lowest();

    for (qreal f = minFasCaja; f <= maxFasCaja; f += salto) {

        foreach(auto puntoDecibelios, *interseccionHash->value(funcionHash(f, totalFase, numeroFases))) {

            if (puntoDecibelios.y() >= minMagCaja && puntoDecibelios.y() <= maxMagCaja && puntoDecibelios.x() >= minFasCaja && puntoDecibelios.x() <= maxFasCaja) {

                ambiguo = true;

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
            }
        }
    }



    data_box * datos = new data_box();

    datos->setCambioEtapa(cambioEtapas);
    cambioEtapas = false;


    QVector <qreal> * minimosMaximos = new QVector <qreal> ();

    minimosMaximos->append(pow(10, minMagBound / 20));
    minimosMaximos->append(pow(10, maxMagBound / 20));
    minimosMaximos->append(minFasBound * PI / 180);
    minimosMaximos->append(maxFasBound * PI / 180);

    datos->setMinimoxMaximos(minimosMaximos);

    flags_box f = deteccionViolacion(QPointF(minFasCaja, minMagCaja), interseccionHash, totalFase,
                                     abierta, arriba, numeroFases);

    flags_box f1 = deteccionViolacion(QPointF(maxFasCaja, maxMagCaja), interseccionHash, totalFase,
                                      abierta, arriba, numeroFases);

    if ((minMagBound - minMagCaja) > salto){
        datos->setRecAbajo(true);
        datos->setUniAbajo(f == infeasible);
    }

    if ((maxMagBound - maxMagCaja) < salto){
        datos->setRecArriba(true);
        datos->setUniArriba(f1 == infeasible);
    }

    if ((minFasBound - minFasCaja) > salto){
        datos->setRecIzquierda(true);
        datos->setUniIzquierda(f == infeasible);
    }

    if ((maxFasBound - maxFasCaja) < salto){
        datos->setRecDerecha(true);
        datos->setUniDerecha(f1 == infeasible);
    }


    if (ambiguo) {
        datos->setFlag(ambiguous);
    } else {
        datos->setFlag(f);
    }

    datos->setCompleto(false);

    return datos;
}

data_box *DeteccionViolacionBoundaries::deteccionViolacionCajaNi(cinterval box, DatosBound *boundaries, qint32 contador) {

    QVector< QVector<QPointF> * > * interseccionHash = boundaries->getBoundariesReunHash()->at(contador);
    qint32 totalFase = boundaries->getTamFas() - 1;
    bool abierta = boundaries->getMetaDatosAbierta()->at(contador);
    bool arriba = boundaries->getMetaDatosArriba()->at(contador);


    qreal minFasCaja = std::numeric_limits<qreal>::max(), maxFasCaja = std::numeric_limits<qreal>::lowest(),
            minMagCaja = std::numeric_limits<qreal>::max(), maxMagCaja = std::numeric_limits<qreal>::lowest();

    bool ambiguo = false;

    qint32 numeroFases = boundaries->getDatosFas().y() - boundaries->getDatosFas().x();

    qreal salto = totalFase / numeroFases;


    qreal minFas = _double(InfIm(box)), maxFas = _double(SupIm(box)), minMag = _double(InfRe(box)), maxMag = _double(SupRe(box));

    for (qreal f = minFas; f <= maxFas; f += salto) {

        foreach(auto puntoDecibelios, *interseccionHash->value(funcionHash(f, totalFase, numeroFases))) {

            if (puntoDecibelios.y() >= minMag && puntoDecibelios.y() <= maxMag) {

                ambiguo = true;

                if (puntoDecibelios.x() > maxFasCaja) {
                    maxFasCaja = puntoDecibelios.x();
                }

                if (puntoDecibelios.x() < minFasCaja) {
                    minFasCaja = puntoDecibelios.x();
                }

                if (puntoDecibelios.y() > maxMagCaja) {
                    maxMagCaja = puntoDecibelios.y();
                }

                if (puntoDecibelios.y() < minMagCaja) {
                    minMagCaja = puntoDecibelios.y();
                }
            }
        }
    }



    data_box * datos = new data_box();

    QVector <qreal> * minimosMaximos = new QVector <qreal> ();


    minimosMaximos->append(minMagCaja);
    minimosMaximos->append(maxMagCaja);
    minimosMaximos->append(minFasCaja);
    minimosMaximos->append(maxFasCaja);

    datos->setMinimoxMaximos(minimosMaximos);

    flags_box f = deteccionViolacion(QPointF(minFas, minMag), interseccionHash, totalFase,
                                     abierta, arriba, numeroFases);
    if (f == feasible){
        datos->setUniArriba(true);
    } else {
        datos->setUniArriba(false);
    }

    if (ambiguo) {
        datos->setFlag(ambiguous);
    } else {
        datos->setFlag(f);
    }

    return datos;
}

inline qint32 DeteccionViolacionBoundaries::interseccionCajaCirculo(cinterval box, qreal radioMayor,
                                                                    qreal radioMenor, QPointF centroCirculo) {

    qreal width = _double(diam(Re(box)));
    qreal height = _double(diam(Im(box)));

    qreal w = width / 2, h = height / 2;

    qreal xCentroBox = _double(InfRe(box)) + w;
    qreal yCentroBox = _double(SupIm(box)) - h;

    qreal circleDistanceX = std::abs(centroCirculo.x() - xCentroBox);
    qreal circleDistanceY = std::abs(centroCirculo.y() - yCentroBox);

    if ((circleDistanceX + w > radioMayor) && (circleDistanceX - w > radioMayor) &&
            (circleDistanceY + h > radioMayor) && (circleDistanceY - h) > radioMayor) {
        return 2;
    }

    if (circleDistanceX > (w + radioMayor)) {
        return 0;
    }
    if (circleDistanceY > (h + radioMayor)) {
        return 0;
    }

    if (circleDistanceX < (w - radioMenor)) {
        return 1;
    }
    if (circleDistanceY < (h - radioMenor)) {
        return 1;
    }

    if (pow(circleDistanceX - w, 2) + pow(circleDistanceY - h, 2) > (pow(radioMayor, 2))) { // fuera
        return 0;
    } else if (pow(circleDistanceX + w, 2) + pow(circleDistanceY + h, 2) < (pow(radioMenor, 2))) { // dentro
        return 1;
    } else {
        return -1; // en medio
    }
}
