#include <string>
#include <vector>
#include <cstdint>
#include "src/core/project_controller.h"

#include "src/core/math/expression_cache.h"
#include "src/persistence/project_reader.h"
#include "src/persistence/project_writer.h"

ProjectController::ProjectController() = default;

ProjectController::~ProjectController() = default;

LtiSystem *ProjectController::plant(){
    return m_data.plant();
}

Omega * ProjectController::omega(){
    return m_data.omega();
}


qftbx::SpecificationRecords * ProjectController::specifications(){
    return m_data.specifications();
}

namespace {

//Every parameter name of a system that is about to be published has to be a
//name muParserX can bind, because that is what the sweeps and the search do
//with it. Six single letters are reserved as SI unit postfix operators -
//n, u, m, k, M, G - and "k" is the canonical name for a gain, so this is not
//a theoretical collision: naming a controller gain "k" used to throw a
//mup::ParserError from deep inside the search, which is neither a
//qftbx::Exception nor a std::exception, so it escaped the window's catch and
//took the application down.
//Checked HERE, once per publish, and not in Parameter's constructor: the
//search deep-copies parameters for every box it bisects, and that is not a
//place to ask a parser anything.
void requireUsableNames(LtiSystem & system)
{
    const auto check = [](const Parameter & parameter) {
        if (!parameter.isUncertain()) {
            //A constant carries no variable into any expression; its name is
            //often the number itself.
            return;
        }
        if (!qftbx::math::isUsableVariableName(parameter.name())) {
            throw qftbx::InvalidInput(
                "\"" + parameter.name() + "\" cannot be used as a parameter "
                "name: the expression parser reserves it. The single letters "
                "n, u, m, k, M and G are its unit multipliers, so a gain has "
                "to be called something else - kv, for instance.");
        }
    };

    for (Parameter & parameter : system.numerator()) { check(parameter); }
    for (Parameter & parameter : system.denominator()) { check(parameter); }
    check(system.gain());
    check(system.delay());
}

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
    m_data.setTemplates({});
    m_data.setContour({});
    m_data.setEpsilon(std::nullopt);

    dropBoundariesAndBelow();
}

void ProjectController::dropBoundariesAndBelow(){
    m_data.setBoundaries(std::nullopt);

    dropLoopShaping();
}

void ProjectController::dropLoopShaping(){
    m_data.setLoopShapingResult(nullptr);
}

//The three publishers used to compare the incoming pointer with the stored
//one and return early, so that handing the same object over twice would not
//throw away a finished design. Taking the ownership makes that comparison
//not just unnecessary but wrong: returning early would destroy, on the way
//out, the very object the store is pointing at.
//A null publish is refused, not taken as "remove the step": there is no such
//step in the pipeline, and a reused dialog once published a null plant that
//wiped the plant and everything computed from it. The facade is where that
//stops, whatever the interface does.
bool ProjectController::setPlant(std::unique_ptr<LtiSystem> plant){
    requireNotComputing();

    if (plant == nullptr) {
        throw qftbx::InvalidInput("The project cannot take a null plant.");
    }
    requireUsableNames(*plant);

    const bool changed = m_data.plant() == nullptr || !plant->sameAs(*m_data.plant());

    m_data.setPlant(std::move(plant));

    if (changed) {
        dropTemplatesAndBelow();
    }

    return changed;
}

bool ProjectController::setOmega(std::unique_ptr<Omega> omega){
    requireNotComputing();

    if (omega == nullptr) {
        throw qftbx::InvalidInput("The project cannot take a null set of design frequencies.");
    }

    const bool changed = m_data.omega() == nullptr || !omega->sameAs(*m_data.omega());

    m_data.setOmega(std::move(omega));

    if (changed) {
        dropTemplatesAndBelow();
    }

    return changed;
}

void ProjectController::setSpecifications(std::optional<qftbx::SpecificationRecords> specifications){
    requireNotComputing();

    if (!specifications.has_value()) {
        throw qftbx::InvalidInput("The project cannot take an empty set of specifications.");
    }

    m_data.setSpecifications(std::move(specifications));

    //The templates do not depend on the specifications; the boundaries do.
    dropBoundariesAndBelow();
}

void ProjectController::setTemplates(qftbx::CloudSet clouds, qftbx::CloudSet contour,
                                     bool hasContour){

    m_templates.adopt(m_data, std::move(clouds), std::move(contour), hasContour);

    //New templates make the boundaries built from the old ones meaningless.
    //The stage does not do this: the dependency graph stays here, in the one
    //class that owns it. Only load() comes through here now; computeTemplates()
    //applies the same drop itself.
    dropBoundariesAndBelow();
}

void ProjectController::setBoundaries(std::optional<qftbx::BoundaryData> boundaries){
    m_data.setBoundaries(std::move(boundaries));

    //The search runs against these boundaries: a new set voids its result.
    dropLoopShaping();
}

const qftbx::CloudSet & ProjectController::templates(){
    return m_data.templates();
}

const qftbx::CloudSet & ProjectController::contour(){
    return m_data.contour();
}

