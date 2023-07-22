#include "parserload.h"


using namespace tools;
using namespace std;

XmlParserLoad::XmlParserLoad()
{
}


QVector<bool> *XmlParserLoad::recuperarXmlDatos(QString fichero){

    QVector <bool> * resultado;

    isPlanta = false;
    isEspecificaciones = false;
    isOmega = false;
    isTemplates = false;
    isBoundaries = false;
    isControlador = false;
    isLoopShaping = false;
    isContorno = false;

    QFile file(fichero);

    if (!file.open(QIODevice::ReadOnly)){
        menerror("No se puede leer el fichero", "Cargar Fichero");
        resultado = new QVector <bool> (7, false);
        return resultado;
    }

    stream = new QXmlStreamReader(&file);

    if (!stream->readNextStartElement()){//leemos la cabecera.
        resultado = new QVector <bool> (7, false);
        salidaError();
        return resultado;
    }

    if (stream->name() != "QFT"){
        resultado = new QVector <bool> (7, false);
        salidaError();
        return resultado;
    }

    leerNombreSeccion();

    stream->skipCurrentElement();

    resultado = new QVector <bool> ();

    resultado->append(isPlanta); //planta
    resultado->append(isEspecificaciones);    //especificaciones
    resultado->append(isOmega);  //omega
    resultado->append(isTemplates); //templates
    resultado->append(isBoundaries); //boundaries
    resultado->append(isControlador);   //esructura del controlador
    resultado->append(isLoopShaping); //lazo
    resultado->append(isContorno); //Contorno de los templates

    return resultado;
}

inline void XmlParserLoad::leerNombreSeccion(){

    bool salir = false;

    while (!salir){

        if (!stream->readNextStartElement()){       //leemos la sección
            salir = true;
        }

        if (!asignarLectura(stream->name().toString())){       //asignamos la lectura
            salir = true;
        }
    }
}

inline bool XmlParserLoad::asignarLectura(QString cont){

    if ( cont == "Planta" ){
        isPlanta = leerPlanta(0);
        return isPlanta;
    } else if (cont == "especificaciones") {
        isEspecificaciones = leerEspecificaciones();
        return isEspecificaciones;
    } else if (cont == "omega"){
        isOmega = leerOmega();
        return isOmega;
    } else if (cont == "templates"){
        isTemplates = leerTemplates();
        return isTemplates;
    }else if (cont == "boundaries"){
        isBoundaries = leerBoundaries();
        return isBoundaries;
    }else if (cont == "Controlador"){
        return isControlador = leerPlanta(1);
    }else if(cont == "LoopShaping"){
        return isLoopShaping = leerLoopShaping();
    }

    return false;
}

Sistema *XmlParserLoad::getPlanta(){
    return planta;
}

Omega * XmlParserLoad::getOmega(){
    return omega;
}

QVector <tools::dBND *> * XmlParserLoad::getEspecificaciones(){
    return especificaciones;
}

QVector <QVector <std::complex <qreal>> * > * XmlParserLoad::getTemplates(){
    return templates;
}

QVector <QVector <std::complex <qreal>> * > * XmlParserLoad::getContorno(){
    return contorno;
}

DatosBound *XmlParserLoad::getBoundaries(){
    return bound;
}

QVector <QVector <QVector <qreal> * > *> * XmlParserLoad::getSabana(){
    return sabanas;
}

Sistema * XmlParserLoad::getControlador(){
    return controlador;
}

DatosLoopShaping * XmlParserLoad::getLoopShaping(){
    return loopShaping;
}

QVector <qreal> * XmlParserLoad::getEpsilon(){
    return epsilon;
}

inline bool XmlParserLoad::leerEspec(QMap <QString, QVector <QVector <QPointF> * > *> * mapa){

    if (!stream->readNextStartElement()){       //leemos la sección de la especificación
        return salidaError();
    }

    QString nombre = stream->name().toString();
    QVector <QVector <QPointF> * > * vec = new QVector <QVector <QPointF> * > ();

    qint32 lon = stream->attributes()[0].value().toString().toDouble();

    for (qint32 i = 0; i < lon; i++){

        if (!stream->readNextStartElement()){       //leemos la sección de la x
            return salidaError();
        }

        QVector <qreal> * vec1 = srtovectorReal(stream->readElementText());

        QVector <QPointF> * vec2 = new QVector <QPointF> ();

        for  (qint32 j = 0; j < vec1->size() - 1; j = j+2){

            QPointF punto;

            punto.setX(vec1->at(j));
            punto.setY(vec1->at(j+1));

            vec2->append(punto);
        }

        vec->append(vec2);
        vec1->clear();
    }

    mapa->insert(nombre, vec);

    stream->skipCurrentElement();

    return true;
}

