/**
 * @file
 * @brief Implementation of the project facade.
 *
 * Inputs never invalidate other inputs, only computed artefacts: the plant
 * and the frequencies drop the templates and everything under them, the
 * specifications drop the boundaries, the controller structure drops the
 * design. That order lets a file be assigned section by section, each
 * publisher dropping only what is not set yet; the file is read whole before
 * the project is replaced, so one that fails to parse leaves the current
 * project untouched. Uncertain parameter names are checked against the
 * expression grammar once per publish, since the search copies parameters
 * for every box it bisects. The background starters check their
 * prerequisites on the caller's thread, where the mistake was made.
 */

#include <string>
#include <vector>
#include <cstdint>
#include "src/app/project_controller.h"

#include "src/core/math/expression_tree.h"
#include "src/persistence/project_reader.h"
#include "src/persistence/project_writer.h"

namespace qftbx {

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

void requireUsableNames(LtiSystem & system)
{
    const auto check = [](const Parameter & parameter) {
        if (!parameter.isUncertain()) {
            return;
        }
        if (!ExpressionTree::isUsableVariableName(parameter.name())) {
            throw qftbx::InvalidInput(QFTBX_TR("Core", "\"%1\" cannot be used as a parameter name: it is a function, a constant (pi, e) or the Laplace variable s of the expression grammar, or not an identifier.").arg(parameter.name()));
        }
    };

    for (Parameter & parameter : system.numerator()) { check(parameter); }
    for (Parameter & parameter : system.denominator()) { check(parameter); }
    check(system.gain());
    check(system.delay());
}

}

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

bool ProjectController::setPlant(std::unique_ptr<LtiSystem> plant){
    const Announce announce(*this);
    requireNotComputing();

    if (plant == nullptr) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "The project cannot take a null plant."));
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
    const Announce announce(*this);
    requireNotComputing();

    if (omega == nullptr) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "The project cannot take a null set of design frequencies."));
    }

    const bool changed = m_data.omega() == nullptr || !omega->sameAs(*m_data.omega());

    m_data.setOmega(std::move(omega));

    if (changed) {
        dropTemplatesAndBelow();
    }

    return changed;
}

void ProjectController::setSpecifications(std::optional<qftbx::SpecificationRecords> specifications){
    const Announce announce(*this);
    requireNotComputing();

    if (!specifications.has_value()) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "The project cannot take an empty set of specifications."));
    }

    m_data.setSpecifications(std::move(specifications));

    dropBoundariesAndBelow();
}

void ProjectController::setTemplates(qftbx::CloudSet clouds, qftbx::CloudSet contour,
                                     bool hasContour){
    const Announce announce(*this);

    m_templates.adopt(m_data, std::move(clouds), std::move(contour), hasContour);

    dropBoundariesAndBelow();
}

void ProjectController::setBoundaries(std::optional<qftbx::BoundaryData> boundaries){
    const Announce announce(*this);

    m_data.setBoundaries(std::move(boundaries));

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

    const Announce announce(*this);

    const bool produced = m_templates.run(m_data, std::move(epsilon),
                                          std::move(grids), cuda);

    dropBoundariesAndBelow();

    return produced;
}

std::vector <double> * ProjectController::epsilon(){
    return m_data.epsilon();
}

qftbx::EpsilonMetric ProjectController::epsilonMetric() const{
    return m_data.epsilonMetric();
}

void ProjectController::setEpsilonMetric(qftbx::EpsilonMetric metric){
    m_data.setEpsilonMetric(metric);
}

std::vector<TemplateEngine::EpsilonProposal> ProjectController::proposeEpsilon(){
    return m_templates.proposeEpsilon(m_data);
}

void ProjectController::setWholeCloudStandsIn(bool standsIn){
    m_templates.setWholeCloudStandsIn(standsIn);
}

void ProjectController::setAlphaShapeContour(bool alphaShape){
    m_templates.setAlphaShapeContour(alphaShape);
}

bool ProjectController::alphaShapeContour() const{
    return m_templates.alphaShapeContour();
}

void ProjectController::setBorderSweep(bool border){
    m_templates.setBorderSweep(border);
}

bool ProjectController::borderSweep() const{
    return m_templates.borderSweep();
}

void ProjectController::setClosedFormColumns(bool on){
    m_boundaries.setClosedFormColumns(on);
}

bool ProjectController::closedFormColumns() const{
    return m_boundaries.closedFormColumns();
}

const std::vector<TemplateEngine::ContourReport> & ProjectController::contourReports() const{
    return m_templates.contourReports();
}

std::vector<TemplateEngine::EpsilonProposal> ProjectController::proposeEpsilon(qftbx::ParameterGrids grids,
                                                                               qftbx::EpsilonMetric metric){
    requireNotComputing();
    return m_templates.proposeEpsilon(m_data, std::move(grids), metric);
}

const qftbx::CloudSet & ProjectController::recomputeContour(std::vector <double> epsilon){
    requireNotComputing();

    const Announce announce(*this);

    return m_templates.recomputeContour(m_data, std::move(epsilon));
}

bool ProjectController::computeBoundaries(qftbx::Range phaseRange, std::int32_t phaseCount, qftbx::Range magnitudeRange,
                                     std::int32_t magnitudeCount, double exportInfinity, bool contour, bool cuda){

    requireNotComputing();

    const Announce announce(*this);

    const bool produced = m_boundaries.run(m_data, phaseRange, phaseCount,
                                           magnitudeRange, magnitudeCount,
                                           exportInfinity, contour, cuda);

    dropLoopShaping();

    return produced;
}

BoundaryData *ProjectController::boundaries(){
    return m_data.boundaries();
}

