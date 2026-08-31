#include "parsersave.h"

#include "Modelo/Herramientas/exception.h"

using namespace tools;
using namespace std;

XmlParserSave::XmlParserSave()
{
    guardadoDatos = false;
}

XmlParserSave::~XmlParserSave(){
    if(guardadoDatos){
        delete stream;
    }
}


bool XmlParserSave::guardarXMLDatos(QString fichero, DatosPlanta *datosPlanta){///////////////////////////////////////


    QFile file(fichero);

    if (!file.open(QIODevice::WriteOnly)){
        throw qftbx::FileError("Cannot write project file: " + fichero.toStdString());
    }

    stream = new QXmlStreamWriter(&file);

    stream->setAutoFormatting(true);
    stream->writeStartDocument();
    stream->writeStartElement("QFT");

    guardadoDatos = true;

    QVector <bool> * existen = datosPlanta->getExisten();

    if (existen->at(0))
        guardarPlanta(datosPlanta->getPlanta()->getPlanta(), "Planta");

    if (existen->at(1))
        guardarEspecificaciones(datosPlanta->getEspecificaciones());

    if (existen->at(2))
        guardarOmega(datosPlanta->getOmega());

    if (existen->at(3))
        guardarTemplates(datosPlanta->getTemplates(), existen->at(4));

    if (existen->at(5)){
        guardarBoundaries(datosPlanta->getBoundaries());
    }

    if (existen->at(6))
        guardarPlanta(datosPlanta->getControlador()->getControlador(), "Controlador");

    if (existen->at(7))
        guardarLoopShaping(datosPlanta->getLoopShaping());

    stream->writeEndDocument();

    file.close();

    return true;
}

inline bool XmlParserSave::guardarPlanta(LtiSystem *planta, QString nombre){/////////////////////////////////////////////////////

    stream->writeStartElement(nombre);
    stream->writeAttribute("nombre", planta->name());

    stream->writeStartElement("tipo");
    stream->writeAttribute("tipo", QString::number(static_cast<qint32>(planta->type())));

    stream->writeStartElement("expresion");

    if (planta->type() == LtiSystem::SystemType::FreeForm){
        stream->writeAttribute("size", "2");

        stream->writeTextElement("numerador", planta->numeratorString());
        stream->writeTextElement("denominador", planta->denominatorString());

    } else {
        stream->writeAttribute("size", "0");
    }

    stream->writeEndElement();


    stream->writeStartElement("numerador");
    stream->writeAttribute("size", QString::number(planta->numerator()->size()));

    qint32 i = 0;
    foreach (Parameter * var, *planta->numerator()) {
        guardarVariable(var,QString::number(i));
        i++;
    }

    stream->writeEndElement();


    stream->writeStartElement("denominador");
    stream->writeAttribute("size", QString::number(planta->denominator()->size()));

    i = 0;
    foreach (Parameter * var, *planta->denominator()) {
        guardarVariable(var,QString::number(i));
        i++;
    }

    stream->writeEndElement();

    guardarVariable(planta->gain(),"k");
    guardarVariable(planta->delay(),"ret");

    stream->writeEndElement();
    stream->writeEndElement();
    return true;

}

inline void XmlParserSave::guardarVariable(Parameter *var, QString idem){
    stream->writeStartElement("variable-" + idem);
    stream->writeTextElement("nominal", QString::number(var->nominal()));

    if (var->isUncertain()) {
        stream->writeTextElement("variable", "true");
        stream->writeTextElement("nombre", var->name());
        stream->writeTextElement("exp", var->expression());
        stream->writeStartElement("rango");
        stream->writeTextElement("inicio", QString::number(var->range().x()));
        stream->writeTextElement("final", QString::number(var->range().y()));
        stream->writeEndElement();


    }else {
        stream->writeTextElement("variable", "false");
    }
    stream->writeEndElement();
}

inline void XmlParserSave::guardarOmega(OmegaDAO *omega){
    stream->writeStartElement("omega");
    stream->writeTextElement("inicio",  QString::number(omega->getOmega()->getInicio()));
    stream->writeTextElement("final",  QString::number(omega->getOmega()->getFinal()));
    stream->writeTextElement("nPuntos",  QString::number(omega->getOmega()->getNPuntos()));
    stream->writeTextElement("tipo",  QString::number(omega->getOmega()->getTipo()));
    stream->writeTextElement("valores",  vectorTString(omega->getFrecuencias()));
    stream->writeEndElement();
}

inline QString XmlParserSave::vectorTString(QVector <qreal> *vector){

    QString cadena;

    foreach (const qreal & valor, * vector) {

        cadena += QString::number(valor) + " ";

    }

    return cadena;

}


