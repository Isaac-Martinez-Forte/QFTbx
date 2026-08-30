#include "algorithm_segundo_articulo.h"


using namespace tools;
using namespace cxsc;

#define SACHIN
#define NAND

#define REC_UNION
#define REC_INTER
#define REC_FASE
#define REC_MAG
#define MEJOR_K
#define BI_ARBOL
#define ETAPAS

//#define VER_DIAGRAMAS
//#define mensajes
//#define cambioEtapas
//#define mensajesBi


Algorithm_segundo_articulo::Algorithm_segundo_articulo()
{

}

Algorithm_segundo_articulo::~Algorithm_segundo_articulo() {

}

void Algorithm_segundo_articulo::set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> *omega, DatosBound * boundaries,
                                           qreal epsilon) {

    this->planta = planta;
    this->controlador = controlador->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;


    QVector< QVector<QPointF> * > * boun = boundaries->getBoundariesReun();

    QVector <QPointF> * datosFases = new QVector <QPointF> ();
    QVector <QPointF> * datosMag = new QVector <QPointF> ();

    foreach (auto vector, *boun) {

        qreal DatosFasMin = std::numeric_limits<qreal>::max(), DatosFasMax = std::numeric_limits<qreal>::lowest();
        qreal DatosMagMin = std::numeric_limits<qreal>::max(), DatosMagMax = std::numeric_limits<qreal>::lowest();


        foreach (auto p, *vector) {

            if (p.x() < DatosFasMin) {
                DatosFasMin = p.x();
            }

            if (p.x() > DatosFasMax) {
                DatosFasMax = p.x();
            }

            if (p.y() < DatosMagMin) {
                DatosMagMin = p.y();
            }

            if (p.y() > DatosMagMax) {
                DatosMagMax = p.y();
            }
        }

        datosFases->append(QPointF(DatosFasMin, DatosFasMax));
        datosMag->append(QPointF(DatosMagMin, DatosMagMax));

    }

    boundaries->setDatosFasBound(datosFases);
    boundaries->setDatosMagBound(datosMag);


}

bool Algorithm_segundo_articulo::init_algorithm(){


#if defined (BI_ARBOL) && !(defined (REC_INTER) && (defined (REC_MAG) || defined (REC_FASE)) )
    std::cout << "No se puede activar la Bisección en forma de árbol sin tener activo el recorte en fase" << std::endl;
#endif


    lista = new ListaOrdenada();

    conversion = new Natura_Interval_extension();

    Tripleta2 * tripleta;

    struct FC::return_bisection2 retur;
    deteccion = new DeteccionViolacionBoundaries();
    deteccionViolacion = &DeteccionViolacionBoundaries::deteccionViolacionCajaNiNi;

    frecuenciaPrincipal = 0;


    // Creamos las plantas nominales para todas las frecuencias
    plantas_nominales = new QVector <cxsc::complex> ();
    plantas_nominales2 = new QVector <std::complex <qreal>> ();

    foreach (qreal o, *omega) {
        std::complex <qreal> c = planta->evaluate(o);
        plantas_nominales2->append(c);
        plantas_nominales->append(cxsc::complex(c.real(), c.imag()));
    }

    // Comprobamos si el controlador tiene variables en el numerador o denominador
    comprobarVariables(controlador);


    // Comprobamos si el controlador tiene variables en numerador, denominador o ganancia
    // Si no lo tiene se puede retornar ya, no se puede trabajar con el.
    if (!isVariableNume && !isVariableDeno && !controlador->gain()->isUncertain()) {
        controlador_retorno = FC::guardarControlador(controlador, true);
        return false;
    }

    // Inicializamos la mejor solucion al controlador inicial.

    QVector <Parameter *> * numerador = controlador->numerator();
    QVector <Parameter *> * numerador_nuevo = new QVector <Parameter *> ();

    foreach (Parameter * a, *numerador) {
        numerador_nuevo->append(a->clone());
    }


    QVector <Parameter *> * denominador = controlador->denominator();
    QVector <Parameter *> * denominador_nuevo = new QVector <Parameter *> ();

    foreach (Parameter * a, *denominador) {
        denominador_nuevo->append(a->clone());
    }

    Parameter * kNuevo = controlador->gain()->clone();

    mejorSolucion = controlador->create(controlador->name(), numerador_nuevo, denominador_nuevo, kNuevo, new Parameter (0.0));

    // Insertamos el controlador en la LNV

    Tripleta2 * t = new Tripleta2();
    t->setSistema(controlador);
    t->setRecorteActivado(true);

#ifdef ETAPAS
    t->setEtapas(Etapas::INICIAL);
#else
    t->setEtapas(Etapas::INTERMEDIA);
#endif
#ifdef cambioEtapas
    std::cout << "Etapa Inicial" << std::endl;
#endif

    t->setRecorteActivado(true);

    t = calculoTerminosControlador(t);

    lista->insertar(t);

    while (true) {

        if (lista->esVacia()) {
            menerror("El espacio de parámetros inicial del controlador no es válido.", "Loop Shaping");

            delete conversion;
            delete lista;
            delete deteccion;
            delete mejorSolucion;

            return false;
        }

        tripleta = static_cast<Tripleta2 *>(lista->recuperarPrimero());
        lista->borrarPrimero();

        LtiSystem * controladorActual = tripleta->getSistema();

        // Si la mejor solucion es mejor que el nodo extraido este se descarta y pasa al siguiente.
        if (mejorSolucion->gain()->range().y() < controladorActual->gain()->range().x() ){
            delete tripleta;
            continue;
        }

        // Si la mejor solucion NO es menor que el nodo actual, pero la parte superior si es menor, esta se puede actualizar para quitar espacio de busqueda.
        if (mejorSolucion->gain()->range().y() < controladorActual->gain()->range().y()) {
            controladorActual->gain()->range().setY(mejorSolucion->gain()->range().y() );
        }

        // Aplicar mejoras sobre el nodo
        bool aplicadas = aplicarMejoras(tripleta);

        if (!aplicadas) {
            continue;
        }

        // Comprobamos si el controlador actual es solución.
        if (tripleta->getFlags() == feasible || FC::if_less_epsilon(tripleta->getSistema(), epsilon, omega, conversion, plantas_nominales)) {
            if (tripleta->getFlags() == ambiguous) {
                controlador_retorno = FC::guardarControlador(tripleta->getSistema(), false);
            } else {
                controlador_retorno = FC::guardarControlador(tripleta->getSistema(), true);
            }

            delete conversion;
            delete lista;
            delete deteccion;
            delete tripleta;
            delete mejorSolucion;

            return true;
        }

        retur = biseccion(tripleta);

        if (retur.t1 != nullptr) {
            // Calculamos el beneficio estimado y guardamos en la lista.
            lista->insertar(beneficioEstimado(retur.t1));
        }
        if (retur.t2 != nullptr) {
            // Calculamos el beneficio estimado y guardamos en la lista.
            lista->insertar(beneficioEstimado(retur.t2));
        }
    }
}

// Se implementa un beneficio estimado básico que luego se mejorará
inline Tripleta2 * Algorithm_segundo_articulo::beneficioEstimado (Tripleta2 * tripleta) {

    // TODO falta implementar beneficio estimado avanzado.

    tripleta->setIndex(tripleta->getSistema()->gain()->range().x());

    return tripleta;
}

inline bool Algorithm_segundo_articulo::aplicarMejoras (Tripleta2 *tripleta) {


    bool noError = analizar(tripleta);

    if (!noError) {
        return false;
    }

    if (tripleta->getFlags() == feasible) {
        return true;
    }

    if (tripleta->isRecorteActivado()){

        cambioEtapaFinal = true;

        // Ejecutamos los recortes infeasible.
        tripleta = recortesInfeasible(tripleta);

#ifdef MEJOR_K
        // Ejecutamos la búsqueda de mejor ganancia
        LtiSystem * mS = busquedaMejorGanancia(tripleta);
#else
        LtiSystem * mS = nullptr;
#endif

        // Si búsqueda mejor ganancia ha encontrado un resultado
        if (mS != nullptr) {

            //Comprobamos si es mejor solución que la mejor solución actual.
            // TODO redefinir operador para LtiSystem y tripleta.
            if (mS->gain()->range().y() < mejorSolucion->gain()->range().y()) {

                // Si es así borramos la anterior solución y ponemos la nueva.
                delete mejorSolucion;
                mejorSolucion = mS;
            } else {
                delete mS;
            }

        }
        // Si búsqueda mejor ganancia no nos da resultado ejecutamos los recortes feasible.
#if defined REC_INTER && (defined REC_FASE || defined REC_MAG)
        tripleta = recortesFeasible(tripleta);
#endif


        // TODO revisar esto.
        // Se comprueba si se ha llegado al límite para hacer recortes, si es así, se pasa a la etapa final del algoritmo.
#ifdef ETAPAS
        if (tripleta->getEtapas() == Etapas::INTERMEDIA && cambioEtapaFinal){

            tripleta->setRecorteActivado(false);
            tripleta->setEtapas(Etapas::FINAL);
#ifdef cambioEtapas
            std::cout << "Etapa Final" << std::endl;
#endif
        }
#endif

    }

    return true;
}

//Función que comprueba si la caja actual es feasible, infeasible o ambiguous.