inline bool XmlParserLoad::leerVectorVectorPunto(QVector <QVector <QPointF> * > * vec){

    if (!stream->readNextStartElement()){       //leemos la sección de la especificación
        return salidaError();
    }

    qint32 lon = stream->attributes()[0].value().toString().toDouble();

    for (qint32 i = 0; i < lon; i++){

        if (!stream->readNextStartElement()){       //leemos la sección de la x
            return salidaError();
        }

        QVector <qreal> * vec1 = srtovectorReal(stream->readElementText());

        QVector <QPointF> * vec2 = new QVector <QPointF> ();

        for  (qint32 j = 0; j < vec1->size() - 1; j = j+2){

            QPointF punto;

            punto.setX(vec1->at(j));
            punto.setY(vec1->at(j+1));

            vec2->append(punto);
        }

        vec->append(vec2);
        vec1->clear();
    }

    stream->skipCurrentElement();

    return true;
}

inline bool XmlParserLoad::leerFrecuencias(QMap <QString, QVector <QVector <QPointF> * > *> * mapa){

    if (!stream->readNextStartElement()){       //leemos la sección frecuencia-*
        return salidaError();
    }

    qint32 lon = stream->attributes()[0].value().toString().toDouble();

    for (qint32 i = 0; i < lon; i++){

        if (!leerEspec(mapa)){
            return salidaError();
        }
    }

    stream->skipCurrentElement();

    return true;
}

QVector<bool> * XmlParserLoad::leerVectorBool(){

    if (!stream->readNextStartElement()){
        salidaError();
    }

    return strtobool(stream->readElementText());
}

inline bool XmlParserLoad::leerBoundaries(){

    if (!stream->readNextStartElement()){       //leemos la sección datos
        return salidaError();
    }


    if (stream->name() != "datos"){       //comprobamos el nombre
        return salidaError();
    }


    if (!stream->readNextStartElement()){       //leemos la sección fases
        return salidaError();
    }


    if (stream->name() != "fases"){       //comprobamos el nombre de la fase
        return salidaError();
    }

    qint32 tamFas = stream->attributes()[0].value().toString().toDouble();


    QPointF datosFas;

    if (!stream->readNextStartElement()){       //leemos la sección de la x
        return salidaError();
    }

    datosFas.setX(stream->readElementText().toDouble());


    if (!stream->readNextStartElement()){       //leemos la sección de la y
        return salidaError();
    }

    datosFas.setY(stream->readElementText().toDouble());

    stream->skipCurrentElement();

    if (!stream->readNextStartElement()){       //leemos la sección magnitudes
        return salidaError();
    }

    if (stream->name() != "magnitudes"){       //comprobamos el nombre de la magnitud
        return salidaError();
    }

    qint32 tamMag = stream->attributes()[0].value().toString().toDouble();

    QPointF datosMag;

    if (!stream->readNextStartElement()){       //leemos la sección de la x
        return salidaError();
    }

    datosMag.setX(stream->readElementText().toDouble());


    if (!stream->readNextStartElement()){       //leemos la sección de la y
        return salidaError();
    }

    datosMag.setY(stream->readElementText().toDouble());

    stream->skipCurrentElement();

    if (!stream->readNextStartElement()){       //leemos la sección metaBound
        return salidaError();
    }

    if (stream->name() != "meta-datos"){       //comprobamos el nombre de la metabound
        return salidaError();
    }

    QVector <bool> * metaDatosArriba = new QVector <bool> ();
    QVector <bool> * metaDatosAbierta = new QVector <bool> ();

    metaDatosAbierta = leerVectorBool();
    metaDatosArriba = leerVectorBool();

    stream->skipCurrentElement(); //cerramos los meta datos

    if (!stream->readNextStartElement()){       //leemos la sección bound
        return salidaError();
    }

    if (stream->name() != "bound"){       //comprobamos el nombre del bound
        return salidaError();
    }


    QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * boundaries =
            new QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > ();

    qint32 lon = stream->attributes()[0].value().toString().toDouble();

    for (qint32 i = 0; i < lon; i++){

        QMap <QString, QVector <QVector <QPointF> * > *> * mapa = new QMap <QString, QVector
                <QVector <QPointF> * > *> ();

        if (!leerFrecuencias(mapa)){
            return salidaError();
        }

        boundaries->append(mapa);
    }

    stream->skipCurrentElement(); //cerramos los boundaries

    //Leemos los boundaries reunidos
    QVector <QVector <QPointF> * > * boundariesreunidos = new QVector <QVector <QPointF> * > ();

    if (!leerVectorVectorPunto(boundariesreunidos)){
        return salidaError();
    }


    if (!stream->readNextStartElement()){       //leemos la sección bound reunidos matriz
        return salidaError();
    }

    if (stream->name() != "bound-reunidos-hash"){       //comprobamos el nombre del bound-reunidos
        return salidaError();
    }


    QVector <QVector <QVector <QPointF> * > * > * boundariesreunidoshash = new QVector <QVector <QVector <QPointF> * > *>  ();

    lon = stream->attributes()[0].value().toString().toDouble();

    for (qint32 i = 0; i < lon; i++){

        QVector <QVector <QPointF> * > * vec = new QVector <QVector <QPointF> * > ();

        if (!leerVectorVectorPunto(vec)){
            return salidaError();
        }

        boundariesreunidoshash->append(vec);
    }

    stream->skipCurrentElement(); //cerramos los boundaries reunidos hash

    bound = new DatosBound(boundaries, metaDatosAbierta, metaDatosArriba, tamFas, datosFas, boundariesreunidos,
                           boundariesreunidoshash, tamMag, datosMag);

    stream->skipCurrentElement(); //cerramos datos
    stream->skipCurrentElement(); //cerramos boundaries

    return true;
}


