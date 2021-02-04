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

datos_caja DeteccionViolacionBoundaries::deteccionViolacionCajaNyNi(cinterval box, DatosBound * boundaries, qint32 contador)
{

    QVector< QVector<QPointF> * > * interseccionHash = boundaries->getBoundariesReunHash()->at(contador);
    qreal totalFase = boundaries->getTamFas() - 1;
    QPointF fases = boundaries->getDatosFasBoundLin()->at(contador);
    QPointF magitudes = boundaries->getDatosMagBoundLin()->at(contador);

    if ( InfRe(box) < fases.x() && SupRe(box) > fases.y() && InfIm(box) < magitudes.x() && SupIm(box) > magitudes.y()){
        struct datos_caja datos;
        datos.minimosMaximos = new QVector <qreal> ();

        datos.flag = ambiguous;

        datos.minimosMaximos->append(pow(10, boundaries->getDatosMagBound()->at(contador).x() / 20));
        datos.minimosMaximos->append(pow(10, boundaries->getDatosMagBound()->at(contador).y() / 20));
        datos.minimosMaximos->append(boundaries->getDatosFasBound()->at(contador).x() * PI / 180);
        datos.minimosMaximos->append(boundaries->getDatosFasBound()->at(contador).y() * PI / 180);

        datos.completo = true;

        datos.recAbajo = true;
        datos.uniAbajo = true;
        datos.recArriba = true;
        datos.uniArriba = true;
        datos.recIzquierda = true;
        datos.uniIzquierda = true;
        datos.recDerecha = true;
        datos.uniDerecha = true;

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

    struct datos_caja datos;

    datos.minimosMaximos = new QVector <qreal> ();

    datos.minimosMaximos->append(pow(10, minMagCaja / 20));
    datos.minimosMaximos->append(pow(10, maxMagCaja / 20));
    datos.minimosMaximos->append(minFasCaja * PI / 180);
    datos.minimosMaximos->append(maxFasCaja * PI / 180);

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
        datos.recAbajo = true;
        datos.uniAbajo = f1 != infeasible;
    }

    if (maxMagCaja < (Sup(mag) - 1)){
        datos.recArriba = true;
        datos.uniArriba = f2 != infeasible;
    }

    if (minFasCaja > (Inf(theta) + 1)){
        datos.recIzquierda = true;
        datos.uniIzquierda = f1 != infeasible;
    }

    if (maxFasCaja < (Sup(theta) - 1)){
        datos.recDerecha = true;
        datos.uniDerecha = f2 != infeasible;
    }



    if (ambiguo) {
        datos.flag = ambiguous;
    } else {
        datos.flag = f1;
    }

    datos.completo = false;


    return datos;
}


datos_caja DeteccionViolacionBoundaries::deteccionViolacionCajaNiNi(cinterval box, DatosBound *boundaries, qint32 contador) {


    QVector< QVector<QPointF> * > * interseccionHash = boundaries->getBoundariesReunHash()->at(contador);
    qreal totalFase = boundaries->getTamFas() - 1;
    bool abierta = boundaries->getMetaDatosAbierta()->at(contador);
    bool arriba = boundaries->getMetaDatosArriba()->at(contador);
    QPointF fases = boundaries->getDatosFasBound()->at(contador);
    QPointF magitudes = boundaries->getDatosMagBound()->at(contador);

    if ( InfRe(box) < magitudes.x() && SupRe(box) > magitudes.y() && InfIm(box) < fases.x() && SupIm(box) > fases.y()){
        struct datos_caja datos;
        datos.minimosMaximos = new QVector <qreal> ();

        datos.flag = ambiguous;

        datos.minimosMaximos->append(pow(10, boundaries->getDatosMagBound()->at(contador).x() / 20));
        datos.minimosMaximos->append(pow(10, boundaries->getDatosMagBound()->at(contador).y() / 20));
        datos.minimosMaximos->append(boundaries->getDatosFasBound()->at(contador).x() * PI / 180);
        datos.minimosMaximos->append(boundaries->getDatosFasBound()->at(contador).y() * PI / 180);

        datos.completo = true;

        datos.recAbajo = true;
        datos.uniAbajo = false;
        datos.recArriba = true;
        datos.uniArriba = false;
        datos.recIzquierda = true;
        datos.uniIzquierda = false;
        datos.recDerecha = true;
        datos.uniDerecha = false;

        return datos;
    }


    qreal minFasCaja = std::numeric_limits<qreal>::max(), maxFasCaja = std::numeric_limits<qreal>::lowest(),
            minMagCaja = std::numeric_limits<qreal>::max(), maxMagCaja = std::numeric_limits<qreal>::lowest();

    bool ambiguo = false;

    qreal numeroFases = boundaries->getDatosFas().y() - boundaries->getDatosFas().x();

    qreal salto = numeroFases / totalFase;


    qreal minFas = _double(InfIm(box)), maxFas = _double(SupIm(box)), minMag = _double(InfRe(box)), maxMag = _double(SupRe(box));

    for (qreal f = minFas; f <= maxFas; f += salto) {

        foreach(auto puntoDecibelios, *interseccionHash->value(funcionHash(f, totalFase, numeroFases))) {

            if (puntoDecibelios.y() >= minMag && puntoDecibelios.y() <= maxMag && puntoDecibelios.x() >= minFas && puntoDecibelios.x() <= maxFas) {

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



    struct datos_caja datos;

    datos.minimosMaximos = new QVector <qreal> ();

    datos.minimosMaximos->append(pow(10, minMagCaja / 20));
    datos.minimosMaximos->append(pow(10, maxMagCaja / 20));
    datos.minimosMaximos->append(minFasCaja * PI / 180);
    datos.minimosMaximos->append(maxFasCaja * PI / 180);

    flags_box f = deteccionViolacion(QPointF(minFas, minMag), interseccionHash, totalFase,
                                     abierta, arriba, numeroFases);

    flags_box f1 = deteccionViolacion(QPointF(maxFas, maxMag), interseccionHash, totalFase,
                                      abierta, arriba, numeroFases);

    if (minMagCaja > minMag + 1){
        datos.recAbajo = true;
        datos.uniAbajo = f != infeasible;
    }

    if (maxMagCaja < maxMag - 1){
        datos.recArriba = true;
        datos.uniArriba = f1 != infeasible;
    }

    if (minFasCaja > minFas + 1){
        datos.recIzquierda = true;
        datos.uniIzquierda = f != infeasible;
    }

    if (maxFasCaja < maxFas - 1){
        datos.recDerecha = true;
        datos.uniDerecha = f1 != infeasible;
    }


    if (ambiguo) {
        datos.flag = ambiguous;
    } else {
        datos.flag = f;
    }

    datos.completo = false;

    return datos;
}

datos_caja DeteccionViolacionBoundaries::deteccionViolacionCajaNi(cinterval box, DatosBound *boundaries, qint32 contador) {

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



    struct datos_caja datos;

    datos.minimosMaximos = new QVector <qreal> ();

    datos.minimosMaximos->append(minMagCaja);
    datos.minimosMaximos->append(maxMagCaja);
    datos.minimosMaximos->append(minFasCaja);
    datos.minimosMaximos->append(maxFasCaja);

    flags_box f = deteccionViolacion(QPointF(minFas, minMag), interseccionHash, totalFase,
                                     abierta, arriba, numeroFases);
    if (f == feasible){
        datos.uniArriba = true;
    } else {
        datos.uniArriba = false;
    }

    if (ambiguo) {
        datos.flag = ambiguous;
    } else {
        datos.flag = f;
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