inline void XmlParserSave::guardarEspecificaciones(EspecificacionesDAO *especificaciones){ ///////////////////////////////////////////////

    stream->writeStartElement("especificaciones");
    stream->writeAttribute("longitud", QString::number(especificaciones->getEspecificaciones()->size()));

    QVector <tools::dBND *> * vec = especificaciones->getEspecificaciones();

    for (qint32 i = 0; i < vec->size(); i++){
        guardarEstructuraEspec(vec->at(i));
    }

    stream->writeEndElement();
}

inline void XmlParserSave::guardarEstructuraEspec(dBND *estructura){

    stream->writeStartElement("especificacion");
    stream->writeAttribute("nombre", estructura->nombre);

    if (estructura->utilizado){
        stream->writeTextElement("utilizado", "true");
    } else{
        stream->writeTextElement("utilizado", "false");
    }

    stream->writeTextElement("inicio-frec", QString::number(estructura->frecinicio));
    stream->writeTextElement("final-frec", QString::number(estructura->frecfinal));


    if (estructura->constante){
        stream->writeTextElement("constante", "true");
    } else{
        stream->writeTextElement("constante", "false");
    }

    stream->writeTextElement("altura", QString::number(estructura->altura));

    if (!estructura->constante && estructura->utilizado){
        guardarPlanta(estructura->sistema, estructura->sistema->name());
    }

    stream->writeEndElement();
}

inline void XmlParserSave::guardarTemplates(TemplateDAO *templates, bool isContorno){/////////////////////////////////////////////////////
    stream->writeStartElement("templates");

    stream->writeStartElement("meta-datos");
    stream->writeTextElement("epsilon", vectorTString(templates->getEpsilon()));
    stream->writeEndElement();

    stream->writeStartElement("completo");
    stream->writeAttribute("size", QString::number(templates->getTemplates()->size()));

    guardarVectorComplejo(templates->getTemplates());

    stream->writeEndElement();

    if (isContorno){
        stream->writeStartElement("contorno");
        stream->writeAttribute("size", QString::number(templates->getContorno()->size()));

        guardarVectorComplejo(templates->getContorno());

        stream->writeEndElement();
    }

    stream->writeEndElement();

}
inline void XmlParserSave::guardarVectorComplejo(QVector<QVector<complex<qreal> > *> *vectores){

    qint32 i = 0;
    foreach (QVector <complex <qreal> > * vector, *vectores) {

        QString parteReal;
        QString parteImaginaria;

        for (qint32 j = 0; j < vector->size(); j++){

            complex <qreal> complejo = vector->at(j);

            parteReal += QString::number(complejo.real()) + " ";
            parteImaginaria += QString::number(complejo.imag()) + " ";
        }

        stream->writeTextElement("vector-"+QString::number(i), parteReal);
        stream->writeTextElement("vector-"+QString::number(i) + "." +QString::number(i), parteImaginaria);

        i++;
    }

}

inline void XmlParserSave::guardarVector(QVector<QVector<qreal> *> *vectores){
    qint32 i = 0;
    foreach (QVector <qreal> * vector, *vectores) {

        stream->writeTextElement("vector-"+QString::number(i), vectorTString(vector));

        i++;
    }
}

inline void XmlParserSave::guardarPunto(QVector<QVector<QPointF> *> *vectores){
    qint32 i = 0;
    foreach (QVector <QPointF> * vector, *vectores) {

        QString cadena;

        foreach (QPointF punto, *vector) {
            cadena += QString::number(punto.x()) + " " + QString::number(punto.y()) + " ";
        }

        stream->writeTextElement("vector-"+QString::number(i), cadena);

        i++;
    }
}

inline void XmlParserSave::guardarVectores(QVector<QVector<QVector<qreal> *> *> * vectores){

    qint32 j = 0;
    foreach (QVector <QVector <qreal> * > * dobleVector, *vectores) {


        stream->writeStartElement("omega-" + QString::number(j));
        stream->writeAttribute("size", QString::number(dobleVector->size()));

        guardarVector(dobleVector);

        stream->writeEndElement();

        j++;
    }

}

inline void XmlParserSave::guardarPuntos(QVector<QVector<QVector<QPointF> *> *> * puntos){
    qint32 j = 0;
    foreach (QVector <QVector <QPointF> * > * dobleVector, *puntos) {


        stream->writeStartElement("omega-" + QString::number(j));
        stream->writeAttribute("size", QString::number(dobleVector->size()));

        guardarPunto(dobleVector);

        stream->writeEndElement();

        j++;
    }
}