inline QVector <bool> *  XmlParserLoad::strtobool (QString cadena){

    QList <QString> listacadenas = cadena.split(" ");

    QVector <bool> * vec = new QVector <bool> ();
    bool ok = false;

    foreach (const QString &i, listacadenas){
        if (i.compare("") != 0){
            vec->append(i.toDouble(&ok));
            if (ok != true){
                return NULL;
            }else{
                ok = false;
            }
        }
    }

    return vec;
}

inline bool XmlParserLoad::leerTemplates(){

    if (!stream->readNextStartElement()){       //leemos la sección metadatos
        isTemplates = false;
        return salidaError();
    }


    if (stream->name() != "meta-datos"){
        isTemplates = false;
        return salidaError();
    }

    if (!stream->readNextStartElement()){       //leemos la sección metadatos
        isTemplates = false;
        return salidaError();
    }

    epsilon = srtovectorReal(stream->readElementText());

    stream->skipCurrentElement();

    if (!stream->readNextStartElement()){       //leemos la sección completo
        isTemplates = false;
        return salidaError();
    }

    if (stream->name() != "completo"){
        isTemplates = false;
        return salidaError();
    }

    qint32 longitudCompleto = stream->attributes()[0].value().toString().toDouble();

    templates = new QVector <QVector <std::complex <qreal>> * > ();

    templates->reserve(longitudCompleto);

    for (qint32 j = 0; j < longitudCompleto; j++){

        QVector <std::complex <qreal>> * vector = new QVector <std::complex <qreal>> ();

        if (!leerVectorComplejos(vector)){
            isTemplates = false;
            return false;
        }

        templates->append(vector);

    }
    isTemplates = true;
    stream->skipCurrentElement();

    if (!stream->readNextStartElement()){       //leemos la sección contorno
        isContorno = false;
        return salidaError();
    }

    if (stream->name() != "contorno"){
        isContorno = false;
        return salidaError();
    }

    qint32 longitudContorno = stream->attributes()[0].value().toString().toDouble();

    contorno = new QVector <QVector <std::complex <qreal>> * > ();

    contorno->reserve(longitudCompleto);

    for (qint32 j = 0; j < longitudContorno; j++){

        QVector <std::complex <qreal>> * vector = new QVector <std::complex <qreal>> ();

        if (!leerVectorComplejos(vector)){
            isContorno = false;
            return false;
        }

        contorno->append(vector);

    }

    isContorno = true;

    stream->skipCurrentElement();
    stream->skipCurrentElement();

    return true;
}

