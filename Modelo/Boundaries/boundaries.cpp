#include "boundaries.h"

#include <QElapsedTimer>
#include <iostream>

#include "../Herramientas/tools.h" //linspace de los ejes de la sabana

using namespace std;
using namespace tools;

using qftbx::SpecificationType;


#ifdef CUDA_AVAILABLE
//Función de CUDA que resuelve el algoritmo de la e_hull.
extern "C"
std::vector <float *> * bnd_cuda(vector<complex<double> > templates, complex <double> p0, double infinito, vector<float> fas,
                                 vector<float> mag);
#endif

Boundaries::Boundaries()
{
    boundaries = nullptr;
    metaDatosBoundaries = nullptr;
    boun_reunidos = nullptr;
    boundariesHash = nullptr;
    metaDatosAbierta = nullptr;
    metaDatosArriba = nullptr;
    nueva_omega = nullptr;
    cuda = false;
}

Boundaries::~Boundaries()
{
    liberarResultados();
}

void Boundaries::liberarResultados()
{
    if (boundaries != nullptr){
        foreach (auto * mapa, *boundaries){
            foreach (auto * trazas, *mapa){
                foreach (QVector <QPointF> * traza, *trazas){
                    delete traza;
                }
                delete trazas;
            }
            delete mapa;
        }
        delete boundaries;
        boundaries = nullptr;
    }

    if (metaDatosBoundaries != nullptr){
        foreach (auto * mapa, *metaDatosBoundaries){
            foreach (QVector <QPoint> * meta, *mapa){
                delete meta;
            }
            delete mapa;
        }
        delete metaDatosBoundaries;
        metaDatosBoundaries = nullptr;
    }

    if (boun_reunidos != nullptr){
        foreach (QVector <QPointF> * reunido, *boun_reunidos){
            delete reunido;
        }
        delete boun_reunidos;
        boun_reunidos = nullptr;
    }

    if (boundariesHash != nullptr){
        foreach (auto * porFrecuencia, *boundariesHash){
            foreach (QVector <QPointF> * cubeta, *porFrecuencia){
                delete cubeta;
            }
            delete porFrecuencia;
        }
        delete boundariesHash;
        boundariesHash = nullptr;
    }

    delete metaDatosAbierta;
    metaDatosAbierta = nullptr;

    delete metaDatosArriba;
    metaDatosArriba = nullptr;

    //nueva_omega es un alias del vector del llamador: no se libera.
    nueva_omega = nullptr;
}

