#include "boundaries.h"

#include <QElapsedTimer>
#include <iostream>

using namespace std;
using namespace tools;


#ifdef CUDA_AVAILABLE
//Función de CUDA que resuelve el algoritmo de la e_hull.
extern "C"
std::vector <float *> * bnd_cuda(vector<complex<double> > templates, complex <double> p0, double infinito, vector<float> fas,
                                 vector<float> mag);
#endif

Boundaries::Boundaries()
{
    boundariesCreados = false;
    boolcreados = false;
}

void Boundaries::lanzarCalculo(QVector<qreal> *omega, std::shared_ptr<Sistema> planta, QVector<QVector<complex<qreal> > *> *templates,
                               QVector<dBND *> *altura, QPointF datosFas, qint32 puntosFas, QPointF datosMag,
                               qint32 puntosMag, qreal infinito, bool cuda){


    this->altura = altura;
    this->cuda = cuda;

    tamFas = puntosFas;
    tamMag = puntosMag;
    this->datosFas = datosFas;
    this->datosMag = datosMag;

    if (boolcreados){
        boolseguimiento->clear();
        boolestabilidad->clear();
        boolRPS->clear();
        boolRPE->clear();
        boolEC->clear();
    }

    boolcreados = true;

    boolseguimiento = new QVector <bool> ();
    boolestabilidad = new QVector <bool> ();
    boolruido = new QVector <bool> ();
    boolRPS = new QVector <bool> ();
    boolRPE = new QVector <bool> ();
    boolEC = new QVector <bool> ();

    foreach(qreal o, *omega){
        boolseguimiento->append(altura->at(0)->utilizado && altura->at(0)->frecinicio <= o && altura->at(0)->frecfinal >= o);
        boolestabilidad->append(altura->at(2)->utilizado && altura->at(2)->frecinicio <= o && altura->at(2)->frecfinal >= o);
        boolruido->append(altura->at(3)->utilizado && altura->at(3)->frecinicio <= o && altura->at(3)->frecfinal >= o);
        boolRPS->append(altura->at(4)->utilizado && altura->at(4)->frecinicio <= o && altura->at(4)->frecfinal >= o);
        boolRPE->append(altura->at(5)->utilizado && altura->at(5)->frecinicio <= o && altura->at(5)->frecfinal >= o);
        boolEC->append(altura->at(6)->utilizado && altura->at(6)->frecinicio <= o && altura->at(6)->frecfinal >= o);
    }



#ifdef CUDA_AVAILABLE

    QElapsedTimer timer;
    timer.start();

    if (!cuda){

        bnd(omega, planta,templates,  datosFas,
            puntosFas,datosMag, puntosMag, infinito);

        cout << "boundarie OpenMP: " << timer.elapsed() << " milliseconds" << endl;

    } else {


        boundaries = new QVector <QMap <QString, QVector <QVector <QPointF> * > * > * >  ();
        metaDatosBoundaries = new QVector <QMap <QString, QVector <QPoint> * > * >  ();

        for (int i = 0; i < omega->size(); i++){

            std::complex <qreal> p0 = planta->getPunto(omega->at(i));
            QVector <complex <qreal> >  * p = templates->at(i);

            std::vector <float *> * vecSabanasCuda = bnd_cuda(p->toStdVector(),p0, infinito,
                                                              linspace1(datosFas.x(), datosFas.y(),puntosFas),
                                                              linspace1(datosMag.x(), datosMag.y(), puntosMag));


            //Vector donde se guardaran los boundaries
            QMap <QString, QVector <QVector <QPointF> * > *> * bound = new QMap <QString, QVector <QVector <QPointF> * > *> ();

            //Vector donde se guardaran los metadatos.
            QMap <QString, QVector <QPoint> * > * metaBound = new QMap <QString, QVector <QPoint> * > ();

            //Guardamos todas las sábanas en un vector para pasarla como parámetro
            //vecSabanas = new QVector <QVector <QVector <qreal> * > * > ();


            //vecSabanas->append(sabanaEstabilidadRuido);


            calcularContour(omega->at(i), bound, vecSabanasCuda, metaBound, p0, p, i, abs(datosFas.x()), abs(datosMag.x()) + abs (datosMag.y()), datosMag.y());

            vecSabanasCuda->clear();

            metaDatosBoundaries->append(metaBound);
            boundaries->append(bound);
        }

        boundariesCreados = true;

        nueva_omega = new QVector <qreal> (*omega);

        cout << "boundarie CUDA: " << timer.elapsed() << " milliseconds" << endl;


    }
#else
    QElapsedTimer timer;
    timer.start();

    bnd(omega, planta,templates,  datosFas, puntosFas,datosMag, puntosMag, infinito);

    cout << "boundarie OpenMP: " << timer.elapsed() << " milliseconds" << endl;
#endif

    AlgoritmoInterseccionLineal1D * interseccion = new AlgoritmoInterseccionLineal1D ();

    timer.restart();

    interseccion->ejecutarAlgoritmo(getBoundaries(), metaDatosBoundaries);

    cout << "Algoritmo Intersección 1D: " << timer.elapsed() << " milliseconds" << endl;

    boun_reunidos = interseccion->getInterseccionesVectores();
    boundariesHash = interseccion->getInterseccionesBoundaries();

    metaDatosAbierta = interseccion->getMetadatosAbierta();
    metaDatosArriba = interseccion->getMetadatosArriba();

    delete interseccion;

    //metaDatosBoundaries->clear();

    //Ejecución de Tests ¡NO BORRAR!
    //TestDeteccion * test = new TestDeteccion();
    //test->ejecutarTests(getBoundaries(), metaDatosBoundaries);

    metaDatosBoundaries->clear();

}