inline bool XmlParserLoad::leerVectorComplejos(QVector<std::complex <qreal> > *vector){

    if (!stream->readNextStartElement()){       //leemos la sección del primer vector
        return salidaError();
    }

    QVector<qreal> * vectorReales = srtovectorReal(stream->readElementText());

    if (!stream->readNextStartElement()){       //leemos la sección del segundo vector
        return salidaError();
    }

    //std::cout << stream->readElementText().toStdString() << std::endl;

    QVector<qreal> * vectorImaginarios = srtovectorReal(stream->readElementText());


    vector->reserve(vectorImaginarios->size());

    for (qint32 i = 0; i < vectorReales->size(); i++){
        vector->append(std::complex <qreal> (vectorReales->at(i), vectorImaginarios->at(i)));
    }

    vectorImaginarios->clear();
    vectorReales->clear();

    return true;
}

inline bool XmlParserLoad::leerOmega(){

    bool noError;

    if (!stream->readNextStartElement()){       //leemos la sección inicio
        return salidaError();
    }

    if (stream->name() != "inicio"){
        return salidaError();
    }

    qreal inicio = stream->readElementText().toDouble(&noError);

    if (!noError){
        return salidaError();
    }

    if (!stream->readNextStartElement()){       //leemos la sección final
        return salidaError();
    }

    if (stream->name() != "final"){
        return salidaError();
    }

    qreal final = stream->readElementText().toDouble(&noError);

    if (!noError){
        return salidaError();
    }

    if (!stream->readNextStartElement()){       //leemos la sección nPuntos
        return salidaError();
    }

    if (stream->name() != "nPuntos"){
        return salidaError();
    }

    qint32 nPuntos = stream->readElementText().toDouble(&noError);

    if (!noError){
        return salidaError();
    }

    if (!stream->readNextStartElement()){       //leemos la sección tipo
        return salidaError();
    }

    if (stream->name() != "tipo"){
        return salidaError();
    }

    tiposOmega tipo = (tiposOmega) stream->readElementText().toInt();

    if (!stream->readNextStartElement()){       //leemos la sección valores
        return salidaError();
    }

    if (stream->name() != "valores"){
        return salidaError();
    }

    QVector<qreal> * valores = srtovectorReal(stream->readElementText());

    stream->skipCurrentElement();

    omega = new Omega (inicio, final, nPuntos, valores, tipo);

    return true;
}

