#include "templates.h"

#include "Modelo/Herramientas/exception.h"

#include <QDebug>
#include <QElapsedTimer>
#include <algorithm>
#include <iostream>

using namespace std;
using namespace tools;
using namespace mup;


#ifdef CUDA_AVAILABLE
//Función de CUDA que resuelve el algoritmo de la e_hull.
extern "C"
vector <complex <double> > e_hull_cuda(vector <complex <double> > puntos, float epsilon);
#endif

Templates::Templates()
{
    contornoCalculado = false;
}

Templates::~Templates(){
}

void Templates::setMapa(QHash<Parameter *, QVector<qreal> *> *mapa){
    this->mapa = mapa;
}

void Templates::setEpsilon(QVector<qreal> *epsilon){
    this->epsilon = epsilon;
}

bool Templates::lanzarCalculo(LtiSystem *planta, QVector<qreal> *omega, bool cuda){
    this->cuda = cuda;

    QElapsedTimer timer;
    timer.start();
    this->omega = omega;

#ifndef OpenMP_AVAILABLE
    templates = calcularTemplate_secuencial(planta, omega);
#else
    templates = calcularTemplate_paralelo(planta, omega);
#endif

    qDebug() << "Calcular plantilla: " << timer.elapsed() << "milliseconds";


    if (templates == NULL){
        throw qftbx::ComputationError("Could not compute the templates.");
    }

    QElapsedTimer timer2;
    timer2.start();

    bool retorno = calcularContorno(cuda);

    qDebug() << "Calcular contorno: " << timer2.elapsed() << "milliseconds";

    return retorno;

}

bool Templates::lanzarCalculoContorno(QVector<qreal> *epsilon){
    this->epsilon = epsilon;
    QElapsedTimer timer;
    timer.start();

    bool retorno = calcularContorno(cuda);

    qDebug() << "Calcular contorno: " << timer.elapsed() << "milliseconds";

    return retorno;
}

QVector<QVector<complex<qreal> > *> * Templates::getTemplates(){
    return templates;
}

QVector<QVector<complex<qreal> > *> * Templates::getContorno(){
    return contorno;
}

QVector<qreal> * Templates::getVariables(Parameter * a){

    return mapa->value(a);
}

#ifndef OpenMP_AVAILABLE
QVector<QVector<complex<qreal> > * > * Templates::calcularTemplate_secuencial (LtiSystem *planta, QVector<qreal> *omega){

    ParserX parser (pckALL_COMPLEX);

    parser.EnableAutoCreateVar(true);

    QMap <QString, QVector <qreal> *> * variables = new QMap <QString, QVector <qreal> *> ();
    QVector <QString> * nombres = new QVector <QString> ();

    qint32 lonNume = planta->numerator()->size();
    qint32 lonDeno = planta->denominator()->size();
    qint32 lon = 0;

    qint32 lonOmega = omega->size();

    combinaciones = 1;

    Parameter * var;

    for (qint32 i = 0; i < lonNume; i++){

        var = planta->numerator()->at(i);

        if (!nombres->contains(var->name()) && var->isUncertain()){
            lon++;
            nombres->append(var->name());
            QVector <qreal> * vector = getVariables(var);
            combinaciones *= vector->size();
            variables->insert(var->name(),vector);

            QString s = var->name() + "=" + QString::number(vector->at(0));
            parser.SetExpr(s.toStdString());
            parser.Eval();
        }
    }

    for (qint32 i = 0; i < lonDeno; i++){

        var = planta->denominator()->at(i);

        if (!nombres->contains(var->name()) && var->isUncertain()){
            lon++;
            nombres->append(var->name());
            QVector <qreal> * vector = getVariables(var);
            combinaciones *= vector->size();
            variables->insert(var->name(),vector);

            QString s = var->name() + "=" + QString::number(vector->at(0));
            parser.SetExpr(s.toStdString());
            parser.Eval();
        }
    }


    if (planta->gain()->isUncertain()){
        lon++;
        nombres->append(planta->gain()->name());
        QVector <qreal> * k = getVariables(planta->gain());
        variables->insert(planta->gain()->name(), k);
        combinaciones *= k->size();

        QString s = planta->gain()->name() + "=" + QString::number(k->at(0));
        parser.SetExpr(s.toStdString());
        parser.Eval();
    }


    if(planta->delay()->isUncertain()){
        lon++;
        nombres->append(planta->delay()->name());
        QVector <qreal> * ret = getVariables(planta->delay());
        variables->insert(planta->delay()->name(), ret);
        combinaciones *= ret->size();
        QString s = planta->delay()->name() + "=" + QString::number(ret->at(0));
        parser.SetExpr(s.toStdString());
        parser.Eval();
    }


    QVector<QVector<complex<qreal> > * > * temCompleto = new QVector <QVector<complex<qreal> > * > ();
    temCompleto->reserve(lonOmega);

    for (qint32 u = 0; u < lonOmega; u++){



        QVector <qint32> * contador = new QVector <qint32> (lon + 1, 0);

        qreal w = omega->at(u);

        QString es = planta->expression(w);


        QVector <complex<qreal>> * templateParcial = new QVector <complex<qreal>> ();
        templateParcial->reserve(combinaciones);

        for (qint32 i = 0; i < combinaciones; i++){

            parser.SetExpr(es.toStdString());//Guardar la expresión.

            std::complex <qreal> a = parser.Eval().GetComplex();

            /*qreal fase = arg(a)* 180 / M_PI;
            qreal mag = 20*log10(abs(a));

            if (fase >= -1){
                fase -= 360;
            }

            qreal maglineal = pow(10,mag/20);
            a = complex<qreal> (maglineal * cos (fase * M_PI / 180),
                                maglineal * sin (fase * M_PI / 180));*/

            templateParcial->append(a); //llamar parser

            contador->replace(0, contador->first()+1);

            bool salir = false;

            for (qint32 j = 0; j < lon && salir == false;j++){

                if (contador->at(j) >= (variables->value(nombres->at(j))->size())){
                    contador->replace(j,0);
                    contador->replace(j+1, contador->at(j+1) +1);
                }else {
                    salir = true;
                }

                QString s = nombres->at(j) + "=" + QString::number(variables->value(nombres->at(j))->at(contador->at(j)));
                parser.SetExpr(s.toStdString());
                parser.Eval();
            }
        }

        temCompleto->append(templateParcial);

        contador->clear();
    }

    variables->clear();
    nombres->clear();

    return temCompleto;

}