std::shared_ptr<DatosBound> Boundaries::getBoundaries(){
    return make_shared<DatosBound>(boundaries, metaDatosAbierta, metaDatosArriba ,tamFas, datosFas, boun_reunidos, boundariesHash
                          ,tamMag, datosMag);
}

QVector<QVector<QPointF> *> *Boundaries::getBoundariesReunidos(){
    return boun_reunidos;
}


void Boundaries::calcularContour(qreal omega, QMap <QString, QVector <QVector <QPointF> * > *> * bound,
                                 QVector <QVector <QVector <qreal> * > * > * vecSabanas,
                                 QMap <QString, QVector <QPoint> * > * metaBound,
                                 complex <qreal> p0, QVector <complex <qreal> > * p, qint32 contador,
                                 qreal nPuntosFas, qreal nPuntosMag, qreal moverMag){


    //Boundarie seguimiento
    if (boolseguimiento->at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("Seguimiento",
                      calcularContourVector(altura->at(0), vecSabanas->at(1),
                                            omega, metaBoun, p0, p, 1, nPuntosFas, nPuntosMag, moverMag,
                                            altura->at(1)));

        metaBound->insert("Seguimiento", metaBoun);
    }

    //Boundarie estabilidad
    if (boolestabilidad->at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("Estabilidad",
                      calcularContourVector(altura->at(2), vecSabanas->at(0),
                                            omega, metaBoun, p0, p, 0, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("Estabilidad", metaBoun);
    }

    //Boundarie ruido
    if (boolruido->at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();
        bound->insert("Ruido",
                      calcularContourVector(altura->at(3), vecSabanas->at(0),
                                            omega, metaBoun, p0, p, 0, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("Ruido", metaBoun);
    }

    //Boundarie RPS
    if (boolRPS->at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("RPS",
                      calcularContourVector(altura->at(4), vecSabanas->at(2),
                                            omega, metaBoun, p0, p, 2, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("RPS", metaBoun);
    }

    //Boundarie RPE
    if (boolRPE->at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("RPE",
                      calcularContourVector(altura->at(5), vecSabanas->at(3),
                                            omega, metaBoun, p0, p, 3, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("RPE", metaBoun);
    }

    //Boundarie EC
    if (boolEC->at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("EC",
                      calcularContourVector(altura->at(6), vecSabanas->at(4),
                                            omega, metaBoun, p0, p, 4, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("EC", metaBoun);
    }
}

void Boundaries::calcularContour(qreal omega, QMap <QString, QVector <QVector <QPointF> * > *> * bound,
                                 std::vector <float *> * vecSabanasCuda,
                                 QMap <QString, QVector <QPoint> * > * metaBound,
                                 complex <qreal> p0, QVector <complex <qreal> > * p, qint32 contador,
                                 qreal nPuntosFas, qreal nPuntosMag, qreal moverMag){

    //Boundarie seguimiento
    if (boolseguimiento->at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("Seguimiento",
                      calcularContourVector(altura->at(0), vecSabanasCuda->at(1),
                                            omega, metaBoun, p0, p, 1, nPuntosFas, nPuntosMag, moverMag,
                                            altura->at(1)));

        metaBound->insert("Seguimiento", metaBoun);
    }

    //Boundarie estabilidad
    if (boolestabilidad->at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("Estabilidad",
                      calcularContourVector(altura->at(2), vecSabanasCuda->at(0),
                                            omega, metaBoun, p0, p, 0, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("Estabilidad", metaBoun);
    }

    //Boundarie ruido
    if (boolruido->at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("Ruido",
                      calcularContourVector(altura->at(3), vecSabanasCuda->at(0),
                                            omega, metaBoun, p0, p, 0, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("Ruido", metaBoun);
    }

    //Boundarie RPS
    if (boolRPS->at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("RPS",
                      calcularContourVector(altura->at(4), vecSabanasCuda->at(2),
                                            omega, metaBoun, p0, p, 2, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("RPS", metaBoun);
    }

    //Boundarie RPE
    if (boolRPE->at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("RPE",
                      calcularContourVector(altura->at(5), vecSabanasCuda->at(3),
                                            omega, metaBoun, p0, p, 3, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("RPE", metaBoun);
    }

    //Boundarie EC
    if (boolEC->at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("EC",
                      calcularContourVector(altura->at(6), vecSabanasCuda->at(4),
                                            omega, metaBoun, p0, p, 4, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("EC", metaBoun);
    }

}

QVector <QMap <QString, QVector <QPoint> * > *> * Boundaries::getMetaDatosBoundaries(){
    return metaDatosBoundaries;
}

QVector<QVector<QPointF> *> * Boundaries::calcularContourVector(tools::dBND * altura, QVector<QVector<qreal> *> *sabana, qreal omega,
                                                                QVector<QPoint> *metaBoun, std::complex<qreal> p0, QVector<std::complex<qreal> > *p,
                                                                qint32 i, qreal nPuntosFas, qreal nPuntosMag,
                                                                qreal moverMag, tools::dBND * altura2)
{

    QVector<QVector<QPointF> *> *  boun;

    Contour2 * contour = new Contour2();

    contour->setDatos(altura, sabana, omega, altura2);

    //boun = contour->getContour(nPuntosFas, tamFas, nPuntosMag, tamMag, moverMag);

    boun = contour->getContour(nPuntosFas, nPuntosMag, moverMag);


#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 j = 0; j < boun->size(); j++) {
        QPoint punto;
        //Guardamos la zona de inviolabilidad del boundarie

        if (altura->nombre == "seguimiento"){
            punto.setX(getZona(boun->at(j), p0, p, i,altura2->getAltura(omega) - altura->getAltura(omega)));
        }else {
            punto.setX(getZona(boun->at(j), p0, p, i, altura->getAltura(omega)));
        }

#ifdef OpenMP_AVAILABLE
#pragma omp critical
        {
            metaBoun->append(punto);
        }
#else
        metaBoun->append(punto);
#endif
    }

    if (metaBoun->isEmpty()) {
        metaBoun->append(QPoint(0,0));
    }

    delete contour;

    return boun;
}


QVector<QVector<QPointF> *> * Boundaries::calcularContourVector(dBND *altura, float *sabana, qreal omega,
                                                                QVector<QPoint> *metaBoun, std::complex<qreal> p0,
                                                                QVector<std::complex<qreal> > *p, qint32 i,
                                                                qreal nPuntosFas, qreal nPuntosMag, qreal moverMag,
                                                                dBND *altura2){
    QVector<QVector<QPointF> *> *  boun;

    Contour2 * contour = new Contour2();

    contour->setDatos(altura, sabana, omega, altura2);

    boun = contour->getContour(nPuntosFas,tamFas, nPuntosMag, tamMag, moverMag);


#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 j = 0; j < boun->size(); j++) {
        QPoint punto;
        //Guardamos la zona de inviolabilidad del boundarie

        if (altura->nombre == "seguimiento"){
            punto.setX(getZona(boun->at(j), p0, p, i, altura2->getAltura(omega) - altura->getAltura(omega)));
        }else {
            punto.setX(getZona(boun->at(j), p0, p, i, altura->getAltura(omega)));
        }

#ifdef OpenMP_AVAILABLE
#pragma omp critical
        {
            metaBoun->append(punto);
        }
#else
        metaBoun->append(punto);
#endif
    }

    delete contour;

    return boun;
}

qint32 Boundaries::getZona(QVector<QPointF> * vec, complex <qreal> p0, QVector <complex <qreal> > * p,
                           qint32 i, qreal altura){

    //Buscamos la mayor magnitud del vector
    qreal l = -numeric_limits<qreal>::infinity();
    qreal f = -numeric_limits<qreal>::infinity();

    foreach (QPointF punto, *vec) {
        if(punto.y() > l){
            l = punto.y();
            f = punto.x();
        }
    }

    l -= 1;

    //Se pasa la magnitud a lineal
    qreal maglineal = pow(10,l/20);
    //Se calcula el número complejo correspondiente a la posición de la rejilla.
    //Se pasa de Nichols a lineal.
    complex<qreal> L = complex<qreal> (maglineal * cos (f * M_PI / 180),
                                       maglineal * sin (f * M_PI / 180));


    //Creamos las variables necesarias
    //complex <qreal> complejoMovido;
    //qreal fase;
    complex <qreal> p_actual;
    qreal dTempEstabilidadRuidoSeguimiento;
    qreal dTempRPS;
    qreal dTempRPE;
    qreal dTempEC;

    qreal dEstabilidadRuido = -numeric_limits<qreal>::infinity();
    qreal dSeguimiento = numeric_limits<qreal>::infinity();
    qreal dRPS = -numeric_limits<qreal>::infinity();
    qreal dRPE = -numeric_limits<qreal>::infinity();
    qreal dEC = -numeric_limits<qreal>::infinity();


    //qreal valorAnterior = -0.00001;
    for (qint32 h = 0; h < p->size(); h++) { // temp

        //Cálculo necesario que se guarda en el template
        p_actual = p->at(h);

        complex<qreal> aux_complex = (p0 / p_actual) + L;

        //Estabilidad y ruido del sensor
        dTempEstabilidadRuidoSeguimiento = abs (L / aux_complex);

        //Rechazo de perturbaciones a la salida de la planta
        dTempRPS = abs ((p0 / p_actual) / aux_complex);

        //Rechado de perturbaciones a la entrada de la planta
        dTempRPE = abs (p0 / aux_complex);

        //Esfuerzo de control
        dTempEC = abs ((L / p_actual) / aux_complex);

        if (dTempEstabilidadRuidoSeguimiento > dEstabilidadRuido){
            dEstabilidadRuido = dTempEstabilidadRuidoSeguimiento;
        }
        if (dTempEstabilidadRuidoSeguimiento < dSeguimiento){
            dSeguimiento = dTempEstabilidadRuidoSeguimiento;
        }
        if (dTempRPS > dRPS){
            dRPS = dTempRPS;
        }
        if (dTempRPE > dRPE){
            dRPE = dTempRPE;
        }
        if (dTempEC > dEC) {
            dEC = dTempEC;
        }
    }

    switch (i){
    case 0:
        if (dEstabilidadRuido > altura){
            return 0;
        }
        break;
    case 1:
        if ((dEstabilidadRuido - dSeguimiento) > altura){
            return 0;
        }
        break;
    case 2:
        if(dRPS > altura){
            return 0;
        }
        break;
    case 3:
        if (dRPE > altura){
            return 0;
        }
        break;
    case 4:
        if (dEC > altura){
            return 0;
        }
        break;
    default:
        return 1;
    }

    return 1;
}

void Boundaries::bnd(QVector<qreal> *omega, std::shared_ptr<Sistema> planta, QVector <QVector <complex <qreal> > *>
                     * templates, QPointF datosFas, qint32 puntosFas,
                     QPointF datosMag, qint32 puntosMag, qreal infinito)
{
    // Se genera la rejilla base del algoritmo.
    QVector <qreal> * fases = linspace(datosFas.x(), datosFas.y(), puntosFas);
    QVector <qreal> * mag = linspace(datosMag.x(), datosMag.y(), puntosMag);

    qreal inf;

    //Si el valor de infinito ha sido introducido por el usuario.
    if (infinito < 0){
        inf = numeric_limits<qreal>::infinity();
    }else {
        inf = infinito;
    }

    if (boundariesCreados){
        boundaries->clear();
    }

    QVector <qreal> * aux_omega = omega;

#ifdef OpenMP_AVAILABLE

    boundaries = new QVector <QMap <QString, QVector <QVector <QPointF> * > * > * >  ();
    metaDatosBoundaries = new QVector <QMap <QString, QVector <QPoint> * > * >  ();
    omega = new QVector <qreal> (omega->size());
#else
    boundaries = new QVector <QMap <QString, QVector <QVector <QPointF> * > * > * >  ();
    metaDatosBoundaries = new QVector <QMap <QString, QVector <QPoint> * > * >  ();
    omega = new QVector <qreal> ();
#endif



    qint32 numeroHilo = 0;

    //Se recorren las frecuencias de diseño.
#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 i = 0; i < aux_omega->size(); i++){

        calcularBndOmega(aux_omega->at(i), planta, templates->at(i), fases, mag, inf, i, numeroHilo, omega);
    }

    nueva_omega = omega;

}

QVector <qreal> * Boundaries::getOmega(){
    return nueva_omega;
}


void Boundaries::calcularBndOmega (qreal omega, std::shared_ptr<Sistema> planta,
                                   QVector<std::complex <qreal> > * temp, QVector <qreal> * fases,
                                   QVector <qreal> * mag, qreal inf __attribute__((unused)), qint32 contador, qint32 num_hilo,
                                   QVector <qreal> * nueva_omega){

    //Se crean las variables necesarias
    complex <qreal> p0;

    //Se obtiene cada plantilla.
    QVector <complex <qreal> >  * p = temp;
    //Se resuelve la planta con las frecuencias de diseño.
    p0 = planta->getPunto(omega);

    //Una sábana por cada frecuencia de diseño.
    QVector <QVector <qreal> * > * sabanaEstabilidadRuido = new QVector <QVector <qreal> * > ();
    sabanaEstabilidadRuido->reserve(fases->size());

    QVector <QVector <qreal> * > * sabanaSeguimiento = new QVector <QVector <qreal> * > ();
    sabanaSeguimiento->reserve(fases->size());

    QVector <QVector <qreal> * > * sabanaRPS = new QVector <QVector <qreal> * > ();
    sabanaRPS->reserve(fases->size());

    QVector <QVector <qreal> * > * sabanaRPE = new QVector <QVector <qreal> * > ();
    sabanaRPE->reserve(fases->size());

    QVector <QVector <qreal> * > * sabanaEC = new QVector <QVector <qreal> * > ();
    sabanaEC->reserve(fases->size());

    //Creamos las variables necesarias primer bucle:
    QVector <qreal> * vectorEstabilidadRuido;
    QVector <qreal> * vectorSeguimiento;
    QVector <qreal> * vectorRPS;
    QVector <qreal> * vectorRPE;
    QVector <qreal> * vectorEC;

    //Variables segundo bucle:
    qreal l;
    qreal f;
    qreal maglineal;
    complex <qreal> L;
    //qreal valorAnterior;
    qreal dEstabilidadRuido;
    qreal dRPS;
    qreal dRPE;
    qreal dEC;
    qreal dSeguimiento;

    //Variables tercer bucle:
    //complex <qreal> complejoMovido;
    //qreal fase;
    complex <qreal> p_actual;
    qreal dTempEstabilidadRuidoSeguimiento;
    qreal dTempRPS;
    qreal dTempRPE;
    qreal dTempEC;
    complex<qreal> aux_complex;

    //Se recorre la rejilla
#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 k = 0; k < mag->size(); k++){ // f

        vectorEstabilidadRuido = new QVector <qreal> ();
        vectorEstabilidadRuido->reserve(fases->size());

        vectorSeguimiento = new QVector <qreal> ();
        vectorSeguimiento->reserve(fases->size());

        vectorRPS = new QVector <qreal> ();
        vectorRPS->reserve(fases->size());

        vectorRPE = new QVector <qreal> ();
        vectorRPE->reserve(fases->size());

        vectorEC = new QVector <qreal> ();
        vectorEC->reserve(fases->size());

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
        for (qint32 j = 0; j < fases->size(); j++){ //l

            //variables necesarias
            l = mag->at(k);
            f = fases->at(j);

            //Se pasa la magnitud a lineal
            maglineal = pow(10,l/20);
            //Se calcula el número complejo correspondiente a la posición de la rejilla.
            //Se pasa de Nichols a lineal.
            L = complex<qreal> (maglineal * cos (f * M_PI / 180),
                                maglineal * sin (f * M_PI / 180));


            //Se recorre la plantilla y se calcula la plantilla
            //movida por el punto de la rejilla.

            dEstabilidadRuido = -numeric_limits<qreal>::infinity();
            dSeguimiento = numeric_limits<qreal>::infinity();
            dRPS = -numeric_limits<qreal>::infinity();
            dRPE = -numeric_limits<qreal>::infinity();
            dEC = -numeric_limits<qreal>::infinity();

            for (qint32 h = 0; h < p->size(); h++) { // temp

                //Cálculo necesario que se guarda en el template
                p_actual = p->at(h);

                aux_complex = (p0 / p_actual) + L;

                //Estabilidad y ruido del sensor
                dTempEstabilidadRuidoSeguimiento = abs((L / aux_complex));

                //Rechazo de perturbaciones a la salida de la planta
                dTempRPS =  abs((p0 / p_actual) / aux_complex);

                //Rechado de perturbaciones a la entrada de la planta
                dTempRPE = abs((p0 / aux_complex));

                //Esfuerzo de control
                dTempEC = abs((L / p_actual) / aux_complex);

                if (dTempEstabilidadRuidoSeguimiento > dEstabilidadRuido){
                    dEstabilidadRuido = dTempEstabilidadRuidoSeguimiento;
                }
                if (dTempEstabilidadRuidoSeguimiento < dSeguimiento){
                    dSeguimiento = dTempEstabilidadRuidoSeguimiento;
                }
                if (dTempRPS > dRPS){
                    dRPS = dTempRPS;
                }
                if (dTempRPE > dRPE){
                    dRPE = dTempRPE;
                }
                if (dTempEC > dEC) {
                    dEC = dTempEC;
                }
            }

#ifdef OpenMP_AVAILABLE
#pragma omp critical
            {

                vectorEstabilidadRuido->append(dEstabilidadRuido);
                vectorSeguimiento->append(dEstabilidadRuido - dSeguimiento);
                vectorRPS->append(dRPS);
                vectorRPE->append(dRPE);
                vectorEC->append(dEC);

            }
#else
            vectorEstabilidadRuido->append(20 * log10(dEstabilidadRuido));
            vectorSeguimiento->append((20 * log10(dEstabilidadRuido)) - (20 * log10(dSeguimiento)));
            vectorRPS->append(20 * log10(dRPS));
            vectorRPE->append(20 * log10(dRPE));
            vectorEC->append(20 * log10(dEC));
#endif
        }
#ifdef OpenMP_AVAILABLE
#pragma omp critical
        {
            sabanaEstabilidadRuido->append(vectorEstabilidadRuido);
            sabanaSeguimiento->append(vectorSeguimiento);
            sabanaRPS->append(vectorRPS);
            sabanaRPE->append(vectorRPE);
            sabanaEC->append(vectorEC);
        }
#else
        sabanaEstabilidadRuido->append(vectorEstabilidadRuido);
        sabanaSeguimiento->append(vectorSeguimiento);
        sabanaRPS->append(vectorRPS);
        sabanaRPE->append(vectorRPE);
        sabanaEC->append(vectorEC);
#endif
    }

    //Vector donde se guardaran los boundaries
    QMap <QString, QVector <QVector <QPointF> * > *> * bound = new QMap <QString, QVector <QVector <QPointF> * > *> ();

    //Vector donde se guardaran los metadatos.
    QMap <QString, QVector <QPoint> * > * metaBound = new QMap <QString, QVector <QPoint> * > ();

    //Guardamos todas las sábanas en un vector para pasarla como parámetro
    QVector <QVector <QVector <qreal> * > * > * vecSabanas = new QVector <QVector <QVector <qreal> * > * > ();

    vecSabanas->append(sabanaEstabilidadRuido);
    vecSabanas->append(sabanaSeguimiento);
    vecSabanas->append(sabanaRPS);
    vecSabanas->append(sabanaRPE);
    vecSabanas->append(sabanaEC);


    calcularContour(omega, bound, vecSabanas, metaBound, p0, p, contador, abs(datosFas.x()), abs(datosMag.x()) + abs (datosMag.y()), datosMag.y());

    vecSabanas->clear();

#ifdef OpenMP_AVAILABLE
#pragma omp critical
    {
        metaDatosBoundaries->replace(num_hilo, metaBound);
        boundaries->replace(num_hilo, bound);
        nueva_omega->replace(num_hilo, omega);
        num_hilo++;
    }
#else
    metaDatosBoundaries->append(metaBound);
    boundaries->append(bound);
    nueva_omega->append(omega);
#endif

    boundariesCreados = true;
}

bool Boundaries::pnpoly(QVector<QPointF *> *vector, QPointF *punto)
{
    qint32 i, j;
    qint32 lon = vector->size();
    bool c = false;
    for (i = 0, j = lon-1; i < lon; j = i++) {
        if (((vector->at(i)->y() > punto->y()) != (vector->at(j)->y() > punto->y())) &&
                (punto->x() < (vector->at(j)->x()) - vector->at(i)->x() *
                 (punto->y()-vector->at(i)->y()) / (vector->at(j)->y()-vector->at(i)->y()) +
                 vector->at(i)->x()) )
            c = !c;
    }

    delete punto;
    return c;
}

//Se normaliza a valores [-180,180):
qreal Boundaries::constrainAngle(qreal x){
    x = fmod(x + M_PI,2 * M_PI);
    if (x < 0)
        x += 2 * M_PI;
    return x - M_PI;
}

//Se convierte a valores [-360,360]
qreal Boundaries::angleConv(qreal angle){
    return fmod(constrainAngle(angle),2 * M_PI);
}
qreal Boundaries::angleDiff(qreal a,qreal b){
    qreal dif = fmod(b - a + M_PI,2 * M_PI);
    if (dif < 0)
        dif += 2 * M_PI;
    return dif - M_PI;
}
qreal Boundaries::unwrap(qreal previousAngle,qreal newAngle){
    //Hay que pasar el ángulo actual y el anterior.
    return previousAngle - angleDiff(newAngle,angleConv(previousAngle));
}