inline bool XmlParserLoad::leerEspecificaciones(){

    bool noError;

    QString lon (stream->attributes()[0].value().toString()); //leemos su atributo

    qint32 longitud = lon.toInt(&noError);

    if (!noError){
        return salidaError();
    }

    especificaciones = new QVector <dBND * > ();

    for (qint32 i = 0; i < longitud; i++){

        QString nombre;
        bool utilizado = false;
        QString altura;
        bool constante = false;
        Sistema * sis = NULL;
        qreal iniciofrec = 0;
        qreal finalfrec = 0;


        if (!stream->readNextStartElement()){       //leemos la sección especificacion
            salidaError();
            return NULL;
        }

        nombre = stream->attributes()[0].value().toString(); //leemos su atributo

        if (!stream->readNextStartElement()){       //leemos dentro de la especificación.
            salidaError();
            return NULL;
        }

        QString util (stream->readElementText()); //leemos el booleano utilizado

        if (util == "true"){
            utilizado = true;

            if (!stream->readNextStartElement()){       //nos situamos en el inicio frec
                salidaError();
                return NULL;
            }

            iniciofrec = stream->readElementText().toDouble(&noError);

            if (!noError){
                return salidaError();
            }

            if (!stream->readNextStartElement()){       //nos situamos en el inicio frec
                salidaError();
                return NULL;
            }

            finalfrec = stream->readElementText().toDouble(&noError);

            if (!noError){
                return salidaError();
            }

            if (!stream->readNextStartElement()){       //nos situamos en el booleano
                salidaError();
                return NULL;
            }

            QString cons (stream->readElementText()); //leemos el booleano costante

            if (cons == "true"){
                constante = true;

                if (!stream->readNextStartElement()){       //Nos situamos en la altura
                    salidaError();
                    return NULL;
                }
                altura = stream->readElementText(); //leemos la altura

                if (!noError){
                    return salidaError();
                }
                stream->skipCurrentElement(); //Saltamos el resto de valores vacíos
            }else {

                if (!stream->readNextStartElement()){       //Nos situamos en la altura
                    salidaError();
                    return NULL;
                }

                stream->readElementText();

                if (!stream->readNextStartElement()){       //Nos situamos en la planta
                    salidaError();
                    return NULL;
                }

                bool noError;

                QString nombrePlanta (stream->attributes()[0].value().toString()); //leemos su atributo

                if (!stream->readNextStartElement()){       //leemos la sección tipo
                    return salidaError();
                }

                if (stream->name() != "tipo"){
                    return salidaError();
                }

                Sistema::tipo_planta tipoPlanta = static_cast <Sistema::tipo_planta> (stream->attributes()[0].value().toInt()); //leemos su tipo


                if (!stream->readNextStartElement()){       //leemos la sección expresion
                    return salidaError();
                }

                if (stream->name() != "expresion"){
                    return salidaError();
                }


                qint32 longitudexpresion = stream->attributes()[0].value().toString().toDouble(&noError); //leemos la longitud del numerador.

                if (!noError){
                    return salidaError();
                }


                QString exp_nume;
                QString exp_deno;

                if (longitudexpresion == 2){
                    if (!stream->readNextStartElement()){       //leemos la expresion del numerador
                        salidaError();
                        return NULL;
                    }

                    exp_nume = stream->readElementText();

                    if (!stream->readNextStartElement()){       //leemos la expresion del numerador
                        salidaError();
                        return NULL;
                    }

                    exp_deno = stream->readElementText();
                }

                stream->skipCurrentElement();

                if (!stream->readNextStartElement()){       //leemos la sección numerador //PETA
                    return salidaError();
                }

                if (stream->name() != "numerador"){
                    return salidaError();
                }


                qint32 longitudNumerador = stream->attributes()[0].value().toString().toDouble(&noError); //leemos la longitud del numerador.


                if (!noError){
                    return salidaError();
                }

                QVector <Var*> * numerador = new QVector <Var*>();
                numerador->reserve(longitudNumerador);

                for (qint32 i = 0; i < longitudNumerador; i++){

                    Var * var = leerVariable(0);

                    if(var == NULL){
                        return false;
                    }

                    numerador->append(var);
                }

                stream->skipCurrentElement();



                if (!stream->readNextStartElement()){       //leemos la sección denominador
                    return salidaError();
                }

                if (stream->name() != "denominador"){
                    return salidaError();
                }


                qint32 longitudDenominador = stream->attributes()[0].value().toString().toDouble(&noError); //leemos la longitud del numerador.


                if (!noError){
                    return salidaError();
                }

                QVector <Var*> * denominador = new QVector <Var*>();
                numerador->reserve(longitudDenominador);

                for (qint32 i = 0; i < longitudDenominador; i++){
                    Var * var = leerVariable(0);

                    if(var == NULL){
                        return false;
                    }

                    denominador->append(var);
                }

                stream->skipCurrentElement();

                Var * k = leerVariable(0);

                if(k == NULL){
                    return false;
                }

                Var * ret = leerVariable(0);

                if(ret == NULL){
                    return false;
                }

                if (tipoPlanta == Sistema::cof_polinomios){
                    sis = new CPolinomios (nombrePlanta, numerador,denominador,k,ret);
                }else if (tipoPlanta == Sistema::k_ganancia){
                    sis = new KGanancia(nombrePlanta,numerador,denominador,k,ret);
                }else if (tipoPlanta == Sistema::k_no_ganancia){
                    sis = new KNGanancia(nombrePlanta,numerador,denominador,k,ret);
                }else if (tipoPlanta == Sistema::formato_libre) {
                    sis = new FormatoLibre(nombrePlanta,numerador,denominador,k,ret, exp_nume, exp_deno);
                }else{
                    return salidaError();
                }

                stream->skipCurrentElement();
                stream->skipCurrentElement();
                stream->skipCurrentElement();
            }

        } else{
            stream->skipCurrentElement();
        }

        dBND * estructura = new dBND ();

        estructura->nombre = nombre;
        estructura->utilizado = utilizado;
        estructura->sistema = sis;
        estructura->altura = altura.toDouble();
        estructura->constante = constante;
        estructura->frecinicio = iniciofrec;
        estructura->frecfinal = finalfrec;

        especificaciones->append (estructura);
    }

    stream->skipCurrentElement(); //saltamos la terminación de las especificaciones

    return true;
}