#else
QVector<QVector<std::complex<qreal> > *> * Templates::calcularTemplate_paralelo(LtiSystem *planta, QVector<qreal> *omega){

    QMap <QString, QVector <qreal> *> * variables = new QMap <QString, QVector <qreal> *> ();
    QVector <QString> * nombres = new QVector <QString> ();

    qint32 lonNume = planta->numerator()->size();
    qint32 lonDeno = planta->denominator()->size();
    qint32 lon = 0;

    qint32 lonOmega = omega->size();

    combinaciones = 1;

    Parameter * var;

    for (qint32 i = 0; i < lonNume; i++){

        var = planta->numerator()->at(i);

        if (!nombres->contains(var->name()) && var->isUncertain()){
            lon++;
            nombres->append(var->name());
            QVector <qreal> * vector = getVariables(var);
            combinaciones *= vector->size();
            variables->insert(var->name(),vector);
        }
    }

    for (qint32 i = 0; i < lonDeno; i++){

        var = planta->denominator()->at(i);

        if (!nombres->contains(var->name()) && var->isUncertain()){
            lon++;
            nombres->append(var->name());
            QVector <qreal> * vector = getVariables(var);
            combinaciones *= vector->size();
            variables->insert(var->name(),vector);
        }
    }


    if (planta->gain()->isUncertain()){
        lon++;
        nombres->append(planta->gain()->name());
        QVector <qreal> * k = getVariables(planta->gain());
        variables->insert(planta->gain()->name(), k);
        combinaciones *= k->size();
    }


    if(planta->delay()->isUncertain()){
        lon++;
        nombres->append(planta->delay()->name());
        QVector <qreal> * ret = getVariables(planta->delay());
        variables->insert(planta->delay()->name(), ret);
        combinaciones *= ret->size();
    }

    QVector<QVector<complex<qreal> > * > * temCompleto = new QVector <QVector<complex<qreal> > * > (omega->size());

#pragma omp parallel for
    for (qint32 u = 0; u < lonOmega; u++){
        QVector <complex<qreal>> * templateParcial = new QVector <complex<qreal>> ();

        ParserX parser (pckALL_COMPLEX);

        parser.EnableAutoCreateVar(true);


        QVector <qint32> * contador = new QVector <qint32> (lon + 1, 0);

        qreal w = omega->at(u);

        for (qint32 i = 0; i < nombres->size(); i++){
            QString s = nombres->at(i) + "=" + QString::number(variables->value(nombres->at(i))->at(0));
            parser.SetExpr(s.toStdString());
            parser.Eval();
        }

        QString es = planta->expression(w);

        templateParcial->reserve(combinaciones);

        for (qint32 i = 0; i < combinaciones; i++){

            parser.SetExpr(es.toStdString());//Guardar la expresión.
            templateParcial->append(parser.Eval().GetComplex()); //llamar parser

            contador->replace(0, contador->first()+1);

            bool salir = false;

            for (qint32 j = 0; j < lon && salir == false;j++){

                if (contador->at(j) >= (variables->value(nombres->at(j))->size())){
                    contador->replace(j,0);
                    contador->replace(j+1, contador->at(j+1) +1);
                }else {
                    salir = true;
                }

                QString s = nombres->at(j) + "=" + QString::number(variables->value(nombres->at(j))->at(contador->at(j)));
                parser.SetExpr(s.toStdString());
                parser.Eval();
            }
        }

        //Cada hilo escribe en el indice de SU frecuencia: antes se escribia
        //con un contador compartido en orden de llegada de los hilos, lo que
        //permutaba los templates respecto a omega/epsilon de forma no
        //determinista, y luego se "reparaba" vaciando el vector vivo de
        //Omega (aliasing) y reasignando. Sin permutacion no hay nada que
        //reparar y los vectores del llamante quedan intactos.
        temCompleto->replace(u, templateParcial);

        contador->clear();
    }

    variables->clear();
    nombres->clear();

    return temCompleto;
}