inline bool Algorithm_segundo_articulo::analizar(Tripleta2 * tripleta) {

    data_box * datos;

    flags_box flag_final = feasible;

    QVector <data_box *> * datosCortesBoundaries = new QVector <data_box *> ();

    cinterval caja;

    qreal areaMasGrande = std::numeric_limits<qreal>::lowest();

    bool cambioEtapa = true;

#ifdef VER_DIAGRAMAS

    QVector <QVector<QPointF> * > *vectorCajas = new QVector <QVector<QPointF> * >();
#endif

    for (qint32 k = 0; k < omega->size(); k++) {

        // Comprobamos si para esta frecuencia la caja de proyección es feasible.
        if (!tripleta->isFrecueciaFeasible(k)) {

            caja = conversion->get_box(tripleta->getSistema(), omega->at(k), plantas_nominales->at(k));

            boundaries->setBox(conversion->getBoxDB());

            datos = (deteccion->*deteccionViolacion)(caja, boundaries, k, tripleta->getEtapas());


#ifdef VER_DIAGRAMAS

            QVector <QPointF> * v = new QVector <QPointF> ();

            cinterval caja = conversion->get_box(tripleta->getSistema(),omega->at(k), plantas_nominales->at(k), false);

            v->append(QPointF(_double(InfIm(caja)), _double(InfRe(caja))));
            v->append(QPointF(_double(InfIm(caja)), _double(SupRe(caja))));
            v->append(QPointF(_double(SupIm(caja)), _double(SupRe(caja))));
            v->append(QPointF(_double(SupIm(caja)), _double(InfRe(caja))));

            vectorCajas->append(v);
#endif



            if (datos->getFlag() == infeasible) {

                datosCortesBoundaries->clear();

                delete tripleta;
                return false;
            }

            qreal area = _double(diam(Re(caja)) * diam(Im(caja)));

            if (datos->getFlag() == ambiguous && area > areaMasGrande) {
                areaMasGrande = area;

                frecuenciaPrincipal = k;

                tripleta->setLados(_double(diam(Re(caja))), _double(diam(Im(caja))));
            }

            if (!datos->getCambioEtapa()) {
                cambioEtapa = false;
            }


            if (datos->getFlag() == ambiguous) {
                flag_final = ambiguous;
            }
        } else {
            datos = new data_box();
            datos->setFlag(feasible);
        }


        datosCortesBoundaries->append(datos);
    }

#ifdef VER_DIAGRAMAS
    FC::mostrar_diagrama(vectorCajas, omega, boundaries);
#endif

#ifdef ETAPAS

    if (tripleta->getEtapas() == Etapas::INICIAL && cambioEtapa) {
        tripleta->setEtapas(Etapas::INTERMEDIA);
#ifdef cambioEtapas
        std::cout << "Etapa Intermedia" << std::endl;
#endif
    }
#endif

    tripleta->setFlags(flag_final);

    tripleta->setDatosCortesBoundaries(datosCortesBoundaries);

    return true;
}

inline FC::return_bisection2 Algorithm_segundo_articulo::biseccion(Tripleta2 * tripleta) {


    switch (tripleta->getEtapas()) {
    case Etapas::INICIAL:
        return biseccionArea(tripleta);
        break;

    case Etapas::INTERMEDIA:
#ifdef BI_ARBOL
        return biseccionArbol(tripleta);
#else
        return biseccionArea(tripleta);
#endif
        break;

    case Etapas::FINAL:

        if ((tripleta->getAnchoFas() < tripleta->getAnchoMag())){
            return biseccionFas(tripleta);
        } else {
            return biseccionMag(tripleta);
        }

        break;
    }

    return biseccionArea(tripleta);
}

inline LtiSystem * Algorithm_segundo_articulo::busquedaMejorGanancia (Tripleta2 * tripleta) {

    LtiSystem * v = tripleta->getSistema();

    QVector<data_box *> * datosCortesBoundaries = tripleta->getDatosCortesBoundaries();

    QVector <Parameter *> * denominador = v->denominator();
    QVector <Parameter *> * numerador = v->numerator();
    QVector <Parameter *> * denominador_nuevo = new QVector <Parameter *>();
    QVector <Parameter *> * numerador_nuevo = new QVector <Parameter *>();

    QPointF k = v->gain()->range();
    qreal kNuevo = v->gain()->range().y();

    //QVector <qreal> * mejoresGanacias = new QVector<qreal>();

    QVector <qreal> * numeradorSup = new QVector <qreal> ();
    QVector <qreal> * denominadorInf = new QVector <qreal> ();

    foreach (Parameter * var, *numerador) {
        numeradorSup->append(var->range().y());

        numerador_nuevo->append(var->clone());
    }

    foreach (Parameter * var, *denominador) {
        denominadorInf->append(var->range().x());

        denominador_nuevo->append(var->clone());
    }

    bool entra = false;

    // Frecuencia de diseño a usar, valores de de Mag min y max del boundarie dentro de la caja de proyección, nueva ganancia solución feasible.
    qreal o, cortesMax, gananciaFeasible;

    std::complex <qreal> plantaNominal;
    for (qint32 i = 0; i < omega->size(); i++) {

        if (datosCortesBoundaries->at(i)->getFlag() == ambiguous) {
            o = omega->at(i);
            plantaNominal = plantas_nominales2->at(i);
            cortesMax = datosCortesBoundaries->at(i)->getMinimoxMaximos()->at(1);

            //Análisis de la ganancia

            // Si se puede recortar por abajo y abajo es la parte feasible.
            // Se comenta porque por ahora no se va a implementar esta opción.
            // TODO falta por implementar

            // Si se puede recortar por arriba y arriba es la parte feasible.
            if (datosCortesBoundaries->at(i)->isRecArriba() && !datosCortesBoundaries->at(i)->isUniArriba()){
                gananciaFeasible = cortesMax / abs(v->evaluate(numeradorSup, denominadorInf, 1, 0, o) * plantaNominal);

                if (gananciaFeasible > k.x() && gananciaFeasible < k.y()) {

                    if (gananciaFeasible < kNuevo) {
                        kNuevo = gananciaFeasible;

                        entra = true;
                    }
                } /*else if (datosCortesBoundaries->at(i)->isRecAbajo() && !datosCortesBoundaries->at(i)->isUniAbajo()){

                }*/
            }
        }
    }




    if (entra) {
        k.setY(kNuevo);
        v->gain()->setRange(k);

        cambioEtapaFinal = false;

#ifdef mensajes
        std::cout << "Entra mejor k" << std::endl;
#endif

        return v->create(v->name(), numerador_nuevo, denominador_nuevo, new Parameter("kv", QPointF(k.x(), kNuevo), k.x()), new Parameter (0.0));
    } else {
        return nullptr;
    }
}

inline void Algorithm_segundo_articulo::comprobarVariables(LtiSystem *controlador) {
    bool b = true;


    foreach(Parameter * var, *controlador->numerator()) {
        if (var->isUncertain()) {
            b = false;
            break;
        }
    }

    isVariableNume = !b;

    b = true;

    foreach(Parameter * var, *controlador->denominator()) {
        if (var->isUncertain()) {
            b = false;
            break;
        }
    }

    isVariableDeno = !b;

}