inline void XmlParserSave::guardarEstructuraBound(QString nombre, QVector<QVector<QPointF> *> *datos){

    stream->writeStartElement(nombre); //Empezamos la sección de cada especificación
    stream->writeAttribute("size", QString::number(datos->size())); //numero de boundaries por especificación

    guardarPunto(datos);

    stream->writeEndElement(); //Cerramos cada especificacion

}

inline void XmlParserSave::guardarVectorBool(QVector<bool> *vec, QString nombre){
    QString s;
    foreach (bool b, *vec) {
        if (b){
            s += "1 ";
        }else{
            s += "0 ";
        }
    }

    stream->writeTextElement(nombre, s);
}

inline void XmlParserSave::guardarBoundaries(BoundDAO *boundaries){

    BoundaryData * datos = boundaries->getBound();

    QVector <QMap <QString, QVector <QVector <QPointF> * > * > * > * bound = datos->boundaries();
    QVector <QVector  <QPointF> * > * bound_reunidos = datos->unionBoundaries();
    QVector <QVector <QVector <QPointF> * > * > * bound_reunidos_hash = datos->unionBuckets();
    QVector <bool> * metaDatosArriba = datos->upperFlags();
    QVector <bool> * metaDatosAbierto = datos->openFlags();

    stream->writeStartElement("boundaries"); //Empezamos la sección boundaries


    stream->writeStartElement("datos"); //Datos preliminares

    stream->writeStartElement("fases"); //Datos fases
    stream->writeAttribute("tamFas", QString::number(datos->phaseCount())); //tamFas
    stream->writeTextElement("x", QString::number(datos->phaseRange().x()));
    stream->writeTextElement("y", QString::number(datos->phaseRange().y()));
    stream->writeEndElement();

    stream->writeStartElement("magnitudes"); //Datos magnitudes
    stream->writeAttribute("tamMag", QString::number(datos->magnitudeCount())); //tamMag
    stream->writeTextElement("x", QString::number(datos->magnitudeRange().x()));
    stream->writeTextElement("y", QString::number(datos->magnitudeRange().y()));
    stream->writeEndElement();

    //metadatos

    stream->writeStartElement("meta-datos");

    guardarVectorBool(metaDatosAbierto, "metaDatosABierto");
    guardarVectorBool(metaDatosArriba, "metaDatosArriba");

    stream->writeEndElement();

    stream->writeStartElement("bound"); //Empezamos la sección boundaries
    stream->writeAttribute("size", QString::number(bound->size())); //Número de frecuencias de diseño.

    for (qint32 i = 0; i < bound->size(); i++) {

        stream->writeStartElement("frecuencia-" + QString::number(i)); //número de frecuencia.

        QMap <QString, QVector <QVector <QPointF> * > * > * mapa = bound->at(i);
        QList <QString> claves = mapa->keys();

        stream->writeAttribute("size", QString::number(claves.size())); //Número de frecuencias de diseño.

        foreach (QString key, claves) {             //Recorremos las especificaciones.
            guardarEstructuraBound(key, mapa->value(key));
        }

        stream->writeEndElement(); //Cerramos cada frecuencia
    }

    stream->writeEndElement(); //Cerramos la sección de boundaries

    guardarEstructuraBound("bound-reunidos",bound_reunidos); //Guardamos los boundaries reunidos


    stream->writeStartElement("bound-reunidos-hash"); //Empezamos la sección boundaries reunidos
    stream->writeAttribute("size", QString::number(bound_reunidos_hash->size())); //Número de frecuencias de diseño.

    guardarPuntos(bound_reunidos_hash);

    stream->writeEndElement(); //Cerramos la sección de boundaries reunidos


    stream->writeEndElement();//Cerramos la sección completa.
    stream->writeEndElement();
}

inline void XmlParserSave::guardarLoopShaping(LoopShapingDAO *loopShaping){

    DatosLoopShaping * datos = loopShaping->getLoopShaping();

    stream->writeStartElement("LoopShaping"); //Abrimos LoopShaping

    stream->writeStartElement("datos"); //Datos de LoopShaping

    stream->writeAttribute("nPuntos", QString::number(datos->getNPuntos())); //tamFas
    stream->writeTextElement("x", QString::number(datos->range().x()));
    stream->writeTextElement("y", QString::number(datos->range().y()));

    stream->writeEndElement(); //Ceramos Datos

    if (!guardarPlanta(datos->getControlador(), "LoopShaping")){ //Guardamos la planta
        return;
    }

    stream->writeEndElement(); //Cerramos LoopShaping

}