#endif

QVector <qreal> * Templates::getOmega(){
    return omega;
}

QVector <qreal> * Templates::getEpsilon(){
    return epsilon;
}

bool Templates::calcularContorno(bool cuda __attribute__((unused))){

    bool correcto = true;
    qint32 lon = templates->size();

#ifdef CUDA_AVAILABLE
    if (cuda){
        //Rama CUDA aparcada (decision 2026-08-30): sin validar desde Qt 5
        //(toStdVector/fromStdVector ya no existen en Qt 6). Se conserva como
        //referencia hasta la etapa GPU.
        contorno = new QVector <QVector <complex <qreal> > * > ();

        for (qint32 i = 0; i < lon; i++){

            vector <complex <double> > aux = e_hull_cuda(templates->at(i)->toStdVector(), epsilon->at(i));
            QVector <complex <qreal> > aux2 = QVector<complex <qreal> >::fromStdVector(aux);
            QVector <complex <qreal> > * cont = new QVector <complex <qreal> > (aux2);

            if (aux.size() == 0){
                correcto = false;
            }
            contorno->append(cont);
        }

        if (!correcto){
            throw qftbx::ComputationError("Could not compute the template contours.");
        }

        contornoCalculado = true;
        return true;
    }
#endif

    //Camino CPU, comun con y sin OpenMP. Cada iteracion escribe en el indice
    //de SU frecuencia: antes un contador compartido permutaba los contornos
    //en orden de llegada de los hilos (desalineandolos de templates) y luego
    //se "reparaba" vaciando los vectores vivos de Omega y de la GUI
    //(aliasing). Sin permutacion, omega y epsilon quedan intactos.
    contorno = new QVector <QVector <complex <qreal> > * > (lon);

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 i = 0; i < lon; i++){

        QVector <complex <qreal> > * cont = e_hull(templates->at(i), epsilon->at(i));

        if (cont == NULL){
            cont = new QVector <complex <qreal> >();

#ifdef OpenMP_AVAILABLE
#pragma omp critical
#endif
            {
                correcto = false;
            }
        }

        contorno->replace(i, cont);
    }
    if (!correcto){
        throw qftbx::ComputationError("Could not compute the template contours.");
    }

    contornoCalculado = true;

    return true;
}