//Función que recorta la caja.
inline Tripleta2 * Algorithm_segundo_articulo::recortesInfeasible(Tripleta2 * tripleta) {

    LtiSystem * v = tripleta->getSistema();

    QVector<data_box *> * datosCortesBoundaries = tripleta->getDatosCortesBoundaries();

    QVector <Parameter *> * denominador = v->denominator();
    QVector <Parameter *> * numerador = v->numerator();
    QPointF k = v->gain()->range();
    QPointF kNuevo = v->gain()->range();

    //Creamos los numeradores y denominadores necesarios
    QVector <qreal> * numeradorSup = new QVector <qreal> ();
    QVector <qreal> * numeradorInf = new QVector <qreal> ();
    QVector <qreal> * numeradorInfNuevo = new QVector <qreal> ();
    QVector <qreal> * numeradorSupNuevo = new QVector <qreal> ();

    //Creamos el vector de segundos mejores feasible para guardarlos.
    //QVector <qreal> * segundosTerminosNume = new QVector <qreal>();
    //QVector <qreal> * segundosTerminosDeno = new QVector <qreal>();

    foreach (Parameter * var, *numerador) {
        numeradorInf->append(var->range().x());
        numeradorSup->append(var->range().y());
        numeradorInfNuevo->append(var->range().x());
        numeradorSupNuevo->append(var->range().y());
        //segundosTerminosNume->append(var->range().x());
    }

    QVector <qreal> * denominadorInf = new QVector <qreal> ();
    QVector <qreal> * denominadorSup = new QVector <qreal> ();
    QVector <qreal> * denominadorSupNuevo = new QVector <qreal> ();
    QVector <qreal> * denominadorInfNuevo = new QVector <qreal> ();

    foreach (Parameter * var, *denominador) {
        denominadorInf->append(var->range().x());
        denominadorSup->append(var->range().y());
        denominadorInfNuevo->append(var->range().x());
        denominadorSupNuevo->append(var->range().y());
        //segundosTerminosDeno->append(var->range().y());
    }

    bool entraNume = false, entraDeno = false, entraK = false;

    qreal nuevoMinKReal, nuevoMaxKReal, n, nuevoMinNume, nuevoMaxDeno, o, cortesMinMag, nuevoMaxNume, cortesMaxImag, cortesMinImag, cortesMaxMag, nuevoMinDeno;
    std::complex <qreal> plantaNominal;
    for (qint32 i = 0; i < omega->size(); i++) {

        if (datosCortesBoundaries->at(i)->getFlag() == ambiguous) {

            o = omega->at(i);

            plantaNominal = plantas_nominales2->at(i);
            cortesMinMag = datosCortesBoundaries->at(i)->getMinimoxMaximos()->at(0);
            cortesMaxMag = datosCortesBoundaries->at(i)->getMinimoxMaximos()->at(1);
            cortesMinImag = datosCortesBoundaries->at(i)->getMinimoxMaximos()->at(2);
            cortesMaxImag = datosCortesBoundaries->at(i)->getMinimoxMaximos()->at(3);

            //Análisis de la ganancia
#ifdef SACHIN
            // Unión K
            if (datosCortesBoundaries->at(i)->isRecAbajo() && datosCortesBoundaries->at(i)->isUniAbajo()){
                nuevoMinKReal = cortesMinMag / abs(v->evaluate(numeradorSup, denominadorInf, 1, 0, o) * plantaNominal);

                if (nuevoMinKReal > kNuevo.x() && nuevoMinKReal < kNuevo.y()) {
                    kNuevo.setX(nuevoMinKReal);
                    entraK = true;
                }
            } else if (datosCortesBoundaries->at(i)->isRecArriba() && datosCortesBoundaries->at(i)->isUniArriba()){
                nuevoMaxKReal = cortesMaxMag / abs(v->evaluate(numeradorInf, denominadorSup, 1, 0, o) * plantaNominal);

                if (nuevoMaxKReal > kNuevo.x() && nuevoMaxKReal < kNuevo.y()) {

                    kNuevo.setY(nuevoMaxKReal);
                    entraK = true;
                }
            }
#endif

            //Numerador
            if (isVariableNume){
                for (qint32 j = 0; j < numerador->size(); j++) {
                    if (numerador->at(j)->isUncertain()){

                        n = numeradorSup->at(j);
                        numeradorSup->remove(j);
#ifdef NAND

                        if (datosCortesBoundaries->at(i)->isRecAbajo() && datosCortesBoundaries->at(i)->isUniAbajo()){

                            nuevoMinNume = sqrt( pow((cortesMinMag * abs (v->evaluateDenominator(denominadorInf, o))) /
                                                     (k.y() *  abs (v->evaluateNumerator(numeradorSup, o) * plantaNominal)), 2) - pow(o, 2));

                            if (nuevoMinNume > numeradorInfNuevo->at(j) && nuevoMinNume < numeradorSupNuevo->at(j)) {
                                numeradorInfNuevo->replace(j, nuevoMinNume);
                                entraNume = true;
                            }
                        }
#endif

#if defined REC_FASE && defined REC_UNION
                        if (datosCortesBoundaries->at(i)->isRecDerecha() && datosCortesBoundaries->at(i)->isUniDerecha()){
                            nuevoMaxNume = o / tan(cortesMaxImag - std::arg (v->evaluateNumerator(numeradorSup, o)) + std::arg (v->evaluateDenominator(denominadorInf, o)) - std::arg (plantaNominal));

                            if (nuevoMaxNume > numeradorInfNuevo->at(j) && nuevoMaxNume < numeradorSupNuevo->at(j)) {
                                numeradorSupNuevo->replace(j, nuevoMaxNume);
                                entraNume = true;
                            }
                        }
#endif
                        numeradorSup->insert(j, n);

                        n = numeradorInf->at(j);
                        numeradorInf->remove(j);

#ifdef NAND
                        if (datosCortesBoundaries->at(i)->isRecArriba() && datosCortesBoundaries->at(i)->isUniArriba()){
                            nuevoMaxNume = sqrt( pow((cortesMaxMag * abs (v->evaluateDenominator(denominadorSup, o))) /
                                                     (k.x() *  abs (v->evaluateNumerator(numeradorInf, o) * plantaNominal)), 2) - pow(o, 2));

                            if (nuevoMaxNume > numeradorInfNuevo->at(j) && nuevoMaxNume < numeradorSupNuevo->at(j)) {
                                numeradorSupNuevo->replace(j, nuevoMaxNume);
                                entraNume = true;
                            }
                        }

#endif

#if defined REC_FASE && defined REC_UNION
                        if (datosCortesBoundaries->at(i)->isRecIzquierda() && datosCortesBoundaries->at(i)->isUniIzquierda()){

                            nuevoMinNume = o / tan(cortesMinImag - std::arg (v->evaluateNumerator(numeradorInf, o)) + std::arg (v->evaluateDenominator(denominadorSup, o)) - std::arg (plantaNominal));

                            if (nuevoMinNume > numeradorInfNuevo->at(j) && nuevoMinNume < numeradorSupNuevo->at(j)) {
                                numeradorInfNuevo->replace(j, nuevoMinNume);
                                entraNume = true;
                            }
                        }
#endif

                        numeradorInf->insert(j, n);

                    }
                }
            }


            if (isVariableDeno){
                for (qint32 j = 0; j < denominador->size(); j++) {
                    if (denominador->at(j)->isUncertain()){

                        n = denominadorInf->at(j);
                        denominadorInf->remove(j);

#ifdef NAND

                        if (datosCortesBoundaries->at(i)->isRecAbajo() && datosCortesBoundaries->at(i)->isUniAbajo()){

                            nuevoMaxDeno = sqrt(pow((k.y() * abs (v->evaluateNumerator(numeradorSup, o) * plantaNominal)) /
                                                    (cortesMinMag * abs(v->evaluateDenominator(denominadorInf, o))), 2) - pow(o, 2));

                            if (nuevoMaxDeno < denominadorSupNuevo->at(j) && nuevoMaxDeno > denominadorInfNuevo->at(j)) {
                                denominadorSupNuevo->replace(j, nuevoMaxDeno);
                                entraDeno = true;
                            }
                        }
#endif

#if defined REC_FASE && defined REC_UNION
                        if (datosCortesBoundaries->at(i)->isRecDerecha() && datosCortesBoundaries->at(i)->isUniDerecha()){
                            nuevoMaxDeno = o / tan(-cortesMinImag + std::arg (v->evaluateNumerator(numeradorSup, o)) - std::arg (v->evaluateDenominator(denominadorInf, o)) + std::arg (plantaNominal));

                            if (nuevoMaxDeno > denominadorInfNuevo->at(j) && nuevoMaxDeno < denominadorSupNuevo->at(j)) {
                                denominadorSupNuevo->replace(j, nuevoMaxDeno);
                                entraDeno = true;
                            }
                        }
#endif
                        denominadorInf->insert(j, n);

                        n = denominadorSup->at(j);
                        denominadorSup->remove(j);

#ifdef NAND
                        if (datosCortesBoundaries->at(i)->isRecArriba() && datosCortesBoundaries->at(i)->isUniArriba()){
                            nuevoMinDeno = sqrt(pow((k.x() * abs (v->evaluateNumerator(numeradorInf, o) * plantaNominal)) /
                                                    (cortesMaxMag * abs(v->evaluateDenominator(denominadorSup, o))), 2) - pow(o, 2));
                            if (nuevoMinDeno < denominadorSupNuevo->at(j) && nuevoMinDeno > denominadorInfNuevo->at(j)) {
                                denominadorInfNuevo->replace(j, nuevoMinDeno);
                                entraDeno = true;
                            }
                        }
#endif

#if defined REC_FASE && defined REC_UNION
                        if (datosCortesBoundaries->at(i)->isRecIzquierda() && datosCortesBoundaries->at(i)->isUniIzquierda()){
                            nuevoMinDeno = o / tan(-cortesMaxImag + std::arg (v->evaluateNumerator(numeradorInf, o)) - std::arg (v->evaluateDenominator(denominadorSup, o)) + std::arg (plantaNominal));

                            if (nuevoMinDeno > denominadorInfNuevo->at(j) && nuevoMinDeno < denominadorSupNuevo->at(j)) {
                                denominadorInfNuevo->replace(j, nuevoMinDeno);
                                entraDeno = true;
                            }
                        }
#endif

                        denominadorSup->insert(j, n);
                    }
                }
            }
        }
    }


    if (entraNume || entraDeno || entraK) {
        QVector <Parameter *> * numerador_nuevo;

        numerador_nuevo = new QVector <Parameter *> ();

        for (qint32 i = 0; i < numerador->size(); i++){

            Parameter * var_nume_antiguo = numerador->at(i);
            Parameter * var_nume_nuevo;

            if (var_nume_antiguo->isUncertain()){
                var_nume_nuevo = new Parameter("", QPointF(numeradorInfNuevo->at(i), numeradorSupNuevo->at(i)), 0);
            } else {
                var_nume_nuevo = new Parameter(var_nume_antiguo->nominal());
            }

            numerador_nuevo->append(var_nume_nuevo);
        }

        QVector <Parameter *> * denominador_nuevo;

        denominador_nuevo = new QVector <Parameter *> ();
        for (qint32 i = 0; i < denominador->size(); i++){

            Parameter * var_deno_antiguo = denominador->at(i);
            Parameter * var_deno_nuevo;

            if (var_deno_antiguo->isUncertain()){
                var_deno_nuevo = new Parameter("", QPointF(denominadorInfNuevo->at(i), denominadorSupNuevo->at(i)), 0);
            } else {
                var_deno_nuevo = new Parameter(var_deno_antiguo->nominal());
            }

            denominador_nuevo->append(var_deno_nuevo);
        }

        LtiSystem * nuevo_sistema = v->create(v->name(), numerador_nuevo, denominador_nuevo, new Parameter("kv", kNuevo, 0), new Parameter (0.0));
        delete v;



        numeradorSup->clear();
        numeradorInf->clear();
        numeradorInfNuevo->clear();

        denominadorInf->clear();
        denominadorSup->clear();
        denominadorSupNuevo->clear();

        tripleta->setSistema(nuevo_sistema);

        cambioEtapaFinal = false;
    }


    return tripleta;

}

