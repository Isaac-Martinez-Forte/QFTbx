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

//Everything the sweeps and the search COMPUTE is a function of the inputs
//below it, so publishing a new input has to drop what was computed from the
//old one. Nothing invalidated anything before this: entering a new plant on
//top of a finished design left the old templates in place, and
//computeBoundaries() then combined the NEW plant with the OLD templates and
//produced boundaries for a system that never existed - silently, because
//every pointer was valid.
//
//Inputs never invalidate other inputs, only computed artefacts. That also
//keeps load() working: it assigns in dependency order (plant,
//specifications, omega, templates, boundaries, controller, loop shaping), so
//each step only ever drops things that have not been set yet.
void ProjectController::dropTemplatesAndBelow(){
    data.setTemplates({});
    data.setContour({});
    data.setEpsilon(nullptr);

    dropBoundariesAndBelow();
}

void ProjectController::dropBoundariesAndBelow(){
    data.setBoundaries(std::nullopt);

    dropLoopShaping();
}

void ProjectController::dropLoopShaping(){
    data.setLoopShapingResult(nullptr);
}

void ProjectController::setPlant(LtiSystem *plant){
    //Identity is a no-op, as in the store: re-publishing the same object is
    //not a change and must not throw away a finished design.
    if (data.plant() == plant){
        return;
    }

    data.setPlant(plant);
    dropTemplatesAndBelow();
}

void ProjectController::setOmega(Omega *omega){
    if (data.omega() == omega){
        return;
    }

    data.setOmega(omega);
    dropTemplatesAndBelow();
}

void ProjectController::setSpecifications(QVector<qftbx::SpecificationRecord *> *specifications){
    if (data.specifications() == specifications){
        return;
    }

    data.setSpecifications(specifications);

    //The templates do not depend on the specifications; the boundaries do.
    dropBoundariesAndBelow();
}

void ProjectController::setTemplates(qftbx::CloudSet clouds, qftbx::CloudSet contour,
                                     bool hasContour){

    if (m_templateEngine == nullptr){
        m_templateEngine = new TemplateEngine();
    }

    //Feed the engine too: without this, recomputing the contour after
    //LOADING a project had nothing to walk. Both hold their own copy, which
    //is the price of the aliasing going away.
    m_templateEngine->setClouds(clouds);

    data.setTemplates(std::move(clouds));

    if (hasContour){
        data.setContour(std::move(contour));
    }

    //New templates make the boundaries built from the old ones meaningless.
    //Called from computeTemplates() too, which is why the epsilon is not
    //dropped here: the computation stores it right after.
    dropBoundariesAndBelow();
}

void ProjectController::setContour(qftbx::CloudSet contour){
    data.setContour(std::move(contour));
}

void ProjectController::setBoundaries(std::optional<qftbx::BoundaryData> boundaries){
    data.setBoundaries(std::move(boundaries));

    //The search runs against these boundaries: a new set voids its result.
    dropLoopShaping();
}

const qftbx::CloudSet & ProjectController::templates(){
    return data.templates();
}

const qftbx::CloudSet & ProjectController::contour(){
    return data.contour();
}

bool ProjectController::computeTemplates(QVector <qreal> * epsilon, qftbx::ParameterGrids grids, bool cuda){

    //Preconditions, stated instead of dereferenced. They matter more now that
    //publishing an input DROPS what was computed from the old one: without
    //them a step whose inputs have just been invalidated would walk a null
    //pointer instead of saying what is missing.
    if (data.plant() == nullptr){
        throw qftbx::InvalidInput("The templates need a plant.");
    }
    if (data.omega() == nullptr){
        throw qftbx::InvalidInput("The templates need a set of design frequencies.");
    }

    if (m_templateEngine == nullptr){
        m_templateEngine = new TemplateEngine();
    }

    m_templateEngine->setEpsilon(epsilon);
    m_templateEngine->setGrids(std::move(grids));

    m_templateEngine->compute(plant(), omega()->values(), cuda);

    //The computation no longer reorders or replaces the frequencies: it is
    //enough to keep the epsilon used, for the persistence.
    data.setEpsilon(epsilon);

    const bool produced = !m_templateEngine->clouds().empty()
            && !m_templateEngine->contours().empty();

    setTemplates(m_templateEngine->clouds(), m_templateEngine->contours(),
                 !m_templateEngine->contours().empty());

    return produced;
}

QVector <qreal> * ProjectController::epsilon(){
    return data.epsilon();
}


const qftbx::CloudSet & ProjectController::recomputeContour(QVector <qreal> * epsilon){
    m_templateEngine->computeContours(epsilon);

    setContour(m_templateEngine->contours());
    data.setEpsilon(epsilon);

    return data.contour();
}

bool ProjectController::computeBoundaries(QPointF phaseRange, qint32 phaseCount, QPointF magnitudeRange,
                                     qint32 magnitudeCount, qreal exportInfinity, bool contour, bool cuda){

    if (data.plant() == nullptr || data.frequencies() == nullptr){
        throw qftbx::InvalidInput("The boundaries need a plant and a set of "
                                  "design frequencies.");
    }
    if (data.specifications() == nullptr){
        throw qftbx::InvalidInput("The boundaries need the specifications.");
    }
    if ((contour ? data.contour() : data.templates()).empty()){
        throw qftbx::InvalidInput("The boundaries need the templates, which "
                                  "have to be recomputed after the plant or "
                                  "the design frequencies change.");
    }

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

const qftbx::UnionTraces & ProjectController::unionBoundaries(){
    return data.boundaries()->unionBoundaries();
}

const qftbx::UnionBuckets & ProjectController::unionBuckets(){
    return data.boundaries()->unionBuckets();
}

bool ProjectController::setControllerStructure(LtiSystem *controller){
    if (data.controller() == controller){
        return true;
    }

    data.setController(controller);

    //A different controller structure means the computed controller answers a
    //question nobody asked any more.
    dropLoopShaping();

    return true;
}

LtiSystem * ProjectController::controllerStructure(){
    return data.controller();
}

bool ProjectController::computeLoopShaping(qreal epsilon, tools::LoopShapingAlgorithm algorithm, QPointF plotRange, qreal pointCount,
                                      qint32 initialisation){

    if (data.plant() == nullptr || data.frequencies() == nullptr){
        throw qftbx::InvalidInput("The loop shaping needs a plant and a set of "
                                  "design frequencies.");
    }
    if (data.controller() == nullptr){
        throw qftbx::InvalidInput("The loop shaping needs a controller structure.");
    }
    if (data.boundaries() == nullptr){
        throw qftbx::InvalidInput("The loop shaping needs the boundaries, which "
                                  "have to be recomputed after the plant, the "
                                  "design frequencies, the specifications or "
                                  "the templates change.");
    }

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

std::vector<bool> ProjectController::load(QString fichero){

    ProjectReader reader;

    const std::vector<bool> flags = reader.load(fichero);

    if (flags.at(0))
        setPlant(reader.takePlant());

    if (flags.at(1))
        setSpecifications(reader.takeSpecifications());

    if (flags.at(2))
        setOmega(reader.takeOmega());

    if (flags.at(3)){
        setTemplates(reader.takeTemplates(),
                    flags.at(7) ? reader.takeContour() : qftbx::CloudSet(),
                    flags.at(7));
        data.setEpsilon(reader.takeEpsilon());
    }

    if (flags.at(4))
        setBoundaries(reader.takeBoundaries());

    if (flags.at(5)){
        setControllerStructure(reader.takeController());
    }

    if (flags.at(6)){
        setLoopShapingResult(reader.takeLoopShaping());
    }

    return flags;
}