//Port fiel de EPSHULL.M (epsh2, Montoya 1998; algoritmo de Nordin 1993).
//Divergencia deliberada respecto al MATLAB: cuando no hay candidato inicial
//se devuelve NULL en vez de un contorno vacio (el llamante lo trata como
//error); exceder MAXP lanza ComputationError, como el error del MATLAB.
QVector <complex <qreal> > * Templates::e_hull(QVector<complex<qreal> > *temp, qreal epsilon){

    if (temp == NULL || temp->isEmpty()){
        return NULL;
    }

    //unique(cv): sin duplicados y ordenado con el criterio de MATLAB para
    //complejos (modulo y, a igual modulo, fase). Este orden es ademas el que
    //resuelve los empates de psi igual que la referencia.
    QVector <complex <qreal> > cv = *temp;
    std::sort(cv.begin(), cv.end(),
              [](const complex<qreal> & a, const complex<qreal> & b){
                  const qreal absA = abs(a);
                  const qreal absB = abs(b);
                  if (absA != absB){
                      return absA < absB;
                  }
                  return arg(a) < arg(b);
              });
    cv.erase(std::unique(cv.begin(), cv.end()), cv.end());

    const qint32 numDatos = cv.size();
    const qint32 MAXP = 3 * numDatos;

    //Primer punto: el de mayor parte real (el mas a la derecha), como en
    //EPSHULL.M y en la memoria del PFC. Empates: gana el primero en el
    //orden unique.
    qint32 b1 = 0;
    for (qint32 i = 1; i < numDatos; i++){
        if (real(cv.at(i)) > real(cv.at(b1))){
            b1 = i;
        }
    }

    qint32 b2 = buscarSegundo(b1, &cv, epsilon);

    if (b2 < 0)
        return NULL;

    QVector <qint32> resultado;
    resultado.append(b1);
    resultado.append(b2);

    qint32 punto_previo = b1;
    qint32 punto_actual = b2;

    qint32 punto_sig = buscarSiguiente(b1, b2, &cv, epsilon);
    if (punto_sig < 0)
        return NULL;

    qint32 contador = 2;

    //Se para al volver al par inicial (b1, b2). El recorrido puede repetir
    //puntos (ida y vuelta por puas del template): se conservan, como en el
    //MATLAB, porque son informacion geometrica real del contorno.
    while (b1 != punto_actual || b2 != punto_sig){

        resultado.append(punto_sig);
        contador++;

        if (contador > MAXP){
            //La referencia (EPSHULL.M) cicla sin cerrar en nubes de racimos
            //espaciados ~epsilon ('max_puntos_excedido'). Fallback documentado
            //al recorrido historico, que siempre produce un contorno con
            //cobertura <= epsilon aunque no sea el e-hull canonico.
            qWarning("e_hull: the reference walk did not close (epsilon-hull "
                     "limitation on clustered templates); falling back to the "
                     "relaxed historical walk for this frequency.");
            return e_hull_relaxed(temp, epsilon);
        }

        punto_previo = punto_actual;
        punto_actual = punto_sig;

        punto_sig = buscarSiguiente(punto_previo, punto_actual, &cv, epsilon);

        if (punto_sig < 0){
            return NULL;
        }
    }

    QVector <complex <qreal> > * devolver = new QVector <complex <qreal> > ();
    devolver->reserve(resultado.size());

    foreach (const qint32 var, resultado) {
        devolver->append(cv.at(var));
    }

    return devolver;
}

QVector <complex <qreal> > * Templates::e_hull_relaxed(QVector<complex<qreal> > *temp, qreal epsilon){

    qint32 numDatos = temp->size();
    qint32 MAXP = 3 * numDatos;

    qint32 b1 = 0;
    qreal numDe = -numeric_limits<qreal>::infinity();

    for(qint32 i = 0;i < numDatos ; i++){   //primer punto: maxima parte imaginaria.
        if (imag(temp->at(i)) > numDe){
            b1 = i;
            numDe = imag(temp->at(i));
        }
    }

    qint32 b2 = buscarSegundo(b1, temp, epsilon);

    if (b2 < 0)
        return NULL;

    QVector <qint32> resultado;
    resultado.append(b1);
    resultado.append(b2);

    qint32 punto_previo = b1;
    qint32 punto_actual = b2;

    qint32 punto_sig = buscarSiguiente(b1, b2, temp, epsilon, true);
    if (punto_sig < 0)
        return NULL;

    qint32 contador = 2;

    while (b1 != punto_actual || b2 != punto_sig){

        resultado.append(punto_sig);
        contador++;

        if (contador > MAXP)
            break;      //trunca en silencio: contorno parcial (conducta historica).

        punto_previo = punto_actual;
        punto_actual = punto_sig;

        punto_sig = buscarSiguiente(punto_previo, punto_actual, temp, epsilon, true);

        if (punto_sig < 0){
            return NULL;
        }
    }

    //Deduplicacion de la salida (conducta historica).
    QVector <qint32> unicos;
    foreach (qint32 idx, resultado) {
        if (!unicos.contains(idx)){
            unicos.append(idx);
        }
    }

    QVector <complex <qreal> > * devolver = new QVector <complex <qreal> > ();
    devolver->reserve(unicos.size());

    foreach (const qint32 idx, unicos) {
        devolver->append(temp->at(idx));
    }

    return devolver;
}