//Función que recorta la caja.
inline Tripleta2 *Algorithm_segundo_articulo::recortesFeasible(Tripleta2 *tripleta) {

#if defined REC_INTER && (defined REC_FASE || defined REC_MAG)

    LtiSystem * v = tripleta->getSistema();

    QVector<data_box *> * datosCortesBoundaries = tripleta->getDatosCortesBoundaries();

    QVector <Parameter *> * denominador = v->denominator();
    QVector <Parameter *> * numerador = v->numerator();
    QPointF k = v->gain()->range();

    ListaOrdenada * kNuevoMax = new ListaOrdenada(true);
    ListaOrdenada * kNuevoMin = new ListaOrdenada();

    //Creamos los numeradores y denominadores necesarios
    QVector <qreal> * numeradorSup = new QVector <qreal> ();
    QVector <qreal> * numeradorInf = new QVector <qreal> ();

    QVector <ListaOrdenada *> * numeradorSupNuevoMag = new QVector <ListaOrdenada *> ();
    QVector <ListaOrdenada *> * numeradorInfNuevoMag = new QVector <ListaOrdenada *> ();
    QVector <ListaOrdenada *> * numeradorSupNuevoFas = new QVector <ListaOrdenada *> ();
    QVector <ListaOrdenada *> * numeradorInfNuevoFas = new QVector <ListaOrdenada *> ();

    bool entraK = false;
    bool entraNume = false;
    bool entraDeno = false;

    foreach (Parameter * var, *numerador) {
        numeradorInf->append(var->range().x());
        numeradorSup->append(var->range().y());

        numeradorSupNuevoMag->append(new ListaOrdenada(true));
        numeradorInfNuevoMag->append(new ListaOrdenada());

        numeradorSupNuevoFas->append(new ListaOrdenada(true));
        numeradorInfNuevoFas->append(new ListaOrdenada());
    }

    QVector <qreal> * denominadorInf = new QVector <qreal> ();
    QVector <qreal> * denominadorSup = new QVector <qreal> ();

    QVector <ListaOrdenada *> * denominadorInfNuevoMag = new QVector <ListaOrdenada *> ();
    QVector <ListaOrdenada *> * denominadorSupNuevoMag = new QVector <ListaOrdenada *> ();
    QVector <ListaOrdenada *> * denominadorInfNuevoFas = new QVector <ListaOrdenada *> ();
    QVector <ListaOrdenada *> * denominadorSupNuevoFas = new QVector <ListaOrdenada *> ();

    foreach (Parameter * var, *denominador) {
        denominadorInf->append(var->range().x());
        denominadorSup->append(var->range().y());

        denominadorInfNuevoMag->append(new ListaOrdenada(true));
        denominadorSupNuevoMag->append(new ListaOrdenada());

        denominadorInfNuevoFas->append(new ListaOrdenada(true));
        denominadorSupNuevoFas->append(new ListaOrdenada());
    }

    qreal nuevoMaxKReal, nuevoMinKReal, n, nuevoMinNume, nuevoMaxDeno, o, cortesMinMag, nuevoMaxNume, cortesMaxImag, cortesMinImag, cortesMaxMag, nuevoMinDeno;
    std::complex <qreal> plantaNominal;
    for (qint32 i = 0; i < omega->size(); i++) {

           // TODO revisar la cajas factibles resultado de la bisección.
        if (datosCortesBoundaries->at(i)->getFlag() == ambiguous) {
            o = omega->at(i);
            plantaNominal = plantas_nominales2->at(i);
            cortesMinMag = datosCortesBoundaries->at(i)->getMinimoxMaximos()->at(0);
            cortesMaxMag = datosCortesBoundaries->at(i)->getMinimoxMaximos()->at(1);
            cortesMinImag = datosCortesBoundaries->at(i)->getMinimoxMaximos()->at(2);
            cortesMaxImag = datosCortesBoundaries->at(i)->getMinimoxMaximos()->at(3);

            //Análisis de la ganancia

#ifdef REC_MAG
            // intersección k
            if (datosCortesBoundaries->at(i)->isRecArriba() && !datosCortesBoundaries->at(i)->isUniArriba()){
                nuevoMaxKReal = cortesMaxMag / abs(v->evaluate(numeradorInf, denominadorSup, 1, 0, o) * plantaNominal);

                if (nuevoMaxKReal > k.x() && nuevoMaxKReal < k.y()) {

                    kNuevoMax->insertar(new DatosFeasible(nuevoMaxKReal, i, "kv",
                                                          100 - ((((k.y() - k.x()) - (k.y()  - nuevoMaxKReal))
                                                                  / (k.y()  - k.x())) * 100 )));

                    entraK = true;
                }
            } else if (datosCortesBoundaries->at(i)->isRecAbajo() && !datosCortesBoundaries->at(i)->isUniAbajo()) {
                nuevoMinKReal = cortesMinMag / abs(v->evaluate(numeradorSup, denominadorInf, 1, 0, o) * plantaNominal);

                if (nuevoMinKReal > k.x() && nuevoMinKReal < k.y()) {

                    kNuevoMin->insertar(new DatosFeasible(nuevoMinKReal, i, "kv", ( ((nuevoMinKReal - k.x()) * 100) / (k.y() - k.x()) )));

                    entraK = true;

                }
            }
#endif

            //Numerador
            if (isVariableNume){
                for (qint32 j = 0; j < numerador->size(); j++) {
                    if (numerador->at(j)->isUncertain()){

                        n = numeradorInf->at(j);
                        numeradorInf->remove(j);

#ifdef REC_MAG
                        if (datosCortesBoundaries->at(i)->isRecArriba() && !datosCortesBoundaries->at(i)->isUniArriba()){
                            nuevoMaxNume = sqrt( pow((cortesMaxMag * abs (v->evaluateDenominator(denominadorSup, o))) /
                                                     (k.x() *  abs (v->evaluateNumerator(numeradorInf, o) * plantaNominal)), 2) - pow(o, 2));

                            if (nuevoMaxNume > n && nuevoMaxNume < numeradorSup->at(j)) {
                                numeradorSupNuevoMag->at(j)->insertar(new DatosFeasible(nuevoMaxNume, i, numerador->at(j)->name(),
                                                                                        ( ((numeradorSup->at(j) - nuevoMaxNume) * 100) / (numeradorSup->at(j) - n))));

                                entraNume = true;

                            }
                        }
#endif
#ifdef REC_FASE

                        if (datosCortesBoundaries->at(i)->isRecIzquierda() && !datosCortesBoundaries->at(i)->isUniIzquierda()){
                            nuevoMinNume = o / tan(cortesMinImag - std::arg (v->evaluateNumerator(numeradorInf, o)) + std::arg (v->evaluateDenominator(denominadorSup, o)) - std::arg (plantaNominal));

                            if (nuevoMinNume > n && nuevoMinNume < numeradorSup->at(j)) {
                                numeradorInfNuevoFas->at(j)->insertar(new DatosFeasible(nuevoMinNume, i, numerador->at(j)->name(),
                                                                                        ( ((nuevoMinNume - n) * 100) / (numeradorSup->at(j) - n) )));

                                entraNume = true;
                            }
                        }
#endif
                        numeradorInf->insert(j, n);

                        n = numeradorSup->at(j);
                        numeradorSup->remove(j);

#ifdef REC_MAG
                        if (datosCortesBoundaries->at(i)->isRecAbajo() && !datosCortesBoundaries->at(i)->isUniAbajo()) {
                            nuevoMinNume = sqrt( pow((cortesMinMag * abs (v->evaluateDenominator(denominadorInf, o))) /
                                                     (k.y() *  abs (v->evaluateNumerator(numeradorSup, o) * plantaNominal)), 2) - pow(o, 2));

                            if (nuevoMinNume > numeradorInf->at(j) && nuevoMinNume < n) {
                                numeradorInfNuevoMag->at(j)->insertar(new DatosFeasible(nuevoMinNume, i, numerador->at(j)->name(),
                                                                                        ( ((nuevoMinNume - numeradorInf->at(j)) * 100) / (n - numeradorInf->at(j)) )));

                                entraNume = true;
                            }
                        }
#endif
#ifdef REC_FASE

                        if (datosCortesBoundaries->at(i)->isRecDerecha() && !datosCortesBoundaries->at(i)->isUniDerecha()){
                            nuevoMaxNume = o / tan(cortesMaxImag - std::arg (v->evaluateNumerator(numeradorSup, o)) +
                                                   std::arg (v->evaluateDenominator(denominadorInf, o)) - std::arg (plantaNominal));

                            if (nuevoMaxNume > numeradorInf->at(j) && nuevoMaxNume < n) {
                                numeradorSupNuevoFas->at(j)->insertar(new DatosFeasible(nuevoMaxNume, i, numerador->at(j)->name(),
                                                                                        ( ((n - nuevoMaxNume) * 100) / (n - numeradorInf->at(j)) )));
                                entraNume = true;
                            }
                        }
#endif

                        numeradorSup->insert(j, n);
                    }
                }
            }


            if (isVariableDeno){
                for (qint32 j = 0; j < denominador->size(); j++) {
                    if (denominador->at(j)->isUncertain()){

                        n = denominadorSup->at(j);
                        denominadorSup->remove(j);
#ifdef REC_MAG
                        if (datosCortesBoundaries->at(i)->isRecArriba() && !datosCortesBoundaries->at(i)->isUniArriba()){

                            nuevoMinDeno = sqrt(pow((k.x() * abs (v->evaluateNumerator(numeradorInf, o) * plantaNominal)) /
                                                    (cortesMaxMag * abs(v->evaluateDenominator(denominadorSup, o))), 2) - pow(o, 2));

                            if (nuevoMinDeno < n && nuevoMinDeno > denominadorInf->at(j)) {
                                denominadorInfNuevoMag->at(j)->insertar(new DatosFeasible(nuevoMinDeno, i, denominador->at(j)->name(),
                                                                                          ( ((nuevoMinDeno - denominadorInf->at(j)) * 100) / (n - denominadorInf->at(j)) ) ));
                                entraDeno = true;
                            }
                        }
#endif
#ifdef REC_FASE
                        if (datosCortesBoundaries->at(i)->isRecDerecha() && !datosCortesBoundaries->at(i)->isUniDerecha()){
                            nuevoMaxDeno = o / tan(-cortesMinImag + std::arg (v->evaluateNumerator(numeradorInf, o)) -
                                                   std::arg (v->evaluateDenominator(denominadorSup, o)) + std::arg (plantaNominal));

                            if (nuevoMaxDeno < n && nuevoMaxDeno > denominadorInf->at(j)) {
                                denominadorSupNuevoFas->at(j)->insertar(new DatosFeasible(nuevoMaxDeno, i, denominador->at(j)->name(),
                                                                                          ( ((n - nuevoMaxDeno) * 100) / (n - denominadorInf->at(j)) )));
                                entraDeno = true;
                            }
                        }
#endif
                        denominadorSup->insert(j, n);

                        n = denominadorInf->at(j);
                        denominadorInf->remove(j);
#ifdef REC_MAG

                        if (datosCortesBoundaries->at(i)->isRecAbajo() && !datosCortesBoundaries->at(i)->isUniAbajo()){

                            nuevoMaxDeno = sqrt(pow((k.y() * abs (v->evaluateNumerator(numeradorSup, o) * plantaNominal)) /
                                                    (cortesMinMag * abs(v->evaluateDenominator(denominadorInf, o))), 2) - pow(o, 2));

                            if (nuevoMaxDeno < denominadorSup->at(j) && nuevoMaxDeno > n) {
                                denominadorSupNuevoMag->at(j)->insertar(new DatosFeasible(nuevoMaxDeno, i, denominador->at(j)->name(),
                                                                                          ( ((denominadorSup->at(j) - nuevoMaxDeno) * 100) / (denominadorSup->at(j) - n) )));
                                entraDeno = true;
                            }
                        }
#endif
#ifdef REC_FASE
                        if (datosCortesBoundaries->at(i)->isRecIzquierda() && !datosCortesBoundaries->at(i)->isUniIzquierda()){
                            nuevoMinDeno = tan(-cortesMaxImag + std::arg (v->evaluateNumerator(numeradorInf, o)) -
                                               std::arg (v->evaluateDenominator(denominadorSup, o)) + std::arg (plantaNominal)) * o;

                            if (nuevoMinDeno < denominadorSup->at(j) && nuevoMinDeno > n) {
                                denominadorInfNuevoFas->at(j)->insertar(new DatosFeasible(nuevoMinDeno, i, denominador->at(j)->name(),
                                                                                          ( ((nuevoMinDeno - n) * 100) / (denominadorSup->at(j) - n) )));
                                entraDeno = true;
                            }
                        }
#endif
                        denominadorInf->insert(j, n);

                    }
                }
            }
        }
    }

    if (entraK || entraNume || entraDeno) {

        Parameter * k_nuevo;

        k_nuevo = new Parameter ("kv", QPointF( kNuevoMin->esVacia() ? k.x() : kNuevoMin->recuperarPrimeroBorrar()->getIndex(),
                                          kNuevoMax->esVacia() ? k.y() : kNuevoMax->recuperarPrimeroBorrar()->getIndex()), 0);


        QVector <Parameter *> * numerador_nuevo = new QVector <Parameter *> ();

        //getMax
        for (qint32 i = 0; i < numerador->size(); i++){

            Parameter * var_nume_antiguo = numerador->at(i);
            Parameter * var_nume_nuevo;

            if (var_nume_antiguo->isUncertain()){

                qreal unoMag = numeradorInfNuevoMag->at(i)->esVacia() ? numeradorInf->at(i) : numeradorInfNuevoMag->at(i)->recuperarPrimeroBorrar()->getIndex();
                qreal unoFas = numeradorInfNuevoFas->at(i)->esVacia() ? numeradorInf->at(i) : numeradorInfNuevoFas->at(i)->recuperarPrimeroBorrar()->getIndex();

                qreal dosMag = numeradorSupNuevoMag->at(i)->esVacia() ? numeradorSup->at(i) : numeradorSupNuevoMag->at(i)->recuperarPrimeroBorrar()->getIndex();
                qreal dosFas = numeradorSupNuevoFas->at(i)->esVacia() ? numeradorSup->at(i) : numeradorSupNuevoFas->at(i)->recuperarPrimeroBorrar()->getIndex();

                var_nume_nuevo = new Parameter(var_nume_antiguo->name(),
                                         QPointF(unoMag > unoFas ? unoMag : unoFas, dosMag < dosFas ? dosMag : dosFas), 0);
            } else {
                var_nume_nuevo = new Parameter(var_nume_antiguo->nominal());
            }

            numerador_nuevo->append(var_nume_nuevo);
        }

        QVector <Parameter *> * denominador_nuevo;

        //getMin
        denominador_nuevo = new QVector <Parameter *> ();
        for (qint32 i = 0; i < denominador->size(); i++){

            Parameter * var_deno_antiguo = denominador->at(i);
            Parameter * var_deno_nuevo;

            if (var_deno_antiguo->isUncertain()){

                qreal unoMag = denominadorInfNuevoMag->at(i)->esVacia() ? denominadorInf->at(i) : denominadorInfNuevoMag->at(i)->recuperarPrimeroBorrar()->getIndex();
                qreal unoFas = denominadorInfNuevoFas->at(i)->esVacia() ? denominadorInf->at(i) : denominadorInfNuevoFas->at(i)->recuperarPrimeroBorrar()->getIndex();

                qreal dosMag = denominadorSupNuevoMag->at(i)->esVacia() ? denominadorSup->at(i) : denominadorSupNuevoMag->at(i)->recuperarPrimeroBorrar()->getIndex();
                qreal dosFas = denominadorSupNuevoFas->at(i)->esVacia() ? denominadorSup->at(i) : denominadorSupNuevoFas->at(i)->recuperarPrimeroBorrar()->getIndex();

                var_deno_nuevo = new Parameter(var_deno_antiguo->name(),
                                         QPointF(unoMag > unoFas ? unoMag : unoFas, dosMag < dosFas ? dosMag : dosFas), 0);
            } else {
                var_deno_nuevo = new Parameter(var_deno_antiguo->nominal());
            }

            denominador_nuevo->append(var_deno_nuevo);
        }

        LtiSystem * nuevo_sistema = v->create(v->name(), numerador_nuevo, denominador_nuevo, k_nuevo, new Parameter (0.0));
        delete v;

        cambioEtapaFinal = false;

        tripleta->setSistema(nuevo_sistema);
    }

    numeradorSup->clear();
    numeradorInf->clear();

    denominadorInf->clear();
    denominadorSup->clear();

    tripleta->setKInf(kNuevoMin);
    tripleta->setKSup(kNuevoMax);

    tripleta->setNumeradorInfMag(numeradorInfNuevoMag);
    tripleta->setNumeradorSupMag(numeradorSupNuevoMag);
    tripleta->setNumeradorInfFas(numeradorInfNuevoFas);
    tripleta->setNumeradorSupFas(numeradorSupNuevoFas);

    tripleta->setDenominadorInfMag(denominadorInfNuevoMag);
    tripleta->setDenominadorSupMag(denominadorSupNuevoMag);
    tripleta->setDenominadorInfFas(denominadorInfNuevoFas);
    tripleta->setDenominadorSupFas(denominadorSupNuevoFas);

#endif

    return tripleta;
}