inline bool XmlParserLoad::leerPlanta(qint32 tipoLectura){

    bool noError;

    QString nombrePlanta (stream->attributes()[0].value().toString()); //leemos su atributo

    if (!stream->readNextStartElement()){       //leemos la sección tipo
        return salidaError();
    }

    if (stream->name() != "tipo"){
        return salidaError();
    }

    Sistema::tipo_planta tipoPlanta = static_cast<Sistema::tipo_planta>(stream->attributes()[0].value().toInt()); //leemos su tipo


    if (!stream->readNextStartElement()){       //leemos la sección expresion
        return salidaError();
    }

    if (stream->name() != "expresion"){
        return salidaError();
    }


    qint32 longitudexpresion = stream->attributes()[0].value().toString().toDouble(&noError); //leemos la longitud del numerador.

    if (!noError){
        return salidaError();
    }


    QString exp_nume;
    QString exp_deno;

    if (longitudexpresion == 2){
        if (!stream->readNextStartElement()){       //leemos la expresion del numerador
            salidaError();
            return NULL;
        }

        exp_nume = stream->readElementText();

        if (!stream->readNextStartElement()){       //leemos la expresion del numerador
            salidaError();
            return NULL;
        }

        exp_deno = stream->readElementText();
    }

    stream->skipCurrentElement();

    if (!stream->readNextStartElement()){       //leemos la sección numerador
        return salidaError();
    }

    if (stream->name() != "numerador"){
        return salidaError();
    }


    qint32 longitudNumerador = stream->attributes()[0].value().toString().toDouble(&noError); //leemos la longitud del numerador.


    if (!noError){
        return salidaError();
    }

    QVector <Var*> * numerador = new QVector <Var*>();
    numerador->reserve(longitudNumerador);

    for (qint32 i = 0; i < longitudNumerador; i++){

        Var * var = leerVariable(tipoLectura);

        if(var == NULL){
            return false;
        }

        numerador->append(var);
    }

    stream->skipCurrentElement();



    if (!stream->readNextStartElement()){       //leemos la sección denominador
        return salidaError();
    }

    if (stream->name() != "denominador"){
        return salidaError();
    }


    qint32 longitudDenominador = stream->attributes()[0].value().toString().toDouble(&noError); //leemos la longitud del numerador.


    if (!noError){
        return salidaError();
    }

    QVector <Var*> * denominador = new QVector <Var*>();
    numerador->reserve(longitudDenominador);

    for (qint32 i = 0; i < longitudDenominador; i++){
        Var * var = leerVariable(tipoLectura);

        if(var == NULL){
            return false;
        }

        denominador->append(var);
    }

    stream->skipCurrentElement();

    Var * k = leerVariable(tipoLectura);

    if(k == NULL){
        return false;
    }

    Var * ret = leerVariable(0);

    if(ret == NULL){
        return false;
    }

    if (tipoLectura == 0){
        if (tipoPlanta == Sistema::cof_polinomios){
            planta = new CPolinomios (nombrePlanta, numerador,denominador,k,ret);
        }else if (tipoPlanta == Sistema::k_ganancia){
            planta = new KGanancia(nombrePlanta,numerador,denominador,k,ret);
        }else if (tipoPlanta == Sistema::k_no_ganancia){
            planta = new KNGanancia(nombrePlanta,numerador,denominador,k,ret);
        }else if (tipoPlanta == Sistema::formato_libre) {
            planta = new FormatoLibre(nombrePlanta,numerador,denominador,k,ret, exp_nume, exp_deno);
        }else{
            return salidaError();
        }
    }else {
        if (tipoPlanta == Sistema::cof_polinomios){
            controlador = new CPolinomios (nombrePlanta, numerador,denominador,k,ret);
        }else if (tipoPlanta == Sistema::k_ganancia){
            controlador = new KGanancia(nombrePlanta,numerador,denominador,k,ret);
        }else if (tipoPlanta == Sistema::k_no_ganancia){
            controlador = new KNGanancia(nombrePlanta,numerador,denominador,k,ret);
        }else if (tipoPlanta == Sistema::formato_libre) {
            planta = new FormatoLibre(nombrePlanta,numerador,denominador,k,ret, exp_nume, exp_deno);
        }else{
            return salidaError();
        }
    }

    stream->skipCurrentElement();
    stream->skipCurrentElement();

    return true;
}