qint32 Templates::buscarSegundo(qint32 b1, QVector<complex<qreal> > *cv, qreal epsilon){

    qreal dist = 0;
    complex <qreal> primero = cv->at(b1);

    qreal fmin = numeric_limits<qreal>::infinity();
    qint32 pmin = -1;
    qreal dmax = 0;

    complex <qreal> cvActual;

    qreal fas = 0;

    for (qint32 i = 0; i < cv->size(); i++){    //recorremos todo el vector de puntos.

        cvActual = cv->at(i);
        dist = abs(primero - cvActual); //calculamos el valor absoluto de la resta.

        if (dist > 0 && dist <= epsilon){    //nos quedamos con los mas pequeños.

            fas = arg (cvActual - primero); //calculamos la phase del ángulo de la resta de complejos

            if (fas < 0)        // si dicha fase es menor que cero le sumamos 2*PI
                fas += 2 * M_PI;

            //a la fase le restamos el arcocoseno de la distancia entre epsilon.
            fas -= qAcos(dist / epsilon);

            if (fas < fmin){   //vamos cual es la fase mínima y la guardamos
                fmin = fas;
                pmin = i;
                dmax = dist;

            }else if (fas == fmin && dist > dmax){ //si son iguales guardamos aquella que su distancia sea mayor.
                pmin = i;
                dmax = dist;
            }
        }
    }

    return pmin; //retornamos el mínimo
}

qint32 Templates::buscarSiguiente(qint32 punto_previo, qint32 punto_actual,
                                  QVector<complex<qreal> > *cv, qreal epsilon,
                                  bool excluirAnterior){

    complex <qreal> actual = cv->at(punto_actual);
    complex <qreal> anterior = cv->at(punto_previo);

    qreal aco2 = qAcos(abs(anterior-actual) / epsilon);

    qreal fasActual = 0;

    qreal aco1Actual = 0;
    qreal dmax = 0;

    qreal psiActual = 0;


    qreal psiMinActual = numeric_limits<qreal>::infinity();
    qint32 posPsiMin = -1;

    complex <qreal> cvActual;
    qreal absResta;

    for (qint32 i = 0; i < cv->size(); i++){

        cvActual = cv->at(i);
        absResta = abs(cvActual - actual); //Calculamos el valor absoluto de la distancia del punto al punto actual.


        //Candidatos: todo punto a distancia (0, epsilon] del actual, INCLUIDO
        //el anterior (en EPSHULL.M su exclusion esta deliberadamente
        //comentada): entra por el caso fas==0 y es lo que permite volver
        //atras por puas y ramas finas del template. La variante relajada
        //historica lo excluye.
        if (absResta > 0 && absResta <= epsilon &&
                !(excluirAnterior && (cvActual == anterior || cvActual == actual))){

            //--------------------------------------------

            //calculamos la fase entre los dos puntos normalizada.
            fasActual = arg((cvActual - actual) / (anterior - actual));

            if(fasActual < 0) // si es menor que cero le sumamos 2*PI.
                fasActual +=  2 * M_PI;

            //------------------------------------------------------------

            aco1Actual = qAcos(absResta / epsilon );  //calculamos la arcosecante de dicho valor absoluto

            //------------------------------------------------------------

            if(fasActual == 0){  // Vamos a calcular psi la fase, hay tres tipos de cálculos distintos
                psiActual =  2 * M_PI - aco1Actual - aco2;
            }else if (fasActual > 0 && fasActual < aco2){
                psiActual = fasActual + aco1Actual- aco2;
            }else{
                psiActual = fasActual - aco1Actual - aco2;
            }

            if (psiActual < 0)                   // Si alguno es negativo se sumamos 2*PI
                psiActual +=  2 * M_PI;

            //------------------------------------------------------------

            if (psiActual < psiMinActual) {       //Buscamos el psi mínimo
                psiMinActual = psiActual;         //como puede haber iguales nos quedamos con el de la
                posPsiMin = i;                   //distancia mas grande.
                dmax = absResta;
            }else if (psiActual == psiMinActual &&
                      (absResta > dmax)){
                posPsiMin = i;
                dmax = absResta;
            }
        }
    }

    return posPsiMin; //retornamos el mínimo

}