//Función que calcula los términos del controlador en decibelios
inline Tripleta2 * Algorithm_segundo_articulo::calculoTerminosControlador (Tripleta2* controlador) {

    LtiSystem * s = controlador->getSistema();
    QVector <cinterval> * terminosNume = new QVector<cinterval>();
    QVector <cinterval> * terminosDeno = new QVector<cinterval>();
    cinterval terminosK;

    if (this->isVariableNume){
        foreach (Parameter*  nume , *s->numerator()){
            terminosNume->append(conversion->get_box_termino_nume(nume, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));
        }
    }

    controlador->setTerNume(terminosNume);

    if (this->isVariableDeno){
        foreach (Parameter*  deno , *s->denominator()){
            terminosDeno->append(conversion->get_box_termino_deno(deno, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));
        }
    }

    controlador->setTerDeno(terminosDeno);

    terminosK = conversion->get_box_termino_k(controlador->getSistema()->gain(), plantas_nominales->at(frecuenciaPrincipal));

    controlador->setTerK(terminosK);

    return controlador;
}

inline FC::return_bisection2 Algorithm_segundo_articulo::biseccionArea(Tripleta2 * tripleta) {

    QVector <cinterval> * terminosNume = tripleta->getTerNume();
    QVector <cinterval> * terminosDeno = tripleta->getTerDeno();
    cinterval terminosK = tripleta->getTerK();

    QVector <cinterval> * terminosCopiaNume = new QVector <cinterval> ();
    QVector <cinterval> * terminosCopiaDeno = new QVector <cinterval> ();
    cinterval terminosCopiaK;


    LtiSystem * current_controlador = tripleta->getSistema();

    QVector <Parameter *> * numerador = current_controlador->numerator();
    QVector <Parameter *> * denominador = current_controlador->denominator();

    Parameter * ret = current_controlador->delay();

    QVector <Parameter *> * numeradorCopia = new QVector <Parameter *> ();
    QVector <Parameter *> * denominadorCopia = new QVector <Parameter *> ();

    QString nombre = current_controlador->name();

    qint32 posicion = -1;
    qreal mejorArea = -1;

    qint32 contador = 0;

    // Calculamos para la ganancia K


    terminosCopiaK = terminosK;
    qreal area = _double(diam(Re(terminosK)));

    if (area > mejorArea) {
        mejorArea = area;
        posicion = -1;
    }


    // Calculamos para el numerador
    for (qint32 i = 0; i < numerador->size(); i++){

        numeradorCopia->append(numerador->at(i)->clone());

        if (numerador->at(i)->isUncertain()) {

            cinterval t = terminosNume->at(i);
            terminosCopiaNume->append(t);
            qreal area = _double(diam(Re(t)) * diam(Im(t)));

            if (area > mejorArea) {
                mejorArea = area;
                posicion = contador;
            }
        }

        contador++;
    }


    // Calculamos para el denominador
    for (qint32 i = 0; i < denominador->size(); i++){

        denominadorCopia->append(denominador->at(i)->clone());

        if (denominador->at(i)->isUncertain()) {
            cinterval t = terminosDeno->at(i);
            terminosCopiaDeno->append(t);

            qreal area = _double(diam(Re(t)) * diam(Im(t)));

            if (area > mejorArea) {
                mejorArea = area;
                posicion = contador;
            }
        }

        contador++;
    }


    Parameter * k, * kCopia;

    if (posicion == -1) {

        Parameter * variable = current_controlador->gain();

        qreal punto_medio = variable->range().x() + (variable->range().y() - variable->range().x()) / 2;

        k = new Parameter("", QPointF(variable->range().x(), punto_medio), variable->range().x());
        kCopia =  new Parameter("", QPointF(punto_medio, variable->range().y()), punto_medio);

        terminosK = conversion->get_box_termino_k(k, plantas_nominales->at(frecuenciaPrincipal));
        terminosCopiaK = conversion->get_box_termino_k(kCopia, plantas_nominales->at(frecuenciaPrincipal));

        delete variable;

    }else{

        k = current_controlador->gain();
        kCopia = k->clone();

        if (posicion < numerador->size()) {

            Parameter * variable = numerador->at(posicion);

            qreal punto_medio = variable->range().x() + (variable->range().y() - variable->range().x()) / 2;

            Parameter * var1 = new Parameter("", QPointF(variable->range().x(), punto_medio), variable->range().x());
            Parameter * var2 =  new Parameter("", QPointF(punto_medio, variable->range().y()), punto_medio);

            terminosNume->replace(posicion, conversion->get_box_termino_nume(var1, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));
            terminosCopiaNume->replace(posicion, conversion->get_box_termino_nume(var2, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));


            numerador->replace(posicion, var1);
            numeradorCopia->replace(posicion, var2);


            delete variable;

        } else {

            qint32 pos = posicion - numerador->size();

            Parameter * variable = denominador->at(pos);

            qreal punto_medio = variable->range().x() + (variable->range().y() - variable->range().x()) / 2;

            Parameter * var1 = new Parameter("", QPointF(variable->range().x(), punto_medio), variable->range().x());
            Parameter * var2 =  new Parameter("", QPointF(punto_medio, variable->range().y()), punto_medio);

            terminosDeno->replace(pos, conversion->get_box_termino_deno(var1, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));
            terminosCopiaDeno->replace(pos, conversion->get_box_termino_deno(var2, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));


            denominador->replace(pos, var1);
            denominadorCopia->replace(pos, var2);


            delete variable;

        }
    }

    Tripleta2 * t1, * t2;
    LtiSystem * v1, * v2;

    v1 = current_controlador->create(nombre, numerador, denominador, k, ret);
    v2 = current_controlador->create(nombre, numeradorCopia, denominadorCopia, kCopia, ret->clone());

    t1 = new Tripleta2();

    t1->setSistema(v1);
    t1->setTerNume(terminosNume);
    t1->setTerDeno(terminosDeno);
    t1->setTerK(terminosK);
    t1->setFrecuenciasFeasible(tripleta->getFrecuenciasFeasible());
    t1->setRecorteActivado(tripleta->isRecorteActivado());
    t1->setEtapas(tripleta->getEtapas());

    t2 = new Tripleta2();

    t2->setSistema(v2);
    t2->setTerNume(terminosCopiaNume);
    t2->setTerDeno(terminosCopiaDeno);
    t2->setTerK(terminosCopiaK);
    t2->setRecorteActivado(tripleta->isRecorteActivado());
    t2->setEtapas(tripleta->getEtapas());



    QHash <qreal, qreal> * hash = tripleta->getFrecuenciasFeasible();

    if (hash != nullptr) {
        t2->setFrecuenciasFeasible(new QHash <qreal, qreal> (*(hash)));
    } else {
        t2->setFrecuenciasFeasible(new QHash <qreal, qreal> ());
    }


    struct FC::return_bisection2 retur;

    retur.t1 = t1;
    retur.t2 = t2;
    retur.descartado = false;


    tripleta->noBorrar2();
    delete tripleta;

    return retur;
}