inline Var *XmlParserLoad::leerVariable(qint32 tipoLectura){
    bool noError;

    Var * var;

    if (!stream->readNextStartElement()){       //leemos la sección variable
        salidaError();
        return NULL;
    }

    if (!stream->readNextStartElement()){       //leemos dentro de la variable
        salidaError();
        return NULL;
    }

    qreal nominal = stream->readElementText().toDouble(&noError);

    if (!noError){
        salidaError();
        return NULL;
    }

    if (!stream->readNextStartElement()){       //leemos dentro de la variable
        salidaError();
        return NULL;
    }

    if (tipoLectura == 0){

        if (stream->readElementText() == "false"){
            var = new Var (nominal);
        }else {

            if (!stream->readNextStartElement()){       //leemos dentro de la variable
                salidaError();
                return NULL;
            }

            QString nombreVar (stream->readElementText());

            if (!stream->readNextStartElement()){       //leemos dentro de la variable
                salidaError();
                return NULL;
            }

            QString exp (stream->readElementText());

            if (!stream->readNextStartElement()){       //leemos la sección rango
                salidaError();
                return NULL;
            }

            if (stream->name().toString() != "rango"){
                salidaError();
                return NULL;
            }

            if (!stream->readNextStartElement()){       //leemos dentro del rango
                salidaError();
                return NULL;
            }

            qreal inicio = stream->readElementText().toDouble(&noError);
            if (!noError){
                salidaError();
                return NULL;
            }

            if (!stream->readNextStartElement()){       //leemos dentro del rango
                salidaError();
                return NULL;
            }

            qreal final = stream->readElementText().toDouble(&noError);
            if (!noError){
                salidaError();
                return NULL;
            }

            stream->skipCurrentElement();

            var = new Var (nombreVar, QPointF(inicio, final), nominal, exp);
        }
    } else {
        if (stream->readElementText() == "false"){
            var = new Var (nominal);

            bool salta = false;

            if (!stream->readNextStartElement()){       //leemos dentro de la variable
                salta = true;
            }

            if (!salta) {

                QString nombreVar (stream->readElementText());

                if (!stream->readNextStartElement()){       //leemos dentro de la variable
                    salidaError();
                    return NULL;
                }

                QString exp (stream->readElementText());

                if (!stream->readNextStartElement()){       //leemos la sección rango
                    salidaError();
                    return NULL;
                }

                if (stream->name().toString() != "rango"){
                    salidaError();
                    return NULL;
                }

                if (!stream->readNextStartElement()){       //leemos dentro del rango
                    salidaError();
                    return NULL;
                }

                qreal inicio = stream->readElementText().toDouble(&noError);
                if (!noError){
                    salidaError();
                    return NULL;
                }

                if (!stream->readNextStartElement()){       //leemos dentro del rango
                    salidaError();
                    return NULL;
                }

                qreal final = stream->readElementText().toDouble(&noError);
                if (!noError){
                    salidaError();
                    return NULL;
                }

                stream->skipCurrentElement();
                var->setRango(QPointF(inicio, final));
            }

        }else {

            if (!stream->readNextStartElement()){       //leemos dentro de la variable
                salidaError();
                return NULL;
            }

            QString nombreVar (stream->readElementText());

            if (!stream->readNextStartElement()){       //leemos dentro de la variable
                salidaError();
                return NULL;
            }

            QString exp (stream->readElementText());

            if (!stream->readNextStartElement()){       //leemos la sección rango
                salidaError();
                return NULL;
            }

            if (stream->name().toString() != "rango"){
                salidaError();
                return NULL;
            }

            if (!stream->readNextStartElement()){       //leemos dentro del rango
                salidaError();
                return NULL;
            }

            qreal inicio = stream->readElementText().toDouble(&noError);
            if (!noError){
                salidaError();
                return NULL;
            }

            if (!stream->readNextStartElement()){       //leemos dentro del rango
                salidaError();
                return NULL;
            }

            qreal final = stream->readElementText().toDouble(&noError);
            if (!noError){
                salidaError();
                return NULL;
            }

            stream->skipCurrentElement();

            var = new Var (nombreVar, QPointF(inicio, final), nominal, exp);
        }
    }

    stream->skipCurrentElement();

    return var;
}