void Boundaries::lanzarCalculo(QVector<qreal> *omega, LtiSystem *planta, QVector<QVector<complex<qreal> > *> *templates,
                               QVector<dBND *> *altura, QPointF datosFas, qint32 puntosFas, QPointF datosMag,
                               qint32 puntosMag, qreal infinito, bool cuda){


    //El registro historico se valida al convertirlo: una especificacion en
    //uso con altura <= 0 o banda invertida lanza qftbx::InvalidInput aqui,
    //en la frontera, en vez de degenerar el corte en silencio.
    especificaciones = toSpecificationSet(*altura);
    this->cuda = cuda;

    tamFas = puntosFas;
    tamMag = puntosMag;
    this->datosFas = datosFas;
    this->datosMag = datosMag;

    //Los resultados de la ejecucion anterior se liberan aqui: cada ejecucion
    //acumulaba contenedores enteros (y la mascara boolruido ni se vaciaba).
    liberarResultados();

    boolseguimiento.clear();
    boolestabilidad.clear();
    boolruido.clear();
    boolRPS.clear();
    boolRPE.clear();
    boolEC.clear();

    //Como siempre, la banda de seguimiento la gobierna T_L (el limite
    //inferior); T_U solo aporta la altura del corte.
    foreach(qreal o, *omega){
        boolseguimiento.append(especificaciones.at(SpecificationType::TrackingLower).appliesAt(o));
        boolestabilidad.append(especificaciones.at(SpecificationType::Stability).appliesAt(o));
        boolruido.append(especificaciones.at(SpecificationType::SensorNoise).appliesAt(o));
        boolRPS.append(especificaciones.at(SpecificationType::OutputDisturbance).appliesAt(o));
        boolRPE.append(especificaciones.at(SpecificationType::InputDisturbance).appliesAt(o));
        boolEC.append(especificaciones.at(SpecificationType::ControlEffort).appliesAt(o));
    }

    if (boolseguimiento.contains(true) &&
            !especificaciones.at(SpecificationType::TrackingUpper).used()){
        //El codigo historico desreferenciaba la planta nula de T_U.
        throw qftbx::InvalidInput("The tracking boundary needs both tracking "
                                  "specifications (T_L and T_U).");
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

            std::complex <qreal> p0 = planta->evaluate(omega->at(i));
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

    DatosBound * vista = getBoundaries();
    interseccion->ejecutarAlgoritmo(vista, metaDatosBoundaries);
    delete vista;

    cout << "Algoritmo Intersección 1D: " << timer.elapsed() << " milliseconds" << endl;

    boun_reunidos = interseccion->getInterseccionesVectores();
    boundariesHash = interseccion->getInterseccionesBoundaries();

    metaDatosAbierta = interseccion->getMetadatosAbierta();
    metaDatosArriba = interseccion->getMetadatosArriba();

    delete interseccion;

    //Los metadatos ya han sido consumidos por la union 1D.
    foreach (auto * mapa, *metaDatosBoundaries){
        foreach (QVector <QPoint> * meta, *mapa){
            delete meta;
        }
        delete mapa;
    }
    metaDatosBoundaries->clear();

}

DatosBound *Boundaries::getBoundaries(){
    return new DatosBound(boundaries, metaDatosAbierta, metaDatosArriba ,tamFas, datosFas, boun_reunidos, boundariesHash
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
    if (boolseguimiento.at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("Seguimiento",
                      calcularContourVector(especificaciones.trackingSpreadDb(omega), vecSabanas->at(1),
                                            metaBoun, p0, p, 1, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("Seguimiento", metaBoun);
    }

    //Boundarie estabilidad
    if (boolestabilidad.at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("Estabilidad",
                      calcularContourVector(especificaciones.at(SpecificationType::Stability).boundDb(omega), vecSabanas->at(0),
                                            metaBoun, p0, p, 0, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("Estabilidad", metaBoun);
    }

    //Boundarie ruido
    if (boolruido.at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();
        bound->insert("Ruido",
                      calcularContourVector(especificaciones.at(SpecificationType::SensorNoise).boundDb(omega), vecSabanas->at(0),
                                            metaBoun, p0, p, 0, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("Ruido", metaBoun);
    }

    //Boundarie RPS
    if (boolRPS.at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("RPS",
                      calcularContourVector(especificaciones.at(SpecificationType::OutputDisturbance).boundDb(omega), vecSabanas->at(2),
                                            metaBoun, p0, p, 2, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("RPS", metaBoun);
    }

    //Boundarie RPE
    if (boolRPE.at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("RPE",
                      calcularContourVector(especificaciones.at(SpecificationType::InputDisturbance).boundDb(omega), vecSabanas->at(3),
                                            metaBoun, p0, p, 3, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("RPE", metaBoun);
    }

    //Boundarie EC
    if (boolEC.at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("EC",
                      calcularContourVector(especificaciones.at(SpecificationType::ControlEffort).boundDb(omega), vecSabanas->at(4),
                                            metaBoun, p0, p, 4, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("EC", metaBoun);
    }
}

#ifdef CUDA_AVAILABLE
void Boundaries::calcularContour(qreal omega, QMap <QString, QVector <QVector <QPointF> * > *> * bound,
                                 std::vector <float *> * vecSabanasCuda,
                                 QMap <QString, QVector <QPoint> * > * metaBound,
                                 complex <qreal> p0, QVector <complex <qreal> > * p, qint32 contador,
                                 qreal nPuntosFas, qreal nPuntosMag, qreal moverMag){

    //Boundarie seguimiento
    if (boolseguimiento.at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("Seguimiento",
                      calcularContourVector(especificaciones.trackingSpreadDb(omega), vecSabanasCuda->at(1),
                                            metaBoun, p0, p, 1, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("Seguimiento", metaBoun);
    }

    //Boundarie estabilidad
    if (boolestabilidad.at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("Estabilidad",
                      calcularContourVector(especificaciones.at(SpecificationType::Stability).boundDb(omega), vecSabanasCuda->at(0),
                                            metaBoun, p0, p, 0, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("Estabilidad", metaBoun);
    }

    //Boundarie ruido
    if (boolruido.at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("Ruido",
                      calcularContourVector(especificaciones.at(SpecificationType::SensorNoise).boundDb(omega), vecSabanasCuda->at(0),
                                            metaBoun, p0, p, 0, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("Ruido", metaBoun);
    }

    //Boundarie RPS
    if (boolRPS.at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("RPS",
                      calcularContourVector(especificaciones.at(SpecificationType::OutputDisturbance).boundDb(omega), vecSabanasCuda->at(2),
                                            metaBoun, p0, p, 2, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("RPS", metaBoun);
    }

    //Boundarie RPE
    if (boolRPE.at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("RPE",
                      calcularContourVector(especificaciones.at(SpecificationType::InputDisturbance).boundDb(omega), vecSabanasCuda->at(3),
                                            metaBoun, p0, p, 3, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("RPE", metaBoun);
    }

    //Boundarie EC
    if (boolEC.at(contador)){

        QVector <QPoint> * metaBoun = new QVector <QPoint> ();

        bound->insert("EC",
                      calcularContourVector(especificaciones.at(SpecificationType::ControlEffort).boundDb(omega), vecSabanasCuda->at(4),
                                            metaBoun, p0, p, 4, nPuntosFas, nPuntosMag, moverMag));

        metaBound->insert("EC", metaBoun);
    }

}
#endif

QVector <QMap <QString, QVector <QPoint> * > *> * Boundaries::getMetaDatosBoundaries(){
    return metaDatosBoundaries;
}

QVector<QVector<QPointF> *> * Boundaries::calcularContourVector(qreal umbralDb, QVector<QVector<qreal> *> *sabana,
                                                                QVector<QPoint> *metaBoun, std::complex<qreal> p0, QVector<std::complex<qreal> > *p,
                                                                qint32 i, qreal nPuntosFas, qreal nPuntosMag,
                                                                qreal moverMag)
{

    QVector<QVector<QPointF> *> *  boun;

    Contour2 * contour = new Contour2();

    contour->setDatos(umbralDb, sabana);

    boun = contour->getContour(nPuntosFas, nPuntosMag, moverMag);


    //Pre-dimensionado y escritura en el indice j: el critical de antes
    //permutaba los metadatos respecto a sus trazas segun el orden de hilos.
    metaBoun->resize(boun->size());

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 j = 0; j < boun->size(); j++) {
        QPoint punto;
        //Guardamos la zona de inviolabilidad del boundarie: el umbral es el
        //mismo corte en dB con el que se ha trazado el contorno.
        punto.setX(getZona(boun->at(j), p0, p, i, umbralDb));

        metaBoun->replace(j, punto);
    }

    if (metaBoun->isEmpty()) {
        metaBoun->append(QPoint(0,0));
    }

    delete contour;

    return boun;
}


#ifdef CUDA_AVAILABLE
QVector<QVector<QPointF> *> * Boundaries::calcularContourVector(qreal umbralDb, float *sabana,
                                                                QVector<QPoint> *metaBoun, std::complex<qreal> p0,
                                                                QVector<std::complex<qreal> > *p, qint32 i,
                                                                qreal nPuntosFas, qreal nPuntosMag, qreal moverMag){
    QVector<QVector<QPointF> *> *  boun;

    Contour2 * contour = new Contour2();

    contour->setDatos(umbralDb, sabana);

    boun = contour->getContour(nPuntosFas,tamFas, nPuntosMag, tamMag, moverMag);


    //Pre-dimensionado y escritura en el indice j: el critical de antes
    //permutaba los metadatos respecto a sus trazas segun el orden de hilos.
    metaBoun->resize(boun->size());

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 j = 0; j < boun->size(); j++) {
        QPoint punto;
        //Guardamos la zona de inviolabilidad del boundarie: el umbral es el
        //mismo corte en dB con el que se ha trazado el contorno.
        punto.setX(getZona(boun->at(j), p0, p, i, umbralDb));

        metaBoun->replace(j, punto);
    }

    delete contour;

    return boun;
}
#endif

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

    //La sabana esta en dB: el sondeo de zona compara tambien en dB (antes
    //se comparaban magnitudes LINEALES contra alturas en dB, y para el
    //seguimiento una resta lineal contra un spread en dB).
    switch (i){
    case 0:
        if (20 * log10(dEstabilidadRuido) > altura){
            return 0;
        }
        break;
    case 1:
        if ((20 * log10(dEstabilidadRuido) - 20 * log10(dSeguimiento)) > altura){
            return 0;
        }
        break;
    case 2:
        if(20 * log10(dRPS) > altura){
            return 0;
        }
        break;
    case 3:
        if (20 * log10(dRPE) > altura){
            return 0;
        }
        break;
    case 4:
        if (20 * log10(dEC) > altura){
            return 0;
        }
        break;
    default:
        return 1;
    }

    return 1;
}

void Boundaries::bnd(QVector<qreal> *omega, LtiSystem *planta, QVector <QVector <complex <qreal> > *>
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

    //Contenedores pre-dimensionados: cada frecuencia escribe en SU indice.
    //La version anterior con OpenMP creaba los contenedores VACIOS y escribia
    //con replace(num_hilo) donde num_hilo se pasaba POR VALOR (siempre 0):
    //escritura fuera de rango y boundaries de tamano 0 en el build por
    //defecto. El vector de frecuencias del llamante ya no se toca.
    boundaries = new QVector <QMap <QString, QVector <QVector <QPointF> * > * > * >  (omega->size());
    metaDatosBoundaries = new QVector <QMap <QString, QVector <QPoint> * > * >  (omega->size());

    //Se recorren las frecuencias de diseño.
#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 i = 0; i < omega->size(); i++){

        calcularBndOmega(omega->at(i), planta, templates->at(i), fases, mag, inf, i);
    }

    delete fases;
    delete mag;

    nueva_omega = omega;

}

QVector <qreal> * Boundaries::getOmega(){
    return nueva_omega;
}


void Boundaries::calcularBndOmega (qreal omega, LtiSystem * planta,
                                   QVector<std::complex <qreal> > * temp, QVector <qreal> * fases,
                                   QVector <qreal> * mag, qreal inf __attribute__((unused)), qint32 contador){

    //Se crean las variables necesarias
    complex <qreal> p0;

    //Se obtiene cada plantilla.
    QVector <complex <qreal> >  * p = temp;
    //Se resuelve la planta con las frecuencias de diseño.
    p0 = planta->evaluate(omega);

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

    //Se recorre la rejilla (sin paralelismo anidado: el bucle exterior por
    //frecuencia ya es paralelo, y estos bucles comparten variables de ambito
    //de funcion).
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

            //La sabana se guarda SIEMPRE en dB (contrato validado contra el
            //golden; la antigua rama OpenMP guardaba magnitudes lineales y
            //el seguimiento como resta lineal).
            vectorEstabilidadRuido->append(20 * log10(dEstabilidadRuido));
            vectorSeguimiento->append((20 * log10(dEstabilidadRuido)) - (20 * log10(dSeguimiento)));
            vectorRPS->append(20 * log10(dRPS));
            vectorRPE->append(20 * log10(dRPE));
            vectorEC->append(20 * log10(dEC));
        }
        sabanaEstabilidadRuido->append(vectorEstabilidadRuido);
        sabanaSeguimiento->append(vectorSeguimiento);
        sabanaRPS->append(vectorRPS);
        sabanaRPE->append(vectorRPE);
        sabanaEC->append(vectorEC);
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

    //Las sabanas (~1.7 MB por frecuencia) ya no se necesitan: los contornos
    //y las zonas estan extraidos. Antes se abandonaban con un clear().
    foreach (QVector <QVector <qreal> * > * sabana, *vecSabanas){
        foreach (QVector <qreal> * filaSabana, *sabana){
            delete filaSabana;
        }
        delete sabana;
    }
    delete vecSabanas;

    //Cada frecuencia escribe en su indice: sin criticals ni permutaciones.
    metaDatosBoundaries->replace(contador, metaBound);
    boundaries->replace(contador, bound);
}