inline FC::return_bisection2 Algorithm_segundo_articulo::biseccionMag(Tripleta2 * tripleta) {

    QVector <cinterval> * terminosNume = tripleta->getTerNume();
    QVector <cinterval> * terminosDeno = tripleta->getTerDeno();
    cinterval terminosK = tripleta->getTerK();

    QVector <cinterval> * terminosCopiaNume = new QVector <cinterval> ();
    QVector <cinterval> * terminosCopiaDeno = new QVector <cinterval> ();
    cinterval terminosCopiaK;

    LtiSystem * current_controlador = tripleta->getSistema();

    QVector <Parameter *> * numerador = current_controlador->numerator();
    QVector <Parameter *> * denominador = current_controlador->denominator();

    Parameter * ret = current_controlador->delay();

    Parameter * k = current_controlador->gain();

    QVector <Parameter *> * numeradorCopia = new QVector <Parameter *> ();
    QVector <Parameter *> * denominadorCopia = new QVector <Parameter *> ();

    QString nombre = current_controlador->name();

    qint32 posicion = -1;
    qreal mejorMag = -1;


    // Calculamos la k
    mejorMag = _double(diam(20 * cxsc::log10(cxsc::abs(interval(k->range().x(), k->range().y()) * plantas_nominales->at(frecuenciaPrincipal)))));
    qint32 contador = 0;


    for (qint32 i = 0; i < numerador->size(); i++){

        numeradorCopia->append(numerador->at(i)->clone());

        if (numerador->at(i)->isUncertain()) {
            cinterval t = terminosNume->at(i);
            terminosCopiaNume->append(t);
            qreal mag = _double(diam(Re(t)));

            if (mag > mejorMag) {
                mejorMag = mag;
                posicion = contador;
            }
        }

        contador++;
    }

    for (qint32 i = 0; i < denominador->size(); i++){

        denominadorCopia->append(denominador->at(i)->clone());

        if (denominador->at(i)->isUncertain()) {
            cinterval t = terminosDeno->at(i);
            terminosCopiaDeno->append(t);

            qreal mag = _double(diam(Re(t)));

            if (mag > mejorMag) {
                mejorMag = mag;
                posicion = contador;
            }
        }

        contador++;
    }

    Parameter * k1, * k2;

    if (posicion == -1) {


        qreal punto_medio = k->range().x() + (k->range().y() - k->range().x()) / 2;

        k1 = new Parameter("", QPointF(k->range().x(), punto_medio), k->range().x());
        k2 = new Parameter("", QPointF(punto_medio, k->range().y()), punto_medio);

        terminosK = conversion->get_box_termino_k(k1, plantas_nominales->at(frecuenciaPrincipal));
        terminosCopiaK = conversion->get_box_termino_k(k2, plantas_nominales->at(frecuenciaPrincipal));

        delete k;

    } else {

        k1 = k;
        k2 = k->clone();

        if (posicion < numerador->size()) {

            Parameter * variable = numerador->at(posicion);

            qreal punto_medio = variable->range().x() + (variable->range().y() - variable->range().x()) / 2;

            Parameter * var1 = new Parameter("", QPointF(variable->range().x(), punto_medio), variable->range().x());
            Parameter * var2 =  new Parameter("", QPointF(punto_medio, variable->range().y()), punto_medio);

            terminosNume->replace(posicion, conversion->get_box_termino_nume(var1, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));
            terminosCopiaNume->replace(posicion, conversion->get_box_termino_nume(var2, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));


            numerador->replace(posicion, var1);
            numeradorCopia->replace(posicion, var2);


            delete variable;

        } else {

            qint32 pos = posicion - numerador->size();

            Parameter * variable = denominador->at(pos);

            qreal punto_medio = variable->range().x() + (variable->range().y() - variable->range().x()) / 2;

            Parameter * var1 = new Parameter("", QPointF(variable->range().x(), punto_medio), variable->range().x());
            Parameter * var2 =  new Parameter("", QPointF(punto_medio, variable->range().y()), punto_medio);

            terminosDeno->replace(pos, conversion->get_box_termino_deno(var1, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));
            terminosCopiaDeno->replace(pos, conversion->get_box_termino_deno(var2, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));


            denominador->replace(pos, var1);
            denominadorCopia->replace(pos, var2);


            delete variable;

        }
    }

    Tripleta2 * t1, * t2;
    LtiSystem * v1, * v2;

    v1 = current_controlador->create(nombre, numerador, denominador, k1, ret);
    v2 = current_controlador->create(nombre, numeradorCopia, denominadorCopia, k2, ret->clone());

    t1 = new Tripleta2();

    t1->setSistema(v1);
    t1->setTerNume(terminosNume);
    t1->setTerDeno(terminosDeno);
    t1->setTerK(terminosK);
    t1->setFrecuenciasFeasible(tripleta->getFrecuenciasFeasible());
    t1->setRecorteActivado(tripleta->isRecorteActivado());
    t1->setEtapas(tripleta->getEtapas());


    t2 = new Tripleta2();

    t2->setSistema(v2);
    t2->setTerNume(terminosCopiaNume);
    t2->setTerDeno(terminosCopiaDeno);
    t2->setTerK(terminosCopiaK);
    t2->setRecorteActivado(tripleta->isRecorteActivado());
    t2->setEtapas(tripleta->getEtapas());


    QHash <qreal, qreal> * hash = tripleta->getFrecuenciasFeasible();

    if (hash != nullptr) {
        t2->setFrecuenciasFeasible(new QHash <qreal, qreal> (*(hash)));
    } else {
        t2->setFrecuenciasFeasible(new QHash <qreal, qreal> ());
    }

    struct FC::return_bisection2 retur;

    retur.t1 = t1;
    retur.t2 = t2;
    retur.descartado = false;

    tripleta->noBorrar2();
    delete tripleta;

    return retur;
}


