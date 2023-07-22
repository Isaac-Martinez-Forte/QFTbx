#include "templates.h"

#include <QDebug>
#include <QElapsedTimer>
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

void Templates::setMapa(QHash<Var *, QVector<qreal> *> *mapa){
    this->mapa = mapa;
}

void Templates::setEpsilon(QVector<qreal> *epsilon){
    this->epsilon = epsilon;
}

bool Templates::lanzarCalculo(Sistema *planta, QVector<qreal> *omega, bool cuda){
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
        menerror("No se han podido calcular los Templates", "Calculo de Templates");
        return false;
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

QVector<qreal> * Templates::getVariables(Var * a){

    return mapa->value(a);
}

#ifndef OpenMP_AVAILABLE
QVector<QVector<complex<qreal> > * > * Templates::calcularTemplate_secuencial (Sistema *planta, QVector<qreal> *omega){

    ParserX parser (pckALL_COMPLEX);

    parser.EnableAutoCreateVar(true);

    QMap <QString, QVector <qreal> *> * variables = new QMap <QString, QVector <qreal> *> ();
    QVector <QString> * nombres = new QVector <QString> ();

    qint32 lonNume = planta->getNumerador()->size();
    qint32 lonDeno = planta->getDenominador()->size();
    qint32 lon = 0;

    qint32 lonOmega = omega->size();

    combinaciones = 1;

    Var * var;

    for (qint32 i = 0; i < lonNume; i++){

        var = planta->getNumerador()->at(i);

        if (!nombres->contains(var->getNombre()) && var->isVariable()){
            lon++;
            nombres->append(var->getNombre());
            QVector <qreal> * vector = getVariables(var);
            combinaciones *= vector->size();
            variables->insert(var->getNombre(),vector);

            QString s = var->getNombre() + "=" + QString::number(vector->at(0));
            parser.SetExpr(s.toStdString());
            parser.Eval();
        }
    }

    for (qint32 i = 0; i < lonDeno; i++){

        var = planta->getDenominador()->at(i);

        if (!nombres->contains(var->getNombre()) && var->isVariable()){
            lon++;
            nombres->append(var->getNombre());
            QVector <qreal> * vector = getVariables(var);
            combinaciones *= vector->size();
            variables->insert(var->getNombre(),vector);

            QString s = var->getNombre() + "=" + QString::number(vector->at(0));
            parser.SetExpr(s.toStdString());
            parser.Eval();
        }
    }


    if (planta->getK()->isVariable()){
        lon++;
        nombres->append(planta->getK()->getNombre());
        QVector <qreal> * k = getVariables(planta->getK());
        variables->insert(planta->getK()->getNombre(), k);
        combinaciones *= k->size();

        QString s = planta->getK()->getNombre() + "=" + QString::number(k->at(0));
        parser.SetExpr(s.toStdString());
        parser.Eval();
    }


    if(planta->getRet()->isVariable()){
        lon++;
        nombres->append(planta->getRet()->getNombre());
        QVector <qreal> * ret = getVariables(planta->getRet());
        variables->insert(planta->getRet()->getNombre(), ret);
        combinaciones *= ret->size();
        QString s = planta->getRet()->getNombre() + "=" + QString::number(ret->at(0));
        parser.SetExpr(s.toStdString());
        parser.Eval();
    }


    QVector<QVector<complex<qreal> > * > * temCompleto = new QVector <QVector<complex<qreal> > * > ();
    temCompleto->reserve(lonOmega);

    for (qint32 u = 0; u < lonOmega; u++){



        QVector <qint32> * contador = new QVector <qint32> (lon + 1, 0);

        qreal w = omega->at(u);

        QString es = planta->getExpr(w);


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
QVector<QVector<std::complex<qreal> > *> * Templates::calcularTemplate_paralelo(Sistema *planta, QVector<qreal> *omega){

    QMap <QString, QVector <qreal> *> * variables = new QMap <QString, QVector <qreal> *> ();
    QVector <QString> * nombres = new QVector <QString> ();

    qint32 lonNume = planta->getNumerador()->size();
    qint32 lonDeno = planta->getDenominador()->size();
    qint32 lon = 0;

    qint32 lonOmega = omega->size();

    combinaciones = 1;

    Var * var;

    for (qint32 i = 0; i < lonNume; i++){

        var = planta->getNumerador()->at(i);

        if (!nombres->contains(var->getNombre()) && var->isVariable()){
            lon++;
            nombres->append(var->getNombre());
            QVector <qreal> * vector = getVariables(var);
            combinaciones *= vector->size();
            variables->insert(var->getNombre(),vector);
        }
    }

    for (qint32 i = 0; i < lonDeno; i++){

        var = planta->getDenominador()->at(i);

        if (!nombres->contains(var->getNombre()) && var->isVariable()){
            lon++;
            nombres->append(var->getNombre());
            QVector <qreal> * vector = getVariables(var);
            combinaciones *= vector->size();
            variables->insert(var->getNombre(),vector);
        }
    }


    if (planta->getK()->isVariable()){
        lon++;
        nombres->append(planta->getK()->getNombre());
        QVector <qreal> * k = getVariables(planta->getK());
        variables->insert(planta->getK()->getNombre(), k);
        combinaciones *= k->size();
    }


    if(planta->getRet()->isVariable()){
        lon++;
        nombres->append(planta->getRet()->getNombre());
        QVector <qreal> * ret = getVariables(planta->getRet());
        variables->insert(planta->getRet()->getNombre(), ret);
        combinaciones *= ret->size();
    }

    QVector<QVector<complex<qreal> > * > * temCompleto = new QVector <QVector<complex<qreal> > * > (omega->size());
    temCompleto->reserve(lonOmega);

    QVector <qreal> * nueva_omega = new QVector <qreal> (omega->size());

    QVector <qreal> * nueva_epsilon = new QVector <qreal> (epsilon->size());
    qint32 numeroHilo = 0;

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

        QString es = planta->getExpr(w);

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

#pragma omp critical
        {
            nueva_omega->replace(numeroHilo, w);

            temCompleto->replace(numeroHilo, templateParcial);
            nueva_epsilon->replace(numeroHilo, epsilon->at(u));
            numeroHilo++;
        }

        contador->clear();
    }

    variables->clear();
    nombres->clear();

    epsilon->clear();
    epsilon = nueva_epsilon;

    this->omega->clear();
    this->omega = nueva_omega;

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
    if (!cuda){

#ifndef OpenMP_AVAILABLE
        contorno = new QVector <QVector <complex <qreal> > * > ();
#else

        qint32 numHilo = 0;
        QVector <qreal> * nueva_omega = new QVector <qreal> (omega->size());
        QVector <qreal> * nueva_epsilon = new QVector <qreal> (epsilon->size());

        contorno = new QVector <QVector <complex <qreal> > * > (omega->size());
#pragma omp parallel for
#endif
        for (qint32 i = 0; i < lon; i++){

            QVector <complex <qreal> > * cont = e_hull(templates->at(i), epsilon->at(i));

            if (cont == NULL){
                //menerror("Error calculando el contorno de los templates.", "Templates");
                cont = new QVector <complex <qreal> >();

#ifdef OpenMP_AVAILABLE
#pragma omp critical
                correcto = false;
#else
                correcto = false;
#endif
            }

#ifdef OpenMP_AVAILABLE
#pragma omp critical
            {
                contorno->replace(numHilo, cont);
                nueva_omega->replace(numHilo, omega->at(i));
                nueva_epsilon->replace(numHilo, epsilon->at(i));
                numHilo++;
            }
#else
            contorno->append(cont);
#endif

        }

#ifdef OpenMP_AVAILABLE
        omega->clear();
        epsilon->clear();

        omega = nueva_omega;
        epsilon = nueva_epsilon;
#endif

    } else {

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
    }
#else

#ifndef OpenMP_AVAILABLE
    contorno = new QVector <QVector <complex <qreal> > * > ();
#else

    qint32 numHilo = 0;
    QVector <qreal> * nueva_omega = new QVector <qreal> (omega->size());
    QVector <qreal> * nueva_epsilon = new QVector <qreal> (epsilon->size());

    contorno = new QVector <QVector <complex <qreal> > * > (omega->size());
#pragma omp parallel for
#endif
    for (qint32 i = 0; i < lon; i++){

        QVector <complex <qreal> > * cont = e_hull(templates->at(i), epsilon->at(i));

        if (cont == NULL){
            //menerror("Error calculando el contorno de los templates.", "Templates");
            cont = new QVector <complex <qreal> >();

            cout << "Error al calcular el contorno" << endl;

#ifdef OpenMP_AVAILABLE
#pragma omp critical
            correcto = false;
#else
            correcto = false;
#endif
            //epsilon->clear();
        }

#ifdef OpenMP_AVAILABLE
#pragma omp critical
        {
            contorno->replace(numHilo, cont);
            nueva_omega->replace(numHilo, omega->at(i));
            nueva_epsilon->replace(numHilo, epsilon->at(i));
            numHilo++;
        }
#else
        contorno->append(cont);
#endif

    }

#ifdef OpenMP_AVAILABLE
    omega->clear();
    epsilon->clear();

    omega = nueva_omega;
    epsilon = nueva_epsilon;
#endif

#endif

    if (!correcto){
        menerror("No se ha podido calcular el contorno de los Templates", "Calculo de Templates");
        return false;
    }

    contornoCalculado = true;

    return true;
}


QVector <complex <qreal> > * Templates::e_hull(QVector<complex<qreal> > *temp, qreal epsilon){

    qint32 numDatos = temp->size();
    qint32 MAXP = 3 * numDatos;

    qint32 b1 = 0;
    qreal numDe = -numeric_limits<qreal>::infinity();

    QVector <complex <qreal> > * puntos_nichols = temp;

    qreal complejoimag;

    for(qint32 i = 0;i < numDatos ; i++){   //calculamos que punto esta mas a la derecha y ese sera el primer punto.
        complejoimag = imag(temp->at(i));
        // pasamos todos los números complejos al plano de nichols.

        if (complejoimag > numDe){
            b1 = i;
            numDe = complejoimag;
        }
    }

    qint32 b2 = buscarSegundo(b1, puntos_nichols, epsilon); //buscamos el segundo punto.

    if (b2 < 0)
        return NULL;

    QVector <qint32> * resultado = new QVector <qint32>();

    qint32 punto_previo = b1;
    qint32 punto_actual = b2;

    resultado->append(b1);
    resultado->append(b2);

    qreal punto_sig = buscarSiguiente(b1,b2,puntos_nichols,epsilon); //a partir de aquí buscamos los demás puntos
    if (punto_sig < 0)
        return NULL;

    qint32 contador = 2;


    while (b1 != punto_actual || b2 != punto_sig){

        resultado->append(punto_sig);
        contador++;

        if (contador > MAXP)
            break;

        punto_previo = punto_actual;
        punto_actual = punto_sig;


        punto_sig = buscarSiguiente(punto_previo, punto_actual, puntos_nichols, epsilon);

        if (punto_sig < 0){
            return NULL;
        }
    }


    QVector <qint32> * aux = new QVector <qint32> ();

    bool isRepetido = false;

    foreach (qint32 var1, *resultado) {  //Quitamos los valores repetidos del vector

        isRepetido = false;

        foreach (qint32 var2, *aux) {
            if (var1 == var2){
                isRepetido = true;
            }
        }
        if (!isRepetido){
            aux->append(var1);
        }
    }


    QVector <complex <qreal> > * devolver = new QVector <complex <qreal> > ();
    devolver->reserve(aux->size());

    foreach (const qint32 var, *aux) {
        devolver->append(temp->at(var));
    }

    resultado->clear();
    aux->clear();
    //puntos_nichols->clear();

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

            //calculamos el arcotangente del punto actual dividido por epsilon
            fas -= qAcos(dist / epsilon);     //a la fase actual le restamos el arcotangente.

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
                                  QVector<complex<qreal> > *cv, qreal epsilon){

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


        if (absResta > 0 && absResta <= epsilon && cvActual != actual && cvActual != anterior){ // Si la distancia es mayor que cero y menor que epsilon.

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
