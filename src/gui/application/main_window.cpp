#include "src/gui/loopshaping/loop_boundaries_viewer.h"
#include "src/gui/application/main_window.h"
#include "src/gui/application/error_message.h"
#include "src/gui/common/plot_palette.h"
#include "ui_main_window.h"

#include <QFileDialog>
#include <QMessageBox>

#include "src/core/math/point.h"
#include "src/core/common/exception.h"
#include "src/core/pipeline/pipeline_step.h"

#include <mpParser.h>

#include <vector>


namespace qftbx {

namespace {

//The wait cursor as an object, so it comes back whatever way the scope ends.
//It was set and unset by hand, and the arrow appeared THREE times in the
//template handler alone - once per way out - which is a shape where any new
//return leaves the hourglass spinning over a window that is done working.
class WaitCursor
{
public:
    explicit WaitCursor(QWidget * widget) : m_widget(widget)
    {
        if (m_widget != nullptr) {
            m_widget->setCursor(Qt::WaitCursor);
        }
    }

    ~WaitCursor()
    {
        if (m_widget != nullptr) {
            m_widget->setCursor(Qt::ArrowCursor);
        }
    }

    WaitCursor(const WaitCursor &) = delete;
    WaitCursor & operator=(const WaitCursor &) = delete;

private:
    QWidget * m_widget;
};

}

MainWindow::MainWindow(qftbx::Settings settings, QWidget *parent) :
    QMainWindow(parent),
    ui(std::make_unique<Ui::MainWindow>()),
    m_settings(std::move(settings))
{
    
    ui->setupUi(this);
    setWindowTitle(tr("QFT: Quantitative feedback theory"));

    //7 real steps: with the 0-8 range the bar never reached 100%.
    ui->progressBar->setRange(0,7);

    createSession();
}

MainWindow::~MainWindow()
{
    destroySession();

}

void MainWindow::createSession(){

    controller = std::make_unique<ProjectController>();

    //What the core needs of the settings; the dialogs get theirs when they
    //are built.
    controller->applySettings(m_settings);

    //An empty project: every step undone, so this switches the buttons off
    //and puts the bar at zero without enumerating either. The seven flags it
    //used to reset by hand are gone, and two of their comments were CROSSED -
    //boundariesDone said "estructura del controller" and controllerDone said
    //"boundaries" - which is what hand-kept parallel state looks like after a
    //while.
    refreshAvailability();

    //Without this, Save after New overwrote the last opened file with the
    //empty project.
    saveFilePath.clear();
}

//Qt's own mechanism: every dialog and viewer here is a child of this
//window, and destroying one is how a new session gets a fresh one.
//
//Each delete used to be guarded by the step's own progress flag, which is
//also how the flag got out of step with the pointer. There is no flag left to
//consult: deleting a null pointer is a no-op, so the pointer decides.
void MainWindow::destroyDialogs(){
    delete plantDialog;
    plantDialog = nullptr;

    delete specificationsDialog;
    specificationsDialog = nullptr;

    delete frequenciesDialog;
    frequenciesDialog = nullptr;

    delete templatesDialog;
    templatesDialog = nullptr;
    delete templateViewer;
    templateViewer = nullptr;

    delete boundaryGridDialog;
    boundaryGridDialog = nullptr;
    delete boundaryViewer;
    boundaryViewer = nullptr;
    delete boundaryUnionViewer;
    boundaryUnionViewer = nullptr;

    delete controllerDialog;
    controllerDialog = nullptr;

    delete bodeViewer;
    bodeViewer = nullptr;

    delete loopShapingDialog;
    loopShapingDialog = nullptr;
    delete loopShapingViewer;
    loopShapingViewer = nullptr;
}

//The POINTER says whether a step's widgets exist, which is the question
//being asked. The flag it replaced answered a different one - whether the
//step was done - and the two only agreed by being maintained together.
void MainWindow::ensurePlantDialog()
{
    if (plantDialog == nullptr){
        plantDialog = new PlantDialog(this);
    }
}

void MainWindow::ensureSpecificationsDialog()
{
    if (specificationsDialog == nullptr){
        specificationsDialog = new SpecificationsDialog(frequencyValues(),
                                                        controller->specifications(),
                                                        this);
    }
}

void MainWindow::ensureFrequenciesDialog()
{
    if (frequenciesDialog == nullptr){
        frequenciesDialog = new FrequenciesDialog(this);
        frequenciesDialog->applyFrequencyCountLimit(m_settings.limits.maxFrequencyCount);
    }
}

void MainWindow::ensureTemplatesWidgets()
{
    if (templatesDialog == nullptr){
        templatesDialog = new TemplatesDialog(this);
        templatesDialog->setMaxPointCount(m_settings.limits.maxTemplatePoints);
        templatesDialog->setDefaultPointCount(m_settings.defaults.templatePointCount);
        templateViewer = new TemplateViewer(this);
        installContourRecomputer();
    }
}

void MainWindow::ensureBoundariesWidgets()
{
    if (boundaryGridDialog == nullptr){
        boundaryGridDialog = new BoundaryGridDialog(this);
        boundaryGridDialog->setMaxGridCells(m_settings.limits.maxGridCells);
        boundaryGridDialog->applyDefaults(m_settings.defaults);
        boundaryViewer = new BoundaryViewer(this);
        boundaryUnionViewer = new BoundaryUnionViewer(this);
    }
}

void MainWindow::ensureControllerDialog()
{
    if (controllerDialog == nullptr){
        controllerDialog = new ControllerDialog(this);
    }
}

void MainWindow::ensureLoopShapingWidgets()
{
    if (loopShapingDialog == nullptr){
        loopShapingDialog = new LoopShapingDialog(this);
        loopShapingDialog->setLimits(m_settings.limits.maxMagnitude,
                                     m_settings.limits.maxTemplatePoints);
        loopShapingDialog->applyDefaults(m_settings.defaults);
        loopShapingViewer = new LoopShapingViewer(this);
    }
}

void MainWindow::setDialogRunner(DialogRunner run)
{
    m_runDialog = std::move(run);
}

void MainWindow::setFileChooser(FileChooser choose)
{
    m_chooseFile = std::move(choose);
}

//Without a runner it is exec(), which is what the application does. The
//indirection exists so a test can be the user.
void MainWindow::runDialog(StepDialog * dialog)
{
    if (dialog == nullptr) {
        return;
    }

    //THE dialog is reused between visits, so a previous acceptance has to be
    //forgotten here. Without this, wasAccepted() answered true for ever after
    //the first OK - with the payload already handed over - and closing a
    //reopened dialog published a null one, wiping the step from the project.
    dialog->clearAcceptance();

    if (m_runDialog != nullptr) {
        m_runDialog(dialog);
        return;
    }

    dialog->exec();
}

QString MainWindow::chooseFile(bool forSaving, const QString & title)
{
    if (m_chooseFile != nullptr) {
        return m_chooseFile(forSaving);
    }

    return forSaving
            ? QFileDialog::getSaveFileName(this, title, "plant",
                                           tr("QFT Files (*.qft)"))
            : QFileDialog::getOpenFileName(this, title, "plant",
                                           tr("QFT Files (*.qft)"));
}

//Every enable rule, and the progress bar, DERIVED from what the project
//holds. This is the third of the four hand-written copies of the pipeline's
//dependency order to go: the facade owns the order, completed() answers it,
//and this asks rather than remembers.
//
//What it replaces: an enable condition inside each of the seven handlers, the
//same conditions again in the open handler, seven booleans and a counter kept
//by hand, and a stepBack() that walked the counter backwards.
//
//One rule is deliberately tighter than what it replaces. The Bode action used
//to be enabled by the frequencies alone, while the action itself refuses
//without a plant as well - so it could be pressed to no effect. It follows the
//action's own guard now, and nothing that worked stops working.
void MainWindow::refreshAvailability()
{
    if (controller == nullptr) {
        return;
    }

    const qftbx::StepSet done = controller->completed();

    //A step the project does not have has no widgets. Deciding that by hand,
    //once per way out of each handler, is what produced a teardown block
    //written some fifteen times - and it was decided WRONG in a case nobody
    //noticed: cancelling the template dialog after the templates had been
    //computed used to delete their viewer and mark the step undone, while the
    //project still held the templates. Derived, that case answers itself.
    //
    //The Bode viewer is not here because it is not a step: it is a view of
    //the plant and the frequencies, and it has its own action.
    if (!done.has(qftbx::Step::Plant)) {
        delete plantDialog;
        plantDialog = nullptr;
    }

    if (!done.has(qftbx::Step::Specifications)) {
        delete specificationsDialog;
        specificationsDialog = nullptr;
    }

    if (!done.has(qftbx::Step::Frequencies)) {
        delete frequenciesDialog;
        frequenciesDialog = nullptr;
    }

    if (!done.has(qftbx::Step::Controller)) {
        delete controllerDialog;
        controllerDialog = nullptr;
    }

    if (!done.has(qftbx::Step::Templates)) {
        delete templatesDialog;
        templatesDialog = nullptr;
        delete templateViewer;
        templateViewer = nullptr;
    }

    if (!done.has(qftbx::Step::Boundaries)) {
        delete boundaryGridDialog;
        boundaryGridDialog = nullptr;
        delete boundaryViewer;
        boundaryViewer = nullptr;
        delete boundaryUnionViewer;
        boundaryUnionViewer = nullptr;
    }

    if (!done.has(qftbx::Step::LoopShaping)) {
        delete loopShapingDialog;
        loopShapingDialog = nullptr;
        delete loopShapingViewer;
        loopShapingViewer = nullptr;
    }

    const bool plant          = done.has(qftbx::Step::Plant);
    const bool specifications = done.has(qftbx::Step::Specifications);
    const bool frequencies    = done.has(qftbx::Step::Frequencies);
    const bool templates      = done.has(qftbx::Step::Templates);
    const bool boundaries     = done.has(qftbx::Step::Boundaries);
    const bool structure      = done.has(qftbx::Step::Controller);

    ui->specificationsButton->setEnabled(frequencies);
    ui->templatesButton->setEnabled(plant && frequencies);
    ui->boundariesButton->setEnabled(templates && specifications);
    ui->loopButton->setEnabled(boundaries && structure);
    ui->bodeAction->setEnabled(plant && frequencies);

    ui->progressBar->setValue(static_cast<int>(done.count()));
}

void MainWindow::installContourRecomputer(){
    templateViewer->setContourRecomputer([this](std::vector<double> epsilon) {
        recomputeContour(std::move(epsilon));
    });
}

void MainWindow::recomputeContour(std::vector<double> epsilon){
    //The viewer asked for a tighter contour: the computation, and the
    //reporting of its failure, belong here.
    try {
        //It walks the epsilon-hull over every cloud, which is work, and this
        //was the one computation in the window with no cursor at all: it
        //froze under a pointer that said nothing was happening.
        const WaitCursor waiting(this);

        controller->recomputeContour(std::move(epsilon));
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Template computation"), e.what());
        return;
    }

    templateViewer->refreshContour(controller->contour(),
                                   controller->omega()->values(),
                                   controller->epsilon());
}

const std::vector<double> * MainWindow::frequencyValues() const{
    Omega * omega = controller->omega();

    if (omega == nullptr){
        return nullptr;
    }

    return omega->values();
}

void MainWindow::destroySession(){
    destroyDialogs();

    controller.reset();
}

void MainWindow::on_plantButton_clicked()
{
    ensurePlantDialog();

    runDialog(plantDialog);

    if (plantDialog->wasAccepted()){
        //The dialogs only describe; publishing into the project is the
        //window's job, so a dialog never needs to know the facade.
        std::unique_ptr<LtiSystem> described = plantDialog->takePlant();
        //Publish only what was actually received. The payload is MOVED out of
        //the dialog, so asking twice gives a null the second time - and
        //publishing a null wipes the step from the project, and everything
        //computed from it. That used to be reachable, because a reused dialog
        //reported an acceptance it had already handed over; StepDialog and
        //runDialog() closed that path. This stays as the invariant it is:
        //nothing moved-from goes into the project.
        if (described == nullptr){
            refreshAvailability();
            return;
        }

        //The project drops the templates and everything after them when the
        //plant changes; refreshAvailability() below follows it, so nothing
        //is decided here (it used to compare the address of a freshly built
        //object with the stored one, which never matched).
        controller->setPlant(std::move(described));
    } else {
        delete plantDialog;
        plantDialog = nullptr;
    }

    //Buttons and bar from the project, not from flags kept here.
    refreshAvailability();
}

void MainWindow::on_specificationsButton_clicked()
{

    //Both of these refuse a project with no design frequencies by throwing,
    //and an exception leaving a slot takes the application down. The button
    //is only enabled once the frequencies are in, so this is a broken
    //invariant rather than a user error - which is exactly the kind that
    //should be reported instead of aborting.
    try {
        ensureSpecificationsDialog();
        //The frequency set the dialog was built with may be gone: entering
        //new frequencies destroys the Omega that owns the values.
        specificationsDialog->setFrequencies(frequencyValues());
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Specifications input"), e.what());
        return;
    }

    runDialog(specificationsDialog);

    if (specificationsDialog->wasAccepted()){
        //See on_plantButton_clicked: nothing moved-from goes in, and an empty
        //answer here would wipe the specifications.
        std::optional<qftbx::SpecificationRecords> described =
                specificationsDialog->takeSpecifications();

        if (!described.has_value()){
            refreshAvailability();
            return;
        }

        controller->setSpecifications(std::move(described));

        //The templates do not depend on the specifications; the boundaries
        //do, and the project has already dropped them - the window follows
        //through refreshAvailability() below.
    } else {
        delete specificationsDialog;
        specificationsDialog = nullptr;
    }

    refreshAvailability();
}

void MainWindow::on_frequenciesButton_clicked()
{
    ensureFrequenciesDialog();

    runDialog(frequenciesDialog);

    if (frequenciesDialog->wasAccepted()){
        std::unique_ptr<Omega> described = frequenciesDialog->takeOmega();
        //See on_plantButton_clicked: nothing moved-from goes in.
        if (described == nullptr){
            refreshAvailability();
            return;
        }

        //See on_plantButton_clicked: the project decides what a new set of
        //frequencies drops, and the window follows below.
        controller->setOmega(std::move(described));
    } else {
        delete frequenciesDialog;
        frequenciesDialog = nullptr;
    }

    refreshAvailability();
}

void MainWindow::on_templatesButton_clicked()
{
    ensureTemplatesWidgets();

    templatesDialog->launch(controller->plant(), controller->omega()->values()->size());

    runDialog(templatesDialog);

    if (!templatesDialog->wasAccepted()){
        refreshAvailability();
        return;
    }

    bool templatesOk = false;

    {
        //The hourglass for as long as this scope, and no longer. It used to
        //be put back by hand on each of the three ways out of here.
        const WaitCursor waiting(this);

        try {
            templatesOk = controller->computeTemplates(templatesDialog->takeEpsilon(),
                                                       templatesDialog->grids(),
                                                       templatesDialog->cudaSelected());
        } catch (mup::ParserError & parserError) {
            //A muParserX error is neither a qftbx::Exception nor a
            //std::exception, so it escaped this slot and took the application
            //down. Parameter names are validated when a system is published,
            //which is where this class of problem belongs, but a computation
            //must not be able to kill the window either way.
            QMessageBox::critical(this, tr("Template computation"),
                                  tr("The expression parser refused this "
                                     "computation: %1")
                                      .arg(QString::fromStdString(parserError.GetMsg())));
            refreshAvailability();
            return;
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Template computation"), e.what());
            refreshAvailability();
            return;
        }
    }

    if (!templatesOk){
        refreshAvailability();
        return;
    }

    templateViewer->setData(controller->templates(),
                             controller->contour(),
                             controller->omega()->values(),
                             controller->epsilon());
    templateViewer->plotDiagram(templatesDialog->nicholsSelected());

    templateViewer->show();

    refreshAvailability();
}

void MainWindow::on_boundariesButton_clicked()
{
    ensureBoundariesWidgets();

    runDialog(boundaryGridDialog);

    if (!boundaryGridDialog->wasAccepted()){
        refreshAvailability();
        return;
    }

    bool boundariesOk = false;

    {
        const WaitCursor waiting(this);

        try {
            boundariesOk = controller->computeBoundaries(boundaryGridDialog->phaseRangeValue(),
                                                         boundaryGridDialog->phaseCountValue(),
                                                         boundaryGridDialog->magnitudeRangeValue(),
                                                         boundaryGridDialog->magnitudeCountValue(),
                                                         boundaryGridDialog->infinityValue(),
                                                         boundaryGridDialog->contourSelected(),
                                                         boundaryGridDialog->cudaSelected());
        } catch (mup::ParserError & parserError) {
            //A muParserX error is neither a qftbx::Exception nor a
            //std::exception, so it escaped this slot and took the application
            //down. Parameter names are validated when a system is published,
            //which is where this class of problem belongs, but a computation
            //must not be able to kill the window either way.
            QMessageBox::critical(this, tr("Boundary computation"),
                                  tr("The expression parser refused this "
                                     "computation: %1")
                                      .arg(QString::fromStdString(parserError.GetMsg())));
            refreshAvailability();
            return;
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Boundary computation"), e.what());
            refreshAvailability();
            return;
        }
    }

    if (!boundariesOk){
        refreshAvailability();
        return;
    }

    boundaryViewer->setData(controller->boundaries(), controller->omega()->values());
    boundaryViewer->showDiagram();
    boundaryViewer->show();

    boundaryUnionViewer->setData(controller->unionBoundaries(), controller->omega()->values());
    boundaryUnionViewer->showDiagram();
    boundaryUnionViewer->show();

    refreshAvailability();
}


void MainWindow::on_controllerButton_clicked()
{

    ensureControllerDialog();

    runDialog(controllerDialog);


    if (controllerDialog->wasAccepted()){
        std::unique_ptr<LtiSystem> described = controllerDialog->takeControllerStructure();
        //See on_plantButton_clicked: nothing moved-from goes in.
        if (described == nullptr){
            refreshAvailability();
            return;
        }

        //A different structure voids the design found for the old one; the
        //project drops it and the window follows below.
        controller->setControllerStructure(std::move(described));
    }

    refreshAvailability();
}

void MainWindow::on_loopButton_clicked()
{
    ensureLoopShapingWidgets();

    runDialog(loopShapingDialog);

    if (!loopShapingDialog->wasAccepted()){
        refreshAvailability();
        return;
    }

    bool designed = false;

    {
        //The search is the long one - tens of minutes on a real problem - and
        //it still runs on this thread, so the window is frozen for as long as
        //it takes. The facade can now run it on a worker and be asked to give
        //up; wiring that here needs a cancel button, and where that goes is a
        //decision still open.
        const WaitCursor waiting(this);

        try {
            designed = controller->computeLoopShaping(loopShapingDialog->epsilonValue(),
                                                      loopShapingDialog->algorithmValue(),
                                                      loopShapingDialog->range(),
                                                      loopShapingDialog->pointCountValue(),
                                                      loopShapingDialog->initialisationValue());
        } catch (mup::ParserError & parserError) {
            //Same treatment as a qftbx::Exception, and the reason it is
            //needed: a muParserX error is neither that nor a std::exception,
            //so it escaped this slot and took the application down. Naming a
            //controller gain "k" was enough - the parser reserves it as its
            //kilo postfix operator. Names are validated when a system is
            //published now; this is the net under it.
            QMessageBox::critical(this, tr("Loop Shaping"),
                                  tr("The expression parser refused this "
                                     "computation: %1")
                                      .arg(QString::fromStdString(parserError.GetMsg())));
            refreshAvailability();
            return;
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Loop Shaping"), e.what());
            refreshAvailability();
            return;
        }
    }

    if (!designed){
        refreshAvailability();
        return;
    }

    loopShapingViewer->setData(controller->unionBoundaries(), controller->omega()->values(),
                               controller->loopShapingResult(), controller->plant(),
                               loopShapingDialog->isLinSpace());

    loopShapingViewer->showDiagram();
    loopShapingViewer->show();

    refreshAvailability();
}

void MainWindow::on_actionSave_triggered()
{
    if(saveFilePath.isEmpty()){
        on_actionSaveAs_triggered();
    } else {
        saveProject();
    }
}

void MainWindow::on_actionSaveAs_triggered()
{
    const QString fileName = chooseFile(true, tr("Save file"));


    if (!fileName.isEmpty()){

        if (fileName.right(4) != ".qft"){
            saveFilePath = fileName+".qft";
        } else {
            saveFilePath = fileName;
        }
        saveProject();
    }
}

void MainWindow::saveProject(){
    try {
        controller->save(saveFilePath.toStdString());
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Save file"), e.what());
    }
}

void MainWindow::on_actionOpen_triggered()
{
    const QString fileName = chooseFile(false, tr("Open project"));

    if (!fileName.isEmpty()){

        qftbx::StepSet loaded;

        try {
            loaded = controller->load(fileName.toStdString());
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Open project"), e.what());
            return;
        }

        //The previous session's dialogs are freed: every open used to leak
        //the existing ones and the bar kept accumulating steps across files.
        destroyDialogs();

        //Save writes back to the file that was just opened.
        saveFilePath = fileName;

        //The widgets of the steps the file carried: the same ensure*() the
        //handlers use, so a step's widgets are built in one place. Which
        //steps are done is derived from the project, and the buttons and
        //the bar come from the one call at the end.
        if (loaded.has(qftbx::Step::Plant)) {
            ensurePlantDialog();
        }
        if (loaded.has(qftbx::Step::Specifications)) {
            ensureSpecificationsDialog();
        }
        if (loaded.has(qftbx::Step::Frequencies)) {
            ensureFrequenciesDialog();
        }
        if (loaded.has(qftbx::Step::Templates)) {
            ensureTemplatesWidgets();
        }
        if (loaded.has(qftbx::Step::Boundaries)) {
            ensureBoundariesWidgets();
        }
        if (loaded.has(qftbx::Step::Controller)) {
            ensureControllerDialog();
        }
        if (loaded.has(qftbx::Step::LoopShaping)) {
            ensureLoopShapingWidgets();
        }

        refreshAvailability();
    }

}

void MainWindow::on_actionConsole_triggered()
{
    //Unreachable while the action is disabled in the .ui; kept so the
    //parked console has a caller the day its two paths exist. See
    //MuParserXConsole.
    QString missing;
    if (!MuParserXConsole::launch(&missing)) {
        qftbx::errorMessage(tr("The muParserX console could not be started: "
                               "%1 is not there.").arg(missing), tr("QFTbx"));
    }
}

void MainWindow::on_actionNew_triggered()
{
    destroySession();
    createSession();
}

//The menu entry was enabled and disabled with care but connected to
//nothing: the handler had been commented out since the initial upload and
//the action it was named after has since been renamed, so nothing wired it
//up. Reconnected here; the drawing itself needed fixing (see drawBode).
void MainWindow::on_bodeAction_triggered()
{
    if (controller->completed().has(qftbx::Step::Plant) &&
            controller->completed().has(qftbx::Step::Frequencies)){

        //The pointer answers it: bodeCreated was a second copy of
        //"bodeViewer != nullptr", and destroyDialogs() had to remember to
        //clear both.
        if (bodeViewer == nullptr){
            bodeViewer = new BodeViewer(this);
        }

        bodeViewer->drawBode(controller->plant(), controller->omega());
        bodeViewer->show();
    }else{
        errorMessage(tr("To show the Bode diagram, first enter a valid plant and a set of design frequencies"), tr("QFT"));
    }
}

void MainWindow::on_actionNicholsLoop_triggered()
{
    showLoopDiagrams(true, false);
}

void MainWindow::on_actionNyquistLoop_triggered()
{
    showLoopDiagrams(false, true);
}

void MainWindow::on_actionAllLoopDiagrams_triggered()
{
    showLoopDiagrams(true, true);
}

void MainWindow::showLoopDiagrams(bool nichols, bool nyquist){

    //Without boundaries and a controller structure there is no loop to
    //show (uninitialised DAOs used to be dereferenced).
    const qftbx::StepSet done = controller->completed();

    if (!done.has(qftbx::Step::Boundaries) || !done.has(qftbx::Step::Controller)){
        errorMessage(tr("To show the loop diagram, first compute the boundaries and enter the controller structure."), tr("QFT"));
        return;
    }

    BoundaryData * boundaries = controller->boundaries();

    //The same union read on the complex plane, for the Nyquist half of the
    //view. This used to fabricate a whole BoundaryData: six heap containers
    //at first, then an empty bucket row per frequency and two converted
    //ranges, all to satisfy a constructor - and its points were Nichols
    //points holding real and imaginary parts. The viewer takes the curves it
    //draws, and the conversion is qftbx::toNyquist.
    qftbx::NyquistTraces nyquistTraces;
    nyquistTraces.reserve(boundaries->unionBoundaries().size());

    for (const qftbx::Trace & trace : boundaries->unionBoundaries()) {

        qftbx::NyquistTrace converted;
        converted.reserve(trace.size());

        for (const qftbx::NicholsPoint & point : trace) {
            converted.push_back(qftbx::toNyquist(point));
        }

        nyquistTraces.push_back(std::move(converted));
    }


    //Modal and parentless, so it is this scope's: on the stack. The Nyquist
    //boundaries and their buckets are held by value and die here too -
    //twenty lines of nested deletion used to stand at the end of this
    //function, and BoundaryData had to be told it did not own them.
    LoopBoundariesViewer ver;

    ver.setData(boundaries, nyquistTraces, controller->omega()->values(), controller->plant(),
                 controller->controllerStructure(), nichols, nyquist);

    ver.showDiagram();

    ver.exec();
}

void MainWindow::on_actionTemplates_triggered()
{
    //View-again action: with no computed templates there is nothing to
    //show (it used to mark the step done without data).
    if (!controller->completed().has(qftbx::Step::Templates)){
        return;
    }

    if (!controller->templates().empty() && !controller->contour().empty()){
        templateViewer->setData(controller->templates(),
                                 controller->contour(),
                                 controller->omega()->values(),
                                 controller->epsilon());
        templateViewer->plotDiagram(true);

        templateViewer->show();
    }
}

void MainWindow::on_actionBoundaries_triggered()
{
    if (!controller->completed().has(qftbx::Step::Boundaries)){
        return;
    }

    boundaryUnionViewer->setData(controller->unionBoundaries(), controller->omega()->values());
    boundaryUnionViewer->showDiagram();
    boundaryUnionViewer->show();
}

void MainWindow::on_actionLoop_triggered()
{
    if (!controller->completed().has(qftbx::Step::LoopShaping)){
        return;
    }

    loopShapingViewer->setData(controller->unionBoundaries(),controller->omega()->values(),
                              controller->loopShapingResult(), controller->plant(), loopShapingDialog->isLinSpace());

    loopShapingViewer->showDiagram();
    loopShapingViewer->show();
}

} // namespace qftbx