inline FC::return_bisection2 Algorithm_segundo_articulo::biseccionFas(Tripleta2 * tripleta) {

    QVector <cinterval> * terminosNume = tripleta->getTerNume();
    QVector <cinterval> * terminosDeno = tripleta->getTerDeno();
    cinterval terminosK = tripleta->getTerK();

    QVector <cinterval> * terminosCopiaNume = new QVector <cinterval> ();
    QVector <cinterval> * terminosCopiaDeno = new QVector <cinterval> ();
    cinterval terminosCopiaK;

    LtiSystem * current_controlador = tripleta->getSistema();

    QVector <Parameter *> * numerador = current_controlador->numerator();
    QVector <Parameter *> * denominador = current_controlador->denominator();

    Parameter * ret = current_controlador->delay();

    Parameter * k = current_controlador->gain();

    QVector <Parameter *> * numeradorCopia = new QVector <Parameter *> ();
    QVector <Parameter *> * denominadorCopia = new QVector <Parameter *> ();

    QString nombre = current_controlador->name();

    qint32 posicion = -1;
    qreal mejorFas = -1;

    qint32 contador = 0;

    bool entra = false;


    for (qint32 i = 0; i < numerador->size(); i++){

        numeradorCopia->append(numerador->at(i)->clone());

        if (numerador->at(i)->isUncertain()) {

            cinterval t = terminosNume->at(i);
            terminosCopiaNume->append(t);
            qreal fase = _double(diam(Re(t)));

            if (fase > mejorFas) {
                mejorFas = fase;
                posicion = contador;

                entra = true;
            }
        }

        contador++;
    }

    for (qint32 i = 0; i < denominador->size(); i++){

        denominadorCopia->append(denominador->at(i)->clone());

        if (denominador->at(i)->isUncertain()) {
            cinterval t = terminosDeno->at(i);
            terminosCopiaDeno->append(t);

            qreal fase = _double(diam(Re(t)));

            if (fase > mejorFas) {
                mejorFas = fase;
                posicion = contador;

                entra = true;
            }
        }

        contador++;
    }

    if (!entra) {
        return biseccionArea(tripleta);
    }

    if (posicion < numerador->size()) {

        Parameter * variable = numerador->at(posicion);

        qreal punto_medio = variable->range().x() + (variable->range().y() - variable->range().x()) / 2;

        Parameter * var1 = new Parameter("", QPointF(variable->range().x(), punto_medio), variable->range().x());
        Parameter * var2 =  new Parameter("", QPointF(punto_medio, variable->range().y()), punto_medio);

        terminosNume->replace(posicion, conversion->get_box_termino_nume(var1, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));
        terminosCopiaNume->replace(posicion, conversion->get_box_termino_nume(var2, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));


        numerador->replace(posicion, var1);
        numeradorCopia->replace(posicion, var2);


        delete variable;

    } else {

        qint32 pos = posicion - numerador->size();

        Parameter * variable = denominador->at(pos);

        qreal punto_medio = variable->range().x() + (variable->range().y() - variable->range().x()) / 2;

        Parameter * var1 = new Parameter("", QPointF(variable->range().x(), punto_medio), variable->range().x());
        Parameter * var2 =  new Parameter("", QPointF(punto_medio, variable->range().y()), punto_medio);

        terminosDeno->replace(pos, conversion->get_box_termino_deno(var1, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));
        terminosCopiaDeno->replace(pos, conversion->get_box_termino_deno(var2, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));


        denominador->replace(pos, var1);
        denominadorCopia->replace(pos, var2);

        delete variable;

    }

    Tripleta2 * t1, * t2;
    LtiSystem * v1, * v2;

    v1 = current_controlador->create(nombre, numerador, denominador, k, ret);
    v2 = current_controlador->create(nombre, numeradorCopia, denominadorCopia, k->clone(), ret->clone());

    t1 = new Tripleta2();

    t1->setSistema(v1);
    t1->setTerNume(terminosNume);
    t1->setTerDeno(terminosDeno);
    t1->setTerK(terminosK);
    t1->setFrecuenciasFeasible(tripleta->getFrecuenciasFeasible());
    t1->setRecorteActivado(tripleta->isRecorteActivado());
    t1->setEtapas(tripleta->getEtapas());


    t2 = new Tripleta2();

    t2->setSistema(v2);
    t2->setTerNume(terminosCopiaNume);
    t2->setTerDeno(terminosCopiaDeno);
    t2->setTerK(terminosCopiaK);
    t2->setRecorteActivado(tripleta->isRecorteActivado());
    t2->setEtapas(tripleta->getEtapas());


    QHash <qreal, qreal> * hash = tripleta->getFrecuenciasFeasible();

    if (hash != nullptr) {
        t2->setFrecuenciasFeasible(new QHash <qreal, qreal> (*(hash)));
    } else {
        t2->setFrecuenciasFeasible(new QHash <qreal, qreal> ());
    }

    struct FC::return_bisection2 retur;

    retur.t1 = t1;
    retur.t2 = t2;
    retur.descartado = false;

    tripleta->noBorrar2();
    delete tripleta;

    return retur;
}

