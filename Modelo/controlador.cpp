#include "controlador.h"

Controlador::Controlador()
{
}

Controlador::~Controlador(){

    delete bound;
    delete templates;
    delete loopShaping;
}

LtiSystem *Controlador::getPlanta(){
    return data.plant();
}

Omega * Controlador::getOmega(){
    return data.omega();
}

QVector <qreal> * Controlador::getFrecuencias(){
    return data.frequencies();
}


QVector<qftbx::SpecificationRecord *> *Controlador::getEspecificaciones(){
    return data.specifications();
}

void Controlador::setPlanta(LtiSystem *planta){
    data.setPlant(planta);
}

void Controlador::setValues(Omega *omega){
    data.setOmega(omega);
}

void Controlador::setEspecificaciones(QVector<qftbx::SpecificationRecord *> *espe){
    data.setSpecifications(espe);
}

void Controlador::setTemplate(QVector<QVector<std::complex<qreal> > *> *temp,
                              QVector<QVector<std::complex<qreal> > *> *contorno, bool isContorno){

    if (templates == nullptr){
        templates = new TemplateEngine();
    }

    data.setTemplates(temp);

    //Feed the engine too: without this, recomputing the contour after
    //LOADING a project dereferenced a null pointer.
    templates->setClouds(temp);

    if (isContorno){
        data.setContour(contorno);
    }
}

void Controlador::setContorno(QVector<QVector<std::complex<qreal> > *> *contorno){
    data.setContour(contorno);
}

void Controlador::setBoundaries(BoundaryData *bound){
    data.setBoundaries(bound);
}

QVector <QVector <std::complex <qreal> > * > * Controlador::getTemplate(){
    return data.templates();
}

QVector <QVector <std::complex <qreal> > * > * Controlador::getContorno(){
    return data.contour();
}

bool Controlador::calcularTemplates(QVector <qreal> * epsilon, QHash <QString, QVector<qreal> *> *mapa, bool cuda){

    if (templates == nullptr){
        templates = new TemplateEngine();
    }

    templates->setEpsilon(epsilon);
    templates->setGrids(mapa);

    templates->compute(getPlanta(), getOmega()->values(), cuda);

    //The computation no longer reorders or replaces the frequencies: it is
    //enough to keep the epsilon used, for the persistence.
    data.setEpsilon(epsilon);

    QVector <QVector <std::complex <qreal> > * > * temp = templates->clouds();
    QVector <QVector <std::complex <qreal> > * > * cont = templates->contours();

    setTemplate(temp, cont, cont != nullptr);

    return temp != nullptr && cont != nullptr;
}

QVector <qreal> * Controlador::getEpsilon(){
    return data.epsilon();
}


QVector <QVector <std::complex <qreal> > * > * Controlador::recalcularContorno(QVector <qreal> * epsilon){
    templates->computeContours(epsilon);
    QVector <QVector <std::complex <qreal> > * > * aux = templates->contours();

    setContorno(aux);
    data.setEpsilon(epsilon);

    return aux;
}

bool Controlador::calcularBoundaries(QPointF datosFas, qint32 puntosFas, QPointF datosMag,
                                     qint32 puntosMag, qreal infinito, bool contorno, bool cuda){

    if (bound == nullptr){
        bound = new BoundaryEngine();
    }

    bound->compute(data.frequencies(), data.plant(),
                   contorno ? data.contour() : data.templates(),
                   data.specifications(), datosFas, puntosFas, datosMag, puntosMag,
                   infinito, cuda);

    setBoundaries(bound->boundaryData());

    getOmega()->setValues(bound->omega());

    return true;
}

BoundaryData *Controlador::getBound(){
    return data.boundaries();
}

QVector< QVector<QPointF> * > * Controlador::unionBoundaries(){
    return data.boundaries()->unionBoundaries();
}

QVector< QVector <QVector<QPointF> * > * > * Controlador::unionBuckets(){
    return data.boundaries()->unionBuckets();
}

bool Controlador::setControlador(LtiSystem *controlador){
    data.setController(controlador);

    return true;
}

LtiSystem * Controlador::getControlador(){
    return data.controller();
}

bool Controlador::calcularLoopShaping(qreal epsilon, tools::LoopShapingAlgorithm seleccionado, QPointF rango, qreal nPuntos,
                                      qint32 inicializacion){

    if (loopShaping == nullptr){
        loopShaping = new LoopShaping();
    }

    const bool re = loopShaping->iniciar(data.plant(), data.controller(), data.frequencies(),
                                         data.boundaries(), epsilon, seleccionado,
                                         data.contour(), data.specifications(),
                                         inicializacion);

    if (re){
        data.setLoopShaping(new DatosLoopShaping(loopShaping->getControlador(), rango, nPuntos));
        return true;
    }

    return false;
}

void Controlador::setLoopShaping(DatosLoopShaping *datos){
    data.setLoopShaping(datos);
}

DatosLoopShaping * Controlador::getLoopShaping(){
    return data.loopShaping();
}

bool Controlador::guardarSistema(QString fichero){

    ProjectContent content;

    content.plant = data.plant();
    content.specifications = data.specifications();
    content.omega = data.omega();
    content.templates = data.templates();
    content.epsilon = data.epsilon();

    if (data.hasContour()){
        content.contour = data.contour();
    }

    content.boundaries = data.boundaries();
    content.controller = data.controller();
    content.loopShaping = data.loopShaping();

    ProjectWriter writer;
    writer.save(fichero, content);

    return true;
}

QVector <bool> * Controlador::cargarSistema(QString fichero){

    ProjectReader leer;

    QVector <bool> * retorno = leer.load(fichero);

    if (retorno->value(0))
        setPlanta(leer.takePlant());

    if (retorno->value(1))
        setEspecificaciones(leer.takeSpecifications());

    if (retorno->value(2))
        setValues(leer.takeOmega());

    if (retorno->value(3)){
        setTemplate(leer.takeTemplates(),
                    retorno->value(7) ? leer.takeContour() : nullptr,
                    retorno->value(7));
        data.setEpsilon(leer.takeEpsilon());
    }

    if (retorno->value(4))
        setBoundaries(leer.takeBoundaries());

    if (retorno->value(5)){
        setControlador(leer.takeController());
    }

    if (retorno->value(6)){
        setLoopShaping(leer.takeLoopShaping());
    }

    return retorno;
}