inline bool XmlParserLoad::leerLoopShaping(){
    bool noError;

    if (!stream->readNextStartElement()){       //leemos la sección tipo
        return salidaError();
    }

    if (stream->name() != "datos"){
        return salidaError();
    }

    qint32 nPuntos = stream->attributes()[0].value().toDouble();

    if (!stream->readNextStartElement()){       //leemos la sección datos
        return salidaError();
    }

    qreal x = stream->readElementText().toDouble(&noError);
    if (!noError){
        return salidaError();
    }

    if (!stream->readNextStartElement()){       //leemos dentro de datos la a
        return salidaError();
    }

    qreal y = stream->readElementText().toDouble(&noError);
    if (!noError){
        return salidaError();
    }

    stream->skipCurrentElement();


    if (!stream->readNextStartElement()){
        //Empezamos a leer la planta.
        salidaError();
        return NULL;
    }

    QString nombrePlanta (stream->attributes()[0].value().toString()); //leemos su atributo



    if (!stream->readNextStartElement()){       //leemos la sección tipo
        return salidaError();
    }

    if (stream->name() != "tipo"){
        return salidaError();
    }

    Sistema::tipo_planta tipoPlanta = static_cast<Sistema::tipo_planta>(stream->attributes()[0].value().toInt()); //leemos su tipo


    if (!stream->readNextStartElement()){       //leemos la sección expresion
        return salidaError();
    }

    if (stream->name() != "expresion"){
        return salidaError();
    }


    qint32 longitudexpresion = stream->attributes()[0].value().toString().toDouble(&noError); //leemos la longitud del numerador.

    if (!noError){
        return salidaError();
    }


    QString exp_nume;
    QString exp_deno;

    if (longitudexpresion == 2){
        if (!stream->readNextStartElement()){       //leemos la expresion del numerador
            salidaError();
            return NULL;
        }

        exp_nume = stream->readElementText();

        if (!stream->readNextStartElement()){       //leemos la expresion del numerador
            salidaError();
            return NULL;
        }

        exp_deno = stream->readElementText();
    }

    stream->skipCurrentElement();

    if (!stream->readNextStartElement()){       //leemos la sección numerador
        return salidaError();
    }

    if (stream->name() != "numerador"){
        return salidaError();
    }


    qint32 longitudNumerador = stream->attributes()[0].value().toString().toDouble(&noError); //leemos la longitud del numerador.


    if (!noError){
        return salidaError();
    }

    QVector <Var*> * numerador = new QVector <Var*>();
    numerador->reserve(longitudNumerador);

    for (qint32 i = 0; i < longitudNumerador; i++){

        Var * var = leerVariable(0);

        if(var == NULL){
            return false;
        }

        numerador->append(var);
    }

    stream->skipCurrentElement();



    if (!stream->readNextStartElement()){       //leemos la sección denominador
        return salidaError();
    }

    if (stream->name() != "denominador"){
        return salidaError();
    }


    qint32 longitudDenominador = stream->attributes()[0].value().toString().toDouble(&noError); //leemos la longitud del numerador.


    if (!noError){
        return salidaError();
    }

    QVector <Var*> * denominador = new QVector <Var*>();
    numerador->reserve(longitudDenominador);

    for (qint32 i = 0; i < longitudDenominador; i++){
        Var * var = leerVariable(0);

        if(var == NULL){
            return false;
        }

        denominador->append(var);
    }

    stream->skipCurrentElement();

    Var * k = leerVariable(0);

    if(k == NULL){
        return false;
    }

    Var * ret = leerVariable(0);

    if(ret == NULL){
        return false;
    }

    Sistema * sistema;

    if (tipoPlanta == Sistema::cof_polinomios){
        sistema = new CPolinomios (nombrePlanta, numerador,denominador,k,ret);
    }else if (tipoPlanta == Sistema::k_ganancia){
        sistema = new KGanancia(nombrePlanta,numerador,denominador,k,ret);
    }else if (tipoPlanta == Sistema::k_no_ganancia){
        sistema = new KNGanancia(nombrePlanta,numerador,denominador,k,ret);
    }else if (tipoPlanta == Sistema::formato_libre) {
        sistema = new FormatoLibre(nombrePlanta,numerador,denominador,k,ret, exp_nume, exp_deno);
    }else{
        return salidaError();
    }

    stream->skipCurrentElement();
    stream->skipCurrentElement();

    stream->skipCurrentElement();

    loopShaping = new DatosLoopShaping (sistema, QPointF(x, y), nPuntos);

    return true;
}

inline bool XmlParserLoad::salidaError(){
    cout << stream->lineNumber() << endl;
    menerror("Hay un error en el fichero\n de entrada en la línea: " + stream->lineNumber(),"Cargar Fichero");
    return false;
}