inline FC::return_bisection2 Algorithm_segundo_articulo::biseccionArbol(Tripleta2 * tripleta) {

    LtiSystem * current_controlador = tripleta->getSistema();

    QVector <cinterval> * terminosNume = tripleta->getTerNume();
    QVector <cinterval> * terminosDeno = tripleta->getTerDeno();
    cinterval terminosK = tripleta->getTerK();

    QVector <cinterval> * terminosCopiaNume = new QVector <cinterval> ();
    QVector <cinterval> * terminosCopiaDeno = new QVector <cinterval> ();
    cinterval terminosCopiaK = tripleta->getTerK();

    qint32 mejorPosicion = -1;
    qreal mejorPorcentaje = -1;
    bool izSup = false, mag = false, entra = false;

    QVector <Parameter *> * numerador = current_controlador->numerator();
    QVector <Parameter *> * denominador = current_controlador->denominator();

    ListaOrdenada * kSup = tripleta->getKSup();
    ListaOrdenada * kInf = tripleta->getKInf();

    QVector <ListaOrdenada *> * numeradorInfMag = tripleta->getNumeradorInfMag();
    QVector <ListaOrdenada *> * numeradorSupMag = tripleta->getNumeradorSupMag();
    QVector <ListaOrdenada *> * numeradorInfFas = tripleta->getNumeradorInfFas();
    QVector <ListaOrdenada *> * numeradorSupFas = tripleta->getNumeradorSupFas();

    QVector <ListaOrdenada *> * denominadorInfMag = tripleta->getDenominadorInfMag();
    QVector <ListaOrdenada *> * denominadorSupMag = tripleta->getDenominadorSupMag();
    QVector <ListaOrdenada *> * denominadorInfFas = tripleta->getDenominadorInfFas();
    QVector <ListaOrdenada *> * denominadorSupFas = tripleta->getDenominadorSupFas();

    QVector <Parameter *> * numeradorCopia = new QVector <Parameter *> ();
    QVector <Parameter *> * denominadorCopia = new QVector <Parameter *> ();

    qint32 contador = -1;

    DatosFeasible * dato;

    Parameter * k = current_controlador->gain();
    Parameter * kCopia = k->clone();

#ifdef mensajesBi
    QVector <QString> * numeS = new QVector<QString> ();
    QVector <QString> * denoS = new QVector<QString> ();
    QString kS = QString::fromStdString("[" + std::to_string(k->range().x()) + ". " + std::to_string(k->range().y()) + "]");
#endif

    if (!kSup->esVacia()){

        dato = static_cast<DatosFeasible *>( kSup->recuperarUltimo());
        mejorPorcentaje = dato->getPorcentaje();
        mejorPosicion = contador;

        mag = true;
        izSup = true;

        entra = true;
    } else if (!kInf->esVacia()) {

        dato = static_cast<DatosFeasible *>( kInf->recuperarUltimo());
        mejorPorcentaje = dato->getPorcentaje();
        mejorPosicion = contador;

        mag = true;
        izSup = false;

        entra = true;
    }

    contador++;

    for (qint32 i = 0; i < numeradorSupMag->size(); i++){

        numeradorCopia->append(numerador->at(i)->clone());

#ifdef mensajesBi
        numeS->append(QString::fromStdString("[" + std::to_string(numerador->at(i)->range().x()) + ". " + std::to_string(numerador->at(i)->range().y()) + "]"));
#endif

        if (numerador->at(i)->isUncertain()) {
            terminosCopiaNume->append(terminosNume->at(i));

            if (!numeradorSupMag->at(i)->esVacia()) {

                dato = static_cast<DatosFeasible *>( numeradorSupMag->at(i)->recuperarUltimo());

                qreal por = dato->getPorcentaje();

                if (por > mejorPorcentaje) {
                    mejorPorcentaje = por;
                    mejorPosicion = contador;

                    mag = true;
                    izSup = true;

                    entra = true;
                }
            } else if (!numeradorInfMag->at(i)->esVacia()) {
                dato = static_cast<DatosFeasible *>( numeradorInfMag->at(i)->recuperarUltimo());

                qreal por = dato->getPorcentaje();

                if (por > mejorPorcentaje) {
                    mejorPorcentaje = por;
                    mejorPosicion = contador;

                    mag = true;
                    izSup = false;

                    entra = true;
                }
            }

            if (!numeradorSupFas->at(i)->esVacia()) {
                dato = static_cast<DatosFeasible *>( numeradorSupFas->at(i)->recuperarUltimo());

                qreal por = dato->getPorcentaje();

                if (por > mejorPorcentaje) {
                    mejorPorcentaje = por;
                    mejorPosicion = contador;

                    mag = false;
                    izSup = true;

                    entra = true;
                }
            } else  if (!numeradorInfFas->at(i)->esVacia()) {
                dato = static_cast<DatosFeasible *>( numeradorInfFas->at(i)->recuperarUltimo());

                qreal por = dato->getPorcentaje();

                if (por > mejorPorcentaje) {
                    mejorPorcentaje = por;
                    mejorPosicion = contador;

                    mag = false;
                    izSup = false;

                    entra = true;
                }
            }
        }

        contador++;
    }

    for (qint32 i = 0; i < denominadorInfMag->size(); i++){

        denominadorCopia->append(denominador->at(i)->clone());

#ifdef mensajesBi
        denoS->append(QString::fromStdString("[" + std::to_string(denominador->at(i)->range().x()) + ". " + std::to_string(denominador->at(i)->range().y()) + "]"));
#endif

        if (denominador->at(i)->isUncertain()) {
            terminosCopiaDeno->append(terminosDeno->at(i));

            if (!denominadorSupMag->at(i)->esVacia()) {

                dato = static_cast<DatosFeasible *>( denominadorSupMag->at(i)->recuperarUltimo());

                qreal por = dato->getPorcentaje();

                if (por > mejorPorcentaje) {
                    mejorPorcentaje = por;
                    mejorPosicion = contador;

                    mag = true;
                    izSup = true;

                    entra = true;
                }
            } else if (!denominadorInfMag->at(i)->esVacia()) {
                dato = static_cast<DatosFeasible *>( denominadorInfMag->at(i)->recuperarUltimo());

                qreal por = dato->getPorcentaje();

                if (por > mejorPorcentaje) {
                    mejorPorcentaje = por;
                    mejorPosicion = contador;

                    mag = true;
                    izSup = false;

                    entra = true;
                }
            }

            if (!denominadorSupFas->at(i)->esVacia()) {
                dato = static_cast<DatosFeasible *>( denominadorSupFas->at(i)->recuperarUltimo());

                qreal por = dato->getPorcentaje();

                if (por > mejorPorcentaje) {
                    mejorPorcentaje = por;
                    mejorPosicion = contador;

                    mag = false;
                    izSup = true;

                    entra = true;
                }
            } else  if (!denominadorInfFas->at(i)->esVacia()) {
                dato = static_cast<DatosFeasible *>( denominadorInfFas->at(i)->recuperarUltimo());

                qreal por = dato->getPorcentaje();

                if (por > mejorPorcentaje) {
                    mejorPorcentaje = por;
                    mejorPosicion = contador;

                    mag = false;
                    izSup = false;

                    entra = true;
                }
            }
        }

        contador++;
    }

    if (!entra) {
        return biseccionArea(tripleta);
    }

    QPointF guardarHash;

    if (mejorPosicion == -1) {

        Parameter * variable = k;
        Parameter * variable2 = kCopia;

        qreal punto_corte;

        if (izSup) {

            dato = static_cast<DatosFeasible *>( kSup->recuperarUltimo() );

            punto_corte =  dato->getIndex();

            guardarHash.setX(dato->getPosFrecuencia());
            guardarHash.setY(omega->at(dato->getPosFrecuencia()));

        } else {
            punto_corte =  kInf->recuperarUltimo()->getIndex();
        }

        Parameter * var1 = new Parameter("", QPointF(variable->range().x(), punto_corte), variable->range().x());
        Parameter * var2 =  new Parameter("", QPointF(punto_corte, variable->range().y()), punto_corte);

        k = var1;
        kCopia = var2;


#ifdef mensajes
        std::cout << "Recorte K: [" +  std::to_string(variable->range().x()) + "," + std::to_string(punto_corte) + "] - ["
                     +  std::to_string(punto_corte) + "," + std::to_string(variable->range().y()) + "]";

        std::cout << "Nume: [" +  std::to_string(numerador->at(0)->range().x()) + "," + std::to_string(numerador->at(0)->range().y()) + "]";
        std::cout << "Deno: [" +  std::to_string(denominador->at(0)->range().x()) + "," + std::to_string(denominador->at(0)->range().y()) + "]" << std::endl;
#endif


        terminosK = conversion->get_box_termino_k(var1, plantas_nominales->at(frecuenciaPrincipal));
        terminosCopiaK = conversion->get_box_termino_k(var2, plantas_nominales->at(frecuenciaPrincipal));

        delete variable;
        delete variable2;

    } else if (mejorPosicion < numeradorSupMag->size()) {

        Parameter * variable = numerador->at(mejorPosicion);
        Parameter * variable2 = numeradorCopia->at(mejorPosicion);

        qreal punto_corte;

        if (mag) {
            if (izSup) {

                dato = static_cast<DatosFeasible *>( numeradorSupMag->at(mejorPosicion)->recuperarUltimo() );

                punto_corte =  dato->getIndex();

                guardarHash.setX(dato->getPosFrecuencia());
                guardarHash.setY(omega->at(dato->getPosFrecuencia()));
            } else {

                dato = static_cast<DatosFeasible *>( numeradorInfMag->at(mejorPosicion)->recuperarUltimo() );

                punto_corte =  dato->getIndex();

                guardarHash.setX(dato->getPosFrecuencia());
                guardarHash.setY(omega->at(dato->getPosFrecuencia()));
            }
        } else {
            if (izSup) {

                dato = static_cast<DatosFeasible *>( numeradorSupFas->at(mejorPosicion)->recuperarUltimo() );

                punto_corte =  dato->getIndex();

                guardarHash.setX(dato->getPosFrecuencia());
                guardarHash.setY(omega->at(dato->getPosFrecuencia()));
            } else {

                dato = static_cast<DatosFeasible *>( numeradorInfFas->at(mejorPosicion)->recuperarUltimo() );

                punto_corte =  dato->getIndex();

                guardarHash.setX(dato->getPosFrecuencia());
                guardarHash.setY(omega->at(dato->getPosFrecuencia()));
            }
        }

        Parameter * var1 = new Parameter("", QPointF(variable->range().x(), punto_corte), variable->range().x());
        Parameter * var2 =  new Parameter("", QPointF(punto_corte, variable->range().y()), punto_corte);

#ifdef mensajes
        std::cout << "Recorte N: [" +  std::to_string(variable->range().x()) + ". " + std::to_string(punto_corte) + "] - ["
                     +  std::to_string(punto_corte) + "," + std::to_string(variable->range().y()) + "]";

        std::cout << "K: [" +  std::to_string(k->range().x()) + ". " + std::to_string(k->range().y()) + "]";
        std::cout << "Deno: [" +  std::to_string(denominador->at(0)->range().x()) + ". " + std::to_string(denominador->at(0)->range().y()) + "]" << std::endl;
#endif
        numerador->replace(mejorPosicion, var1);
        numeradorCopia->replace(mejorPosicion, var2);


        terminosNume->replace(mejorPosicion, conversion->get_box_termino_nume(var1, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));
        terminosCopiaNume->replace(mejorPosicion, conversion->get_box_termino_nume(var2, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));


        delete variable;
        delete variable2;
    } else {

        qint32 pos = mejorPosicion - numerador->size();

        Parameter * variable = denominador->at(pos);
        Parameter * variable2 = denominadorCopia->at(pos);

        qreal punto_corte;

        if (mag) {
            if (izSup) {

                dato = static_cast<DatosFeasible *>( denominadorSupMag->at(pos)->recuperarUltimo() );

                punto_corte =  dato->getIndex();

                guardarHash.setX(dato->getPosFrecuencia());
                guardarHash.setY(omega->at(dato->getPosFrecuencia()));
            } else {

                dato = static_cast<DatosFeasible *>( denominadorInfMag->at(pos)->recuperarUltimo() );

                punto_corte =  dato->getIndex();

                guardarHash.setX(dato->getPosFrecuencia());
                guardarHash.setY(omega->at(dato->getPosFrecuencia()));
            }
        } else {
            if (izSup) {

                dato = static_cast<DatosFeasible *>( denominadorSupFas->at(pos)->recuperarUltimo() );

                punto_corte =  dato->getIndex();

                guardarHash.setX(dato->getPosFrecuencia());
                guardarHash.setY(omega->at(dato->getPosFrecuencia()));
            } else {

                dato = static_cast<DatosFeasible *>( denominadorInfFas->at(pos)->recuperarUltimo() );

                punto_corte =  dato->getIndex();

                guardarHash.setX(dato->getPosFrecuencia());
                guardarHash.setY(omega->at(dato->getPosFrecuencia()));
            }
        }

        Parameter * var1 = new Parameter("", QPointF(variable->range().x(), punto_corte), variable->range().x());
        Parameter * var2 =  new Parameter("", QPointF(punto_corte, variable->range().y()), punto_corte);
#ifdef mensajes
        std::cout << "Recorte D: [" +  std::to_string(variable->range().x()) + ". " + std::to_string(punto_corte) + "] - ["
                     +  std::to_string(punto_corte) + ". " + std::to_string(variable->range().y()) + "]";

        std::cout << "K: [" +  std::to_string(k->range().x()) + "," + std::to_string(k->range().y()) + "]";
        std::cout << "Nume: [" +  std::to_string(numerador->at(0)->range().x()) + ". " + std::to_string(numerador->at(0)->range().y()) + "]" << std::endl;
#endif
        denominador->replace(pos, var1);
        denominadorCopia->replace(pos, var2);

        terminosDeno->replace(pos, conversion->get_box_termino_deno(var1, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));
        terminosCopiaDeno->replace(pos, conversion->get_box_termino_deno(var2, omega->at(frecuenciaPrincipal), plantas_nominales->at(frecuenciaPrincipal)));


        delete variable;
        delete variable2;
    }

    Tripleta2 * t1, * t2;
    LtiSystem * v1, * v2;

    Parameter * ret = current_controlador->delay();
    QString nombre = current_controlador->name();

    v1 = current_controlador->create(nombre, numerador, denominador, k, ret);
    v2 = current_controlador->create(nombre, numeradorCopia, denominadorCopia, kCopia, ret->clone());

    t1 = new Tripleta2();

    t1->setSistema(v1);
    t1->setTerNume(terminosNume);
    t1->setTerDeno(terminosDeno);
    t1->setTerK(terminosK);
    t1->setFrecuenciasFeasible(tripleta->getFrecuenciasFeasible());
    t1->setRecorteActivado(tripleta->isRecorteActivado());
    t1->setEtapas(tripleta->getEtapas());


    t2 = new Tripleta2();

    t2->setSistema(v2);
    t2->setTerNume(terminosCopiaNume);
    t2->setTerDeno(terminosCopiaDeno);
    t2->setTerK(terminosCopiaK);
    t2->setRecorteActivado(tripleta->isRecorteActivado());
    t2->setEtapas(tripleta->getEtapas());



    QHash <qreal, qreal> * hash = tripleta->getFrecuenciasFeasible();

    if (hash != nullptr) {
        t2->setFrecuenciasFeasible(new QHash <qreal, qreal> (*(hash)));
    } else {
        t2->setFrecuenciasFeasible(new QHash <qreal, qreal> ());
    }

    t2->addFrecuenciaFeasible(guardarHash.x(), guardarHash.y());


#ifdef mensajes
    std::cout << "FFeasible 1: ";

    if (t1->getFrecuenciasFeasible() == nullptr) {
        std::cout << "null" << std::endl;
    } else {

        std::list<double> list = t1->getFrecuenciasFeasible()->values().toStdList();

        for (auto const &i: list) {
            std::cout << i << " ";
        }

        std::cout << std::endl;
    }

    std::cout << "FFeasible 2: ";

    std::list<double> list = t2->getFrecuenciasFeasible()->values().toStdList();

    for (auto const &i: list) {
        std::cout << i << " ";
    }

    std::cout << std::endl;
#endif

    struct FC::return_bisection2 retur;

    retur.t1 = t1;
    retur.t2 = t2;
    retur.descartado = false;

    tripleta->noBorrar2();
    delete tripleta;

    return retur;

}

LtiSystem * Algorithm_segundo_articulo::getControlador()  {
    return controlador_retorno;
}
