#include "src/core/project_controller.h"

ProjectController::ProjectController()
{
}

ProjectController::~ProjectController(){

    delete m_boundaryEngine;
    delete m_templateEngine;
    delete m_loopShapingEngine;
}

LtiSystem *ProjectController::plant(){
    return data.plant();
}

Omega * ProjectController::omega(){
    return data.omega();
}

QVector <qreal> * ProjectController::frequencies(){
    return data.frequencies();
}


QVector<qftbx::SpecificationRecord *> *ProjectController::specifications(){
    return data.specifications();
}

void ProjectController::setPlant(LtiSystem *plant){
    data.setPlant(plant);
}

void ProjectController::setOmega(Omega *omega){
    data.setOmega(omega);
}

void ProjectController::setSpecifications(QVector<qftbx::SpecificationRecord *> *specifications){
    data.setSpecifications(specifications);
}

void ProjectController::setTemplates(QVector<QVector<std::complex<qreal> > *> *clouds,
                              QVector<QVector<std::complex<qreal> > *> *contour, bool hasContour){

    if (m_templateEngine == nullptr){
        m_templateEngine = new TemplateEngine();
    }

    data.setTemplates(clouds);

    //Feed the engine too: without this, recomputing the contour after
    //LOADING a project dereferenced a null pointer.
    m_templateEngine->setClouds(clouds);

    if (hasContour){
        data.setContour(contour);
    }
}

void ProjectController::setContour(QVector<QVector<std::complex<qreal> > *> *contour){
    data.setContour(contour);
}

void ProjectController::setBoundaries(BoundaryData *m_boundaryEngine){
    data.setBoundaries(m_boundaryEngine);
}

QVector <QVector <std::complex <qreal> > * > * ProjectController::templates(){
    return data.templates();
}

QVector <QVector <std::complex <qreal> > * > * ProjectController::contour(){
    return data.contour();
}

bool ProjectController::computeTemplates(QVector <qreal> * epsilon, QHash <QString, QVector<qreal> *> *grids, bool cuda){

    if (m_templateEngine == nullptr){
        m_templateEngine = new TemplateEngine();
    }

    m_templateEngine->setEpsilon(epsilon);
    m_templateEngine->setGrids(grids);

    m_templateEngine->compute(plant(), omega()->values(), cuda);

    //The computation no longer reorders or replaces the frequencies: it is
    //enough to keep the epsilon used, for the persistence.
    data.setEpsilon(epsilon);

    QVector <QVector <std::complex <qreal> > * > * clouds = m_templateEngine->clouds();
    QVector <QVector <std::complex <qreal> > * > * contours = m_templateEngine->contours();

    setTemplates(clouds, contours, contours != nullptr);

    return clouds != nullptr && contours != nullptr;
}

QVector <qreal> * ProjectController::epsilon(){
    return data.epsilon();
}


QVector <QVector <std::complex <qreal> > * > * ProjectController::recomputeContour(QVector <qreal> * epsilon){
    m_templateEngine->computeContours(epsilon);
    QVector <QVector <std::complex <qreal> > * > * contours = m_templateEngine->contours();

    setContour(contours);
    data.setEpsilon(epsilon);

    return contours;
}

bool ProjectController::computeBoundaries(QPointF phaseRange, qint32 phaseCount, QPointF magnitudeRange,
                                     qint32 magnitudeCount, qreal exportInfinity, bool contour, bool cuda){

    if (m_boundaryEngine == nullptr){
        m_boundaryEngine = new BoundaryEngine();
    }

    m_boundaryEngine->compute(data.frequencies(), data.plant(),
                   contour ? data.contour() : data.templates(),
                   data.specifications(), phaseRange, phaseCount, magnitudeRange, magnitudeCount,
                   exportInfinity, cuda);

    setBoundaries(m_boundaryEngine->boundaryData());

    omega()->setOmega(m_boundaryEngine->omega());

    return true;
}

BoundaryData *ProjectController::boundaries(){
    return data.boundaries();
}

QVector< QVector<QPointF> * > * ProjectController::unionBoundaries(){
    return data.boundaries()->unionBoundaries();
}

QVector< QVector <QVector<QPointF> * > * > * ProjectController::unionBuckets(){
    return data.boundaries()->unionBuckets();
}

bool ProjectController::setControllerStructure(LtiSystem *controller){
    data.setController(controller);

    return true;
}

LtiSystem * ProjectController::controllerStructure(){
    return data.controller();
}

bool ProjectController::computeLoopShaping(qreal epsilon, tools::LoopShapingAlgorithm algorithm, QPointF plotRange, qreal pointCount,
                                      qint32 initialisation){

    if (m_loopShapingEngine == nullptr){
        m_loopShapingEngine = new LoopShaping();
    }

    const bool succeeded = m_loopShapingEngine->run(data.plant(), data.controller(), data.frequencies(),
                                         data.boundaries(), epsilon, algorithm,
                                         data.contour(), data.specifications(),
                                         initialisation);

    if (succeeded){
        data.setLoopShapingResult(new LoopShapingResult(m_loopShapingEngine->controllerStructure(), plotRange, pointCount));
        return true;
    }

    return false;
}

void ProjectController::setLoopShapingResult(LoopShapingResult *datos){
    data.setLoopShapingResult(datos);
}

LoopShapingResult * ProjectController::loopShapingResult(){
    return data.loopShaping();
}

bool ProjectController::save(QString fichero){

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

QVector <bool> * ProjectController::load(QString fichero){

    ProjectReader reader;

    QVector <bool> * flags = reader.load(fichero);

    if (flags->value(0))
        setPlant(reader.takePlant());

    if (flags->value(1))
        setSpecifications(reader.takeSpecifications());

    if (flags->value(2))
        setOmega(reader.takeOmega());

    if (flags->value(3)){
        setTemplates(reader.takeTemplates(),
                    flags->value(7) ? reader.takeContour() : nullptr,
                    flags->value(7));
        data.setEpsilon(reader.takeEpsilon());
    }

    if (flags->value(4))
        setBoundaries(reader.takeBoundaries());

    if (flags->value(5)){
        setControllerStructure(reader.takeController());
    }

    if (flags->value(6)){
        setLoopShapingResult(reader.takeLoopShaping());
    }

    return flags;
}