bool ProjectController::computeTemplates(std::vector <double> epsilon, qftbx::ParameterGrids grids, bool cuda){

    requireNotComputing();

    //The preconditions, the engine and the publishing live in the stage now.
    //What stays here is the dependency graph, and it has to be applied
    //EXPLICITLY: this used to reach it through setTemplates, and routing the
    //publishing through the stage instead silently stopped dropping the
    //boundaries. Nothing caught it, which is now covered by
    //StageSequence.RecomputingTheTemplatesDropsTheBoundaries.
    const bool produced = m_templates.run(m_data, std::move(epsilon),
                                          std::move(grids), cuda);

    //New templates make the boundaries built from the old ones meaningless.
    dropBoundariesAndBelow();

    return produced;
}

std::vector <double> * ProjectController::epsilon(){
    return m_data.epsilon();
}


const qftbx::CloudSet & ProjectController::recomputeContour(std::vector <double> epsilon){
    //It rewrites the contour and the epsilon, and MR reads the contour: the
    //one mutating entry point that was missing the guard.
    requireNotComputing();

    return m_templates.recomputeContour(m_data, std::move(epsilon));
}

bool ProjectController::computeBoundaries(qftbx::Range phaseRange, std::int32_t phaseCount, qftbx::Range magnitudeRange,
                                     std::int32_t magnitudeCount, double exportInfinity, bool contour, bool cuda){

    requireNotComputing();

    const bool produced = m_boundaries.run(m_data, phaseRange, phaseCount,
                                           magnitudeRange, magnitudeCount,
                                           exportInfinity, contour, cuda);

    //The search runs against these boundaries: a new set voids its result.
    //Applied here and not in the stage, for the same reason as the
    //templates - the dependency graph lives in one place. And stated
    //explicitly, because reaching it through setBoundaries() is exactly what
    //stopped happening when the templates moved into their own stage.
    dropLoopShaping();

    return produced;
}

BoundaryData *ProjectController::boundaries(){
    return m_data.boundaries();
}

//Every other getter answers nullptr while its step is not done; these two
//return references and cannot, so they say so instead of dereferencing a null.
const qftbx::UnionTraces & ProjectController::unionBoundaries(){
    if (m_data.boundaries() == nullptr) {
        throw qftbx::InvalidInput("There are no boundaries yet.");
    }
    return m_data.boundaries()->unionBoundaries();
}

const qftbx::UnionBuckets & ProjectController::unionBuckets(){
    if (m_data.boundaries() == nullptr) {
        throw qftbx::InvalidInput("There are no boundaries yet.");
    }
    return m_data.boundaries()->unionBuckets();
}

bool ProjectController::setControllerStructure(std::unique_ptr<LtiSystem> controller){
    requireNotComputing();

    if (controller == nullptr) {
        throw qftbx::InvalidInput("The project cannot take a null controller structure.");
    }
    requireUsableNames(*controller);

    const bool changed = m_data.controller() == nullptr || !controller->sameAs(*m_data.controller());

    m_data.setController(std::move(controller));

    //A different controller structure means the computed controller answers a
    //question nobody asked any more. The same one means it still answers it.
    if (changed) {
        dropLoopShaping();
    }

    return changed;
}

LtiSystem * ProjectController::controllerStructure(){
    return m_data.controller();
}

bool ProjectController::computeLoopShaping(double epsilon, tools::LoopShapingAlgorithm algorithm, qftbx::Range plotRange, double pointCount,
                                      std::int32_t initialisation,
                                      const qftbx::CancellationToken * cancellation){

    requireNotComputing();

    //Nothing downstream to invalidate: this result IS the design.
    return m_loopShaping.run(m_data, epsilon, algorithm, plotRange, pointCount,
                             initialisation, cancellation);
}

//A computation in flight has the project data to itself. Refusing is the
//point of the worker living here: the alternative is documenting that callers
//must not touch anything and hoping, which is how data races are written.
void ProjectController::requireNotComputing() const
{
    if (m_background.running()) {
        throw qftbx::InvalidInput("A computation is running: cancel it or wait "
                                  "for it before changing the project.");
    }
}

bool ProjectController::startLoopShaping(double epsilon, tools::LoopShapingAlgorithm algorithm,
                                         qftbx::Range plotRange, double pointCount,
                                         std::int32_t initialisation,
                                         std::function<void ()> finished)
{
    if (m_background.running()) {
        return false;
    }

    //Checked on THIS thread, before anything starts: a search with no
    //boundaries is the caller's mistake and it should surface where the caller
    //made it, not as a message field on a worker that has already returned.
    m_loopShaping.requirePrerequisites(m_data);

    m_cancellation.reset();

    return m_background.start(
        [this, epsilon, algorithm, plotRange, pointCount, initialisation]() {
            return m_loopShaping.run(m_data, epsilon, algorithm, plotRange,
                                     pointCount, initialisation, &m_cancellation);
        },
        std::move(finished));
}

void ProjectController::cancelComputation()
{
    m_cancellation.cancel();
}

