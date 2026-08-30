#include "algorithm_rambabu.h"

using namespace tools;
using namespace cxsc;
using namespace FC;
using namespace alg;

Algorithm_rambabu::Algorithm_rambabu()
{

}

Algorithm_rambabu::~Algorithm_rambabu()
{

}

void Algorithm_rambabu::set_datos(LtiSystem *planta, LtiSystem *controlador, QVector<qreal> * omega, DatosBound *boundaries,
                                  qreal epsilon, QVector<QVector<QVector<QPointF> *> *> *reunBounHash, bool depuracion,
                                  QVector <QVector <std::complex <qreal> > * > * temp, QVector <tools::dBND *> * espe){
    this->planta = planta;
    this->controlador = controlador->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;
    this->reunBounHash = reunBounHash;

    this->metaDatosArriba = boundaries->getMetaDatosArriba();
    this->metaDatosAbierto = boundaries->getMetaDatosAbierta();

    this->tamFas = boundaries->getTamFas() -1;
    this->depuracion = depuracion;

    this->temp = temp;
    this->espe = espe;

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

//Función principal del algoritmo
bool Algorithm_rambabu::init_algorithm(){

    using namespace std;

    lista = new ListaOrdenada ();

    conversion = new Natura_Interval_extension ();
    lista = new ListaOrdenada ();

    deteccion = new DeteccionViolacionBoundaries ();

    plantas_nominales = new QVector <cxsc::complex> ();
    plantas_nominales2 = new QVector <std::complex <qreal>> ();

    foreach (qreal o, *omega) {
        std::complex <qreal> c = planta->evaluate(o);
        plantas_nominales2->append(c);
        plantas_nominales->append(cxsc::complex(c.real(), c.imag()));
    }

    check_box_feasibility(controlador);


    while (true){

        if (lista->esVacia()){
            menerror("El espacio de parámetros inicial del controlador no es válido.", "Loop Shaping");

            delete conversion;
            delete lista;

            return false;
        }

        Tripleta * tripleta = static_cast<Tripleta *>(lista->recuperarPrimero());
        lista->borrarPrimero();

        if (tripleta->getFlags() == feasible || FC::if_less_epsilon(tripleta->getSistema(), epsilon, omega, conversion, plantas_nominales)) {
            if (tripleta->getFlags() == ambiguous){
                controlador_retorno = guardarControlador(tripleta->getSistema(), false);
            } else {
                controlador_retorno = guardarControlador(tripleta->getSistema(), true);
            }

            delete conversion;
            delete lista;
            delete tripleta;
            delete deteccion;

            return true;
        }

        //Split blox
        struct return_bisection retur = split_box_bisection(tripleta->getSistema());

        delete tripleta;

        if (!retur.descartado){
            check_box_feasibility(retur.v1);
            check_box_feasibility(retur.v2);
        }
    }


    return true;
}


LtiSystem * Algorithm_rambabu::getControlador(){
    return controlador_retorno;
}

//Función que comprueba si la caja actual es feasible, infeasible o ambiguous.
flags_box Algorithm_rambabu::check_box_feasibility(LtiSystem * v){

    using namespace std;

    flags_box flag_final = feasible;
    flags_box flag;
    cinterval caja;

    qint32 contador = 0;
    QVector <flags_box> flags;
    qint32 contador2;
    data_box * datos;



#ifdef DEGUB
    QVector <QVector <QPointF> * > * puntos;

    if (depuracion)
        puntos = new QVector <QVector <QPointF> * > ();
#endif

    for (qint32 k = 0; k < omega->size(); k++) {

        caja = conversion->get_box(controlador, omega->at(k), plantas_nominales->at(k), false);

        datos = deteccion->deteccionViolacionCajaNiNi(caja, boundaries, k);


#ifdef DEGUB
        if (depuracion)
            puntos->append(deteccion_violacion->getPuntos());
#endif
        contador2 = 0;

        for (qint32 i = 0; i < flags.size(); i++){
            if (flags.at(i) == infeasible){
                contador2++;
            }
        }

        if (contador2 == 0){
            flag = feasible;
        } else if (contador2 < flags.size()){
            flag = ambiguous;
        } else {
            return infeasible;
        }

        if (flag == ambiguous){
            flag_final = ambiguous;
        }

        contador++;
    }
#ifdef DEGUB
    if (depuracion)
        mostrar_diagrama(puntos);
#endif

    if (flag_final == ambiguous){
        v = acelerated(v);

        if (v == nullptr){
            return infeasible;
        }
    }

    lista->insertar(new Tripleta(controlador->gain()->range().x(), controlador, flag_final));

    return flag_final;
}

LtiSystem * Algorithm_rambabu::acelerated(LtiSystem * controlador){

    //creamos los datos del controlador.

    QVector <QVector <QString> * > * vec;

    if (controlador->type() == LtiSystem::SystemType::ZeroPoleGain){
        vec = kganacia(controlador);
    } else if (controlador->type() == LtiSystem::SystemType::TimeConstantGain){
        vec = knganancia(controlador);
    } else {
        return controlador;
    }

    QVector <QString> * g = vec->at(0);
    QVector <QString> * phi = vec->at(1);

    qreal p_mag;
    qreal p_fas;

    qreal pk_mag;
    qreal pk_fas;

    qint32 contador;

    QPointF rango;
    Parameter * var;

    exp_tree ob("1");
    QMap <std::string, interval > * mapa = new QMap <std::string, interval > ();

    QVector <QString> * reglas = nullptr;

    for (qint32 i = 0; i < temp->size(); i++) { //Recorremos los templates

        contador = 0;

        QVector <std::complex <qreal> > * templates = temp->at(i);

        foreach (std::complex <qreal> complejo, *templates) {

            reglas = new QVector <QString> ();


            //TODO magnitud y radianes.
            p_mag = abs(complejo);
            p_fas = arg(complejo);


            //guardamos las variables en el árbol
            //ejecutamos el árbol
            //guardamos las variables intervalares.
            foreach (var, *controlador->numerator()) {
                if (var->isUncertain()){
                    rango = var->range();
                    mapa->insert(var->name().toStdString(), interval (rango.x(),rango.y()));
                }
            }
            foreach (var, *controlador->denominator()) {
                if (var->isUncertain()){
                    rango = var->range();
                    mapa->insert(var->name().toStdString(), interval (rango.x(),rango.y()));
                }
            }

            var = controlador->gain();
            if (var->isUncertain()){
                mapa->insert(var->name().toStdString(), interval(var->range().x(),var->range().y()));
            }

            //estabilidad

            qreal altura = 0;

            if (espe->at(2)->utilizado && espe->at(2)->frecfinal <= altura){

                //g^2 g^2 + 2gp cos( pi + zeta) + 1

                reglas->append(QString("(") + g->at(i) + "^2)*(" + QString::number(p_mag) + "^2)+"+"2*" + g->at(i) + "*(" + QString::number(p_mag)
                               + ")*cos(" + phi->at(i) + "+(" + QString::number(p_fas) + ")) + 1");

                //estabilidad robusta

                //g^2 p^2 (1 - 1/alt^2) + 2gp cos( pi + zeta) + 1

                altura = pow(10,espe->at(2)->getAltura(omega->at(i))/20);

                reglas->append(QString("(") + g->at(i) + "^2)*(" + QString::number(p_mag) + "^2)*(1-(1/(" + QString::number(
                                    altura)+ "^2)))+"+"2*" + g->at(i) + "*(" + QString::number(p_mag) + ")*cos(" +
                                phi->at(i) + "+(" + QString::number(p_fas) + ")) + 1");

            }


            //rechazo de perturbaciones a la salida.

            //g^2 p^2 + 2gp cos (pi + zeta) + (1 - 1/alt^2)

            if (espe->at(4)->utilizado && espe->at(4)->frecfinal <= altura){

                altura = pow(10,espe->at(4)->getAltura(omega->at(i))/20);

                reglas->append(QString("(") + g->at(i) + "^2)*(" + QString::number(p_mag) + "^2)+"+"2*" + g->at(i) + "*(" + QString::number(p_mag)
                               + ")*cos(" + phi->at(i) + "+(" + QString::number(p_fas) + ")) + (1-(1/("
                               + QString::number(altura) + "^2)))");
            }



            //rechado de perturbaciones a la entrada

            //g^2 p^2 + 2gp cos (alpha + beta) + [1 - p^2 / alt^2] >= 0

            if(espe->at(5)->utilizado && espe->at(5)->frecfinal <= altura){

                altura = pow(10,espe->at(5)->getAltura(omega->at(i))/20);


                 reglas->append(QString("(") + g->at(i) + "^2)*(" + QString::number(p_mag) + "^2)+" + "2*" + g->at(i) +
                                "*(" + QString::number(p_mag) + ")*cos(" + phi->at(i) + "+(" + QString::number(p_fas)
                                + ")) + (1 - ((" + QString::number(p_mag) + "^2 ) / (" + QString::number(altura)
                                + "^2)))");

            }



            //traking

            if (espe->at(0)->utilizado){
                for (qint32 j = contador; j < templates->size(); j++){

                    pk_mag = abs(templates->at(j));
                    pk_fas = -arg(templates->at(j)) * 180;

                    qreal alt = espe->at(1)->getAltura(omega->at(i)) - espe->at(0)->getAltura(omega->at(i));

                    alt = altura = pow(10,alt/20);

                    reglas->append(QString("g") + "^2*" + QString::number(pk_mag) + "^2 *" + QString::number(p_mag) + "^2 *(1-1/"
                                   + QString::number(alt) + "^2 )+2*" + QString("g") + "*" + QString::number(pk_mag) + "*"
                                   + QString::number(p_mag) + "*(" + QString::number(pk_mag) + "*cos(" + QString("phi") + "+" + QString::number(p_fas) + ")-"
                                   + QString::number(p_mag) + "/" + QString::number(alt) + "^2 *cos("+ QString("phi") + "+"
                                   + QString::number(pk_fas) + "))+" + QString::number(pk_mag) + "^2 -" + QString::number(p_mag) + "^2 /"
                                   + QString::number(alt) + "^2");
                }
            }



            //esfuerzo de control

            if(espe->at(6)->utilizado){

                //g^2 p^2 - 1/alt^2 + 2gp cos (pi + zeta) +1

                altura = pow(10,espe->at(6)->getAltura(omega->at(i))/20);

                reglas->append(QString("(") + g->at(i) + "^2)*(" + QString::number(p_mag) + "^2)-(1/(" +
                               QString::number(altura)+ "^2))+"+"2*" + g->at(i) + "*" + QString::number(p_mag) +
                               "*cos(" + phi->at(i) + "+(" + QString::number(p_fas) + ")) + 1");
            }


            foreach (QString es, *reglas) {
                ob.setFunc(es.toStdString());

                //ob.imprimir();

                //std::cout << es.toStdString() << std::endl << std::endl;

                if (!ob.recorrer(mapa)){
                    reglas->clear();

                    g->clear();
                    phi->clear();
                    vec->clear();
                    delete mapa;

                    return NULL;
                }
            }
        }

        reglas->clear();
        contador++;
    }

    //Si todo ha ido bien generamos el nuevo controlador recortado.
    interval i;
    QVector <Parameter *> * nume = new QVector <Parameter *> ();
    foreach (var, *controlador->numerator()) {
        if (var->isUncertain()){
            i = mapa->value(var->name().toStdString());
            nume->append(new Parameter(var->name(), QPointF(_double(Inf(i)), _double(Sup(i))), _double(Inf(i))));
        }else{
            nume->append(new Parameter(var->nominal()));
        }
    }

    QVector <Parameter *> * deno = new QVector <Parameter *> ();
    foreach (var, *controlador->denominator()) {
        if (var->isUncertain()){
            i = mapa->value(var->name().toStdString());
            deno->append(new Parameter(var->name(), QPointF(_double(Inf(i)), _double(Sup(i))), _double(Inf(i))));
        }else{
            deno->append(new Parameter(var->nominal()));
        }
    }

    Parameter * k;
    var = controlador->gain();
    if (var->isUncertain()){
        i = mapa->value(var->name().toStdString());
        k = new Parameter (var->name(), QPointF(_double(Inf(i)), _double(Sup(i))), _double(Inf(i)));
    }else {
        k = new Parameter (var->nominal());
    }


    LtiSystem * ns = controlador->create(controlador->name(), nume, deno, k, new Parameter((qreal)0));

    delete controlador;
    vec->clear();
    delete mapa;

    return ns;
}

QVector <QVector <QString> * > * Algorithm_rambabu::kganacia(LtiSystem * controlador){

    QVector <Parameter *> * nume = controlador->numerator();
    QVector <Parameter *> * deno = controlador->denominator();

    QVector <QString> * mag = new QVector <QString> ();
    QVector <QString> * fas = new QVector <QString> ();

    Parameter * v;

    foreach (qreal w, *omega) {
         QString re = "(kv*(";
         QString re1 = "((";

        for (qint32 i = 0; i < nume->size(); i++) {
            v = nume->at(i);
            if (v->isUncertain()){
                re += "sqrt(" + v->name() + "^2 +" + QString::number(w) + "^2)";
                re1 += "atan(" + v->name() + "/" + QString::number(w) + ")";
            } else {
                re += "sqrt(" + QString::number(v->nominal()) + "^2 +" + QString::number(w) + "^2)";
                re1 += "atan(" + QString::number(v->nominal()) + "/" + QString::number(w) + ")";
            }

            if (i < (nume->size()-1)){
                re += "*";
                re1 += "*";
            }
        }

        re += ")/(";
        re1 +=")-(";

        for (qint32 i = 0; i < deno->size(); i++) {
            v = deno->at(i);
            if (v->isUncertain()){
                re += "sqrt(" + v->name() + "^2 +" + QString::number(w) + "^2)";
                re1 += "atan(" + v->name() + "/" + QString::number(w) + ")";
            } else {
                re += "sqrt(" + QString::number(v->nominal()) + "^2+" + QString::number(w) + "^2)";
                re1 += "atan(" + QString::number(v->nominal()) + "/" + QString::number(w) + ")";
            }

            if (i < (deno->size()-1)){
                re += "*";
                re1 += "*";
            }
        }

        re +="))";
        re1 += "))";

        mag->append(re);
        fas->append(re1);
    }

    QVector <QVector <QString> * > * vec = new QVector <QVector <QString> * > ();

    vec->append(mag);
    vec->append(fas);

    return vec;
}


QVector<QVector<QString> *> *Algorithm_rambabu::knganancia(LtiSystem *controlador){

    QVector <Parameter *> * nume = controlador->numerator();
    QVector <Parameter *> * deno = controlador->denominator();

    QVector <QString> * mag = new QVector <QString> ();
    QVector <QString> * fas = new QVector <QString> ();

    Parameter * v;

    foreach (qreal w, *omega) {
        QString re = "(kv*(";
        QString re1 = "((";

        for (qint32 i = 0; i < nume->size(); i++) {
            v = nume->at(i);
            if (v->isUncertain()){
                re += "sqrt(((" + QString::number(w) + "^2) / (" + v->name() + "^2)) + 1)";
                re1 += "atan(" + v->name() + "/" + QString::number(w) + ")";
            } else {
                re += "sqrt(((" + QString::number(w) + "^2) / (" + QString::number(v->nominal()) + "^2)) + 1)";
                re1 += "atan(" + QString::number(w) + "/" + QString::number(v->nominal()) + ")";
            }

            if (i < (nume->size()-1)){
                re += "*";
                re1 += "*";
            }
        }

        re += ")/(";
        re1 +=")-(";

        for (qint32 i = 0; i < deno->size(); i++) {
            v = deno->at(i);
            if (v->isUncertain()){
                re += "sqrt(((" + QString::number(w) + "^2) / (" + v->name() + "^2)) + 1)";
                re1 += "atan(" + v->name() + "/" + QString::number(w) + ")";
            } else {
                re += "sqrt(((" + QString::number(w) + "^2) / (" + QString::number(v->nominal()) + "^2)) + 1)";
                re1 += "atan(" + QString::number(w) + "/" + QString::number(v->nominal()) + ")";
            }


            if (i < (deno->size()-1)){
                re += "*";
                re1 += "*";
            }
        }

        re +="))";
        re1 += "))";

        mag->append(re);
        fas->append(re1);
    }

    QVector <QVector <QString> * > * vec = new QVector <QVector <QString> * > ();

    vec->append(mag);
    vec->append(fas);

    return vec;
}