const qftbx::UnionTraces & ProjectController::unionBoundaries(){
    if (m_data.boundaries() == nullptr) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "There are no boundaries yet."));
    }
    return m_data.boundaries()->unionBoundaries();
}

const qftbx::UnionBuckets & ProjectController::unionBuckets(){
    if (m_data.boundaries() == nullptr) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "There are no boundaries yet."));
    }
    return m_data.boundaries()->unionBuckets();
}

bool ProjectController::setControllerStructure(std::unique_ptr<LtiSystem> controller){
    const Announce announce(*this);
    requireNotComputing();

    if (controller == nullptr) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "The project cannot take a null controller structure."));
    }
    requireUsableNames(*controller);

    const bool changed = m_data.controller() == nullptr || !controller->sameAs(*m_data.controller());

    m_data.setController(std::move(controller));

    if (changed) {
        dropLoopShaping();
    }

    return changed;
}

LtiSystem * ProjectController::controllerStructure(){
    return m_data.controller();
}

bool ProjectController::computeLoopShaping(double epsilon, qftbx::LoopShapingAlgorithm algorithm, qftbx::Range plotRange, double pointCount,
                                      std::int32_t initialisation,
                                      const qftbx::CancellationToken * cancellation){

    requireNotComputing();

    const Announce announce(*this);

    return m_loopShaping.run(m_data, epsilon, algorithm, plotRange, pointCount,
                             initialisation, cancellation);
}

void ProjectController::requireNotComputing() const
{
    if (m_background.running()) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "A computation is running: cancel it or wait "
                                  "for it before changing the project."));
    }
}

bool ProjectController::startLoopShaping(double epsilon, qftbx::LoopShapingAlgorithm algorithm,
                                         qftbx::Range plotRange, double pointCount,
                                         std::int32_t initialisation,
                                         std::function<void ()> finished)
{
    if (m_background.running()) {
        return false;
    }

    m_loopShaping.requirePrerequisites(m_data);

    m_cancellation.reset();
    m_lastComputation = Computation::LoopShaping;

    return m_background.start(
        [this, epsilon, algorithm, plotRange, pointCount, initialisation]() {
            return m_loopShaping.run(m_data, epsilon, algorithm, plotRange,
                                     pointCount, initialisation, &m_cancellation);
        },
        std::move(finished));
}

bool ProjectController::startTemplates(std::vector<double> epsilon, qftbx::ParameterGrids grids,
                                      bool cuda, std::function<void ()> finished)
{
    if (m_background.running()) {
        return false;
    }

    m_templates.requirePrerequisites(m_data);

    m_cancellation.reset();

    m_lastComputation = Computation::Templates;

    return m_background.start(
        [this, epsilon = std::move(epsilon), grids = std::move(grids), cuda]() mutable {
            return m_templates.run(m_data, std::move(epsilon), std::move(grids),
                                   cuda, &m_cancellation);
        },
        std::move(finished));
}

bool ProjectController::startBoundaries(qftbx::Range phaseRange, std::int32_t phaseCount,
                                        qftbx::Range magnitudeRange, std::int32_t magnitudeCount,
                                        double exportInfinity, bool contour, bool cuda,
                                        std::function<void ()> finished)
{
    if (m_background.running()) {
        return false;
    }

    m_boundaries.requirePrerequisites(m_data, contour);

    m_cancellation.reset();

    m_lastComputation = Computation::Boundaries;

    return m_background.start(
        [this, phaseRange, phaseCount, magnitudeRange, magnitudeCount, exportInfinity,
         contour, cuda]() {
            return m_boundaries.run(m_data, phaseRange, phaseCount,
                                    magnitudeRange, magnitudeCount,
                                    exportInfinity, contour, cuda, &m_cancellation);
        },
        std::move(finished));
}

void ProjectController::collectComputation()
{
    if (m_background.running()) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "The computation has not finished yet."));
    }

    const Announce announce(*this);

    switch (m_lastComputation) {
    case Computation::Templates:
        dropBoundariesAndBelow();
        break;
    case Computation::Boundaries:
        dropLoopShaping();
        break;
    case Computation::LoopShaping:
    case Computation::None:
        break;
    }

    m_lastComputation = Computation::None;
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
    const Announce announce(*this);

    m_data.setLoopShapingResult(std::move(result));
}

LoopShapingResult * ProjectController::loopShapingResult(){
    return m_data.loopShaping();
}

void ProjectController::save(std::string path){
    requireNotComputing();

    ProjectContent content;

    content.plant = m_data.plant();
    content.specifications = m_data.specifications();
    content.omega = m_data.omega();
    content.templates = &m_data.templates();
    content.epsilon = m_data.epsilon();
    content.epsilonMetric = m_data.epsilonMetric();

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
    const Announce announce(*this);

    requireNotComputing();

    ProjectReader reader;

    const ProjectReader::Loaded loaded = reader.load(path);

    m_data = qftbx::ProjectData();

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
        m_data.setEpsilonMetric(reader.epsilonMetric());
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

void ProjectController::applySettings(const qftbx::Settings & settings)
{
    m_loopShaping.setSettings(settings);
    m_templates.setWholeCloudStandsIn(settings.algorithms.wholeTemplateIfNoContour);
    m_templates.setAlphaShapeContour(settings.algorithms.alphaShapeContour);
    m_templates.setBorderSweep(settings.algorithms.borderSweep);
    m_boundaries.setClosedFormColumns(settings.algorithms.closedFormColumns);
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

void ProjectController::invalidateFrom(qftbx::Step step)
{
    switch (step) {
    case qftbx::Step::Plant:
    case qftbx::Step::Frequencies:
    case qftbx::Step::Templates:
        dropTemplatesAndBelow();
        return;
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

}