bool ProjectController::isComputing() const
{
    return m_background.running();
}

void ProjectController::waitForComputation()
{
    m_background.wait();
}

bool ProjectController::lastComputationProduced() const
{
    return m_background.produced();
}

bool ProjectController::lastComputationCancelled() const
{
    return m_background.cancelled();
}

const std::string & ProjectController::lastComputationError() const
{
    return m_background.error();
}

void ProjectController::setLoopShapingResult(std::unique_ptr<LoopShapingResult> result){
    m_data.setLoopShapingResult(std::move(result));
}

LoopShapingResult * ProjectController::loopShapingResult(){
    return m_data.loopShaping();
}

void ProjectController::save(std::string path){
    //Reading, but reading a project a worker is writing the design into.
    requireNotComputing();


    ProjectContent content;

    content.plant = m_data.plant();
    content.specifications = m_data.specifications();
    content.omega = m_data.omega();
    content.templates = &m_data.templates();
    content.epsilon = m_data.epsilon();

    if (m_data.hasContour()){
        content.contour = &m_data.contour();
    }

    content.boundaries = m_data.boundaries();
    content.controller = m_data.controller();
    content.loopShaping = m_data.loopShaping();

    ProjectWriter writer;
    writer.save(path, content);
}

qftbx::StepSet ProjectController::load(std::string path){
    requireNotComputing();

    ProjectReader reader;

    const ProjectReader::Loaded loaded = reader.load(path);

    //The file REPLACES the project; it does not overlay it. Publishing only the
    //steps the file carries on top of whatever was here left a hybrid - a
    //five-step file opened over a finished design kept the old controller
    //structure and the old search result under the new plant - and the window
    //then reported seven steps done for a file that had five. Read first, so a
    //file that fails to parse leaves the current project untouched; then
    //start clean.
    m_data = qftbx::ProjectData();

    //Assigned in dependency order, which is what keeps the invalidation from
    //undoing the step before: each publisher only ever drops things that have
    //not been set yet.
    if (loaded.steps.has(qftbx::Step::Plant)) {
        setPlant(reader.takePlant());
    }

    if (loaded.steps.has(qftbx::Step::Specifications)) {
        setSpecifications(reader.takeSpecifications());
    }

    if (loaded.steps.has(qftbx::Step::Frequencies)) {
        setOmega(reader.takeOmega());
    }

    if (loaded.steps.has(qftbx::Step::Templates)) {
        setTemplates(reader.takeTemplates(),
                     loaded.hasContour ? reader.takeContour() : qftbx::CloudSet(),
                     loaded.hasContour);
        m_data.setEpsilon(reader.takeEpsilon());
    }

    if (loaded.steps.has(qftbx::Step::Boundaries)) {
        setBoundaries(reader.takeBoundaries());
    }

    if (loaded.steps.has(qftbx::Step::Controller)) {
        setControllerStructure(reader.takeController());
    }

    if (loaded.steps.has(qftbx::Step::LoopShaping)) {
        setLoopShapingResult(reader.takeLoopShaping());
    }

    return loaded.steps;
}

//DERIVED, not stored. Each of these is exactly what a "step done" flag meant,
//and the flags were duplicate state: state that can be duplicated is state
//that can go out of sync, and the window kept seven of them by hand.
void ProjectController::applySettings(const qftbx::Settings & settings)
{
    m_loopShaping.setSettings(settings);
}

qftbx::StepSet ProjectController::completed() const
{
    qftbx::StepSet done;

    if (m_data.plant() != nullptr)                { done.add(qftbx::Step::Plant); }
    if (m_data.specifications() != nullptr)       { done.add(qftbx::Step::Specifications); }
    if (m_data.omega() != nullptr)                { done.add(qftbx::Step::Frequencies); }
    if (!m_data.templates().empty())              { done.add(qftbx::Step::Templates); }
    if (m_data.boundaries() != nullptr)           { done.add(qftbx::Step::Boundaries); }
    if (m_data.controller() != nullptr)           { done.add(qftbx::Step::Controller); }
    if (m_data.loopShaping() != nullptr)          { done.add(qftbx::Step::LoopShaping); }

    return done;
}

//One implementation of the cascade instead of three functions calling each
//other. The three are kept as the names the rest of this class already uses,
//so nothing else had to change to gain this.
void ProjectController::invalidateFrom(qftbx::Step step)
{
    switch (step) {
    case qftbx::Step::Plant:
    case qftbx::Step::Frequencies:
    case qftbx::Step::Templates:
        dropTemplatesAndBelow();
        return;
    //The templates do not depend on the specifications; the boundaries do.
    //Grouping this step with the plant, as the first version did, dropped
    //the templates for a change that never touches them - and disagreed
    //with setSpecifications(), which had it right.
    case qftbx::Step::Specifications:
    case qftbx::Step::Boundaries:
        dropBoundariesAndBelow();
        return;
    case qftbx::Step::Controller:
    case qftbx::Step::LoopShaping:
        dropLoopShaping();
        return;
    }
}
