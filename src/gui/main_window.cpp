#include "src/gui/loop_boundaries_viewer.h"
#include "main_window.h"
#include "src/gui/error_message.h"
#include "src/gui/plot_palette.h"
#include "ui_main_window.h"

#include <QMessageBox>

#include "src/core/point.h"
#include "src/core/exception.h"
#include "src/core/pipeline_step.h"

#include <mpParser.h>

#include <vector>
#include <iostream>

using namespace tools;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(std::make_unique<Ui::MainWindow>())
{

    //QQmlApplicationEngine engine;
    //engine.load(QUrl(QStringLiteral("windowsgeneral.qml")));
    
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

    ui->progressBar->setValue(0);
    progressPosition = 0;

    plantDone = false;  //plant
    specificationsDone = false;  //specificationsDialog
    frequenciesDone = false;  //omega
    templatesDone = false;  //templates
    boundariesDone = false;  //estructura del controller
    controllerDone = false;  //boundaries
    loopDone = false;  //lazo

    bodeCreated = false;

    ui->specificationsButton->setEnabled(false);
    ui->templatesButton->setEnabled(false);
    ui->boundariesButton->setEnabled(false);
    ui->loopButton->setEnabled(false);
    ui->bodeAction->setEnabled(false);

    //Without this, Save after New overwrote the last opened file with the
    //empty project.
    saveFilePath.clear();
}

//Walks one step back: the bar must reflect it (it used to keep counting
//steps that no longer existed).
void MainWindow::stepBack(bool & paso){
    if (paso){
        progressPosition--;
        ui->progressBar->setValue(progressPosition);
    }
    paso = false;
}

//Qt's own mechanism: every dialog and viewer here is a child of this
//window, and destroying one is how a new session gets a fresh one.
//
//Each delete used to be guarded by the step's own progress flag. The two
//agree - every path that abandons a dialog deletes it, nulls it and resets
//the flag through stepBack(), which takes it by reference - but a
//destruction path has no business depending on a flag of the interface:
//deleting a null pointer is a no-op, so the pointer decides.
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
    bodeCreated = false;

    delete loopShapingDialog;
    loopShapingDialog = nullptr;
    delete loopShapingViewer;
    loopShapingViewer = nullptr;
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
void MainWindow::runDialog(QDialog * dialog)
{
    if (dialog == nullptr) {
        return;
    }

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

void MainWindow::installContourRecomputer(){
    templateViewer->setContourRecomputer([this](std::vector<double> epsilon) {
        recomputeContour(std::move(epsilon));
    });
}

void MainWindow::recomputeContour(std::vector<double> epsilon){
    //The viewer asked for a tighter contour: the computation, and the
    //reporting of its failure, belong here.
    try {
        controller->recomputeContour(std::move(epsilon));
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Template computation"), e.what());
        return;
    }

    templateViewer->refreshContour(controller->contour(),
                                   controller->omega()->values(),
                                   controller->epsilon());
}

//The project drops whatever was computed from an input that changed, so the
//window has to stop offering the steps that produced it: otherwise the button
//is still there and the step now refuses, which reads as a broken program
//rather than as a step that has to be redone.
void MainWindow::invalidateFromTemplates(){
    stepBack(templatesDone);
    invalidateFromBoundaries();

    ui->boundariesButton->setEnabled(false);

    delete templatesDialog;
    templatesDialog = nullptr;
    delete templateViewer;
    templateViewer = nullptr;
}

void MainWindow::invalidateFromBoundaries(){
    stepBack(boundariesDone);
    invalidateLoopShaping();

    ui->loopButton->setEnabled(false);

    delete boundaryGridDialog;
    boundaryGridDialog = nullptr;
    delete boundaryViewer;
    boundaryViewer = nullptr;
    delete boundaryUnionViewer;
    boundaryUnionViewer = nullptr;
}

void MainWindow::invalidateLoopShaping(){
    stepBack(loopDone);

    delete loopShapingDialog;
    loopShapingDialog = nullptr;
    delete loopShapingViewer;
    loopShapingViewer = nullptr;
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
    if (!plantDone){
        plantDialog = new PlantDialog(this);
    }

    runDialog(plantDialog);

    if (plantDialog->wasAccepted()){
        //The dialogs only describe; publishing into the project is the
        //window's job, so a dialog never needs to know the facade.
        std::unique_ptr<LtiSystem> described = plantDialog->takePlant();

        //The controller answers whether it dropped anything, so the window
        //resets its own steps exactly when the project dropped the artefacts
        //behind them. This used to be decided here, by comparing the address
        //of a freshly built object with the stored one - a test that can
        //never match, so it always invalidated. It agreed with the project
        //only by accident.
        const bool changed = controller->setPlant(std::move(described));

        //A different plant voids the templates and everything after them, in
        //the project and therefore in the window too.
        if (changed && templatesDone){
            invalidateFromTemplates();
        }

        if (frequenciesDone){
            ui->templatesButton->setEnabled(true);
        }

        if (!plantDone){
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
        }

        plantDone = true;
    } else {
        delete plantDialog;
        plantDialog = nullptr;
        stepBack(plantDone);
    }
}

void MainWindow::on_specificationsButton_clicked()
{

    //Both of these refuse a project with no design frequencies by throwing,
    //and an exception leaving a slot takes the application down. The button
    //is only enabled once the frequencies are in, so this is a broken
    //invariant rather than a user error - which is exactly the kind that
    //should be reported instead of aborting.
    try {
        if (!specificationsDone){
            specificationsDialog = new SpecificationsDialog(frequencyValues(),
                                                            controller->specifications(),
                                                            this);
        } else {
            //The frequency set the dialog was built with may be gone:
            //entering new frequencies destroys the Omega that owns the values.
            specificationsDialog->setFrequencies(frequencyValues());
        }
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Specifications input"), e.what());
        return;
    }

    runDialog(specificationsDialog);

    if (specificationsDialog->wasAccepted()){
        controller->setSpecifications(specificationsDialog->takeSpecifications());

        //The templates do not depend on the specifications; the boundaries
        //do. Publishing is always a change now: the dialog answers with a
        //fresh set of clones, so there is no identity to compare.
        if (boundariesDone){
            invalidateFromBoundaries();
        }

        if (templatesDone){
            ui->boundariesButton->setEnabled(true);
        }

        if (!specificationsDone){
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
        }
        specificationsDone = true;
    } else {
        delete specificationsDialog;
        specificationsDialog = nullptr;
        stepBack(specificationsDone);
    }
}

void MainWindow::on_frequenciesButton_clicked()
{
    if (!frequenciesDone){
        frequenciesDialog = new FrequenciesDialog(this);
    }

    runDialog(frequenciesDialog);

    if (frequenciesDialog->wasAccepted()){
        std::unique_ptr<Omega> described = frequenciesDialog->takeOmega();

        //The controller answers whether it dropped anything, so the window
        //resets its own steps exactly when the project dropped the artefacts
        //behind them. This used to be decided here, by comparing the address
        //of a freshly built object with the stored one - a test that can
        //never match, so it always invalidated. It agreed with the project
        //only by accident.
        const bool changed = controller->setOmega(std::move(described));

        if (changed && templatesDone){
            invalidateFromTemplates();
        }

        if (plantDone){
            ui->templatesButton->setEnabled(true);
        }

        ui->specificationsButton->setEnabled(true);
        ui->bodeAction->setEnabled(true);

        if (!frequenciesDone){
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
        }

        frequenciesDone = true;

    } else {
        delete frequenciesDialog;
        frequenciesDialog = nullptr;
        stepBack(frequenciesDone);
    }
}

void MainWindow::on_templatesButton_clicked()
{

    if (!templatesDone){
        templatesDialog = new TemplatesDialog(this);
        templateViewer = new TemplateViewer(this);
        installContourRecomputer();
    }

    templatesDialog->launch(controller->plant(), controller->omega()->values()->size());

    runDialog(templatesDialog);

    if (templatesDialog->wasAccepted()){

        this->setCursor(Qt::WaitCursor);

        bool templatesOk = false;

        try {
            templatesOk = controller->computeTemplates(templatesDialog->takeEpsilon(), templatesDialog->grids(),
                                                         templatesDialog->cudaSelected());
        } catch (mup::ParserError & parserError) {
            //A muParserX error is neither a qftbx::Exception nor a
            //std::exception, so it escaped this slot and took the application
            //down. Parameter names are validated when a system is published,
            //which is where this class of problem belongs, but a computation
            //must not be able to kill the window either way.
            this->setCursor(Qt::ArrowCursor);
            QMessageBox::critical(this, tr("Template computation"),
                                  tr("The expression parser refused this "
                                     "computation: %1")
                                      .arg(QString::fromStdString(parserError.GetMsg())));
            return;
        } catch (const qftbx::Exception & e) {
            this->setCursor(Qt::ArrowCursor);
            QMessageBox::critical(this, tr("Template computation"), e.what());
            delete templatesDialog;
            templatesDialog = nullptr;
            delete templateViewer;
            templateViewer = nullptr;
            stepBack(templatesDone);
            return;
        }

        if (templatesOk){

            this->setCursor(Qt::ArrowCursor);


            if (specificationsDone){
                ui->boundariesButton->setEnabled(true);
            }

            templateViewer->setData(controller->templates(),
                                     controller->contour(),
                                     controller->omega()->values(),
                                     controller->epsilon());
            templateViewer->plotDiagram(templatesDialog->nicholsSelected());

            templateViewer->show();

            if (!templatesDone){
                progressPosition++;
                ui->progressBar->setValue(progressPosition);
            }

            templatesDone = true;
        } else {
            delete templatesDialog;
            templatesDialog = nullptr;
            delete templateViewer;
            templateViewer = nullptr;
            stepBack(templatesDone);
        }
        this->setCursor(Qt::ArrowCursor);
    } else {
        delete templatesDialog;
        templatesDialog = nullptr;
        delete templateViewer;
        templateViewer = nullptr;
        stepBack(templatesDone);
    }
}

void MainWindow::on_boundariesButton_clicked()
{

    if (!boundariesDone){
        boundaryGridDialog = new BoundaryGridDialog(this);
        boundaryViewer = new BoundaryViewer(this);
        boundaryUnionViewer = new BoundaryUnionViewer(this);
    }

    runDialog(boundaryGridDialog);

    if (boundaryGridDialog->wasAccepted()){

        this->setCursor(Qt::WaitCursor);

        bool boundariesOk = false;

        try {
            boundariesOk = controller->computeBoundaries(boundaryGridDialog->phaseRangeValue(),
                                                           boundaryGridDialog->phaseCountValue(), boundaryGridDialog->magnitudeRangeValue(),
                                                           boundaryGridDialog->magnitudeCountValue(), boundaryGridDialog->infinityValue(),
                                                           boundaryGridDialog->contourSelected(), boundaryGridDialog->cudaSelected());
        } catch (mup::ParserError & parserError) {
            //A muParserX error is neither a qftbx::Exception nor a
            //std::exception, so it escaped this slot and took the application
            //down. Parameter names are validated when a system is published,
            //which is where this class of problem belongs, but a computation
            //must not be able to kill the window either way.
            this->setCursor(Qt::ArrowCursor);
            QMessageBox::critical(this, tr("Boundary computation"),
                                  tr("The expression parser refused this "
                                     "computation: %1")
                                      .arg(QString::fromStdString(parserError.GetMsg())));
            return;
        } catch (const qftbx::Exception & e) {
            this->setCursor(Qt::ArrowCursor);
            QMessageBox::critical(this, tr("Boundary computation"), e.what());
            delete boundaryGridDialog;
            boundaryGridDialog = nullptr;
            delete boundaryViewer;
            boundaryViewer = nullptr;
            delete boundaryUnionViewer;
            boundaryUnionViewer = nullptr;
            stepBack(boundariesDone);
            return;
        }

        if (!boundariesOk){
            this->setCursor(Qt::ArrowCursor);

            delete boundaryGridDialog;
            boundaryGridDialog = nullptr;
            delete boundaryViewer;
            boundaryViewer = nullptr;
            delete boundaryUnionViewer;
            boundaryUnionViewer = nullptr;
            stepBack(boundariesDone);

            return;
        }

        this->setCursor(Qt::ArrowCursor);

        boundaryViewer->setData(controller->boundaries(), controller->omega()->values());
        boundaryViewer->showDiagram();
        boundaryViewer->show();

        boundaryUnionViewer->setData(controller->unionBoundaries(), controller->omega()->values());
        boundaryUnionViewer->showDiagram();
        boundaryUnionViewer->show();

        if (!boundariesDone){
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
        }

        boundariesDone = true;

        if (controllerDone && boundariesDone){
            ui->loopButton->setEnabled(true);
        }

    }
}


void MainWindow::on_controllerButton_clicked()
{

    if (!controllerDone){
        controllerDialog = new ControllerDialog(this);
    }

    runDialog(controllerDialog);


    if (controllerDialog->wasAccepted()){
        std::unique_ptr<LtiSystem> described = controllerDialog->takeControllerStructure();

        //The controller answers whether it dropped anything, so the window
        //resets its own steps exactly when the project dropped the artefacts
        //behind them. This used to be decided here, by comparing the address
        //of a freshly built object with the stored one - a test that can
        //never match, so it always invalidated. It agreed with the project
        //only by accident.
        const bool changed = controller->setControllerStructure(std::move(described));

        if (changed && loopDone){
            invalidateLoopShaping();
        }

        if (boundariesDone){
            ui->loopButton->setEnabled(true);
        }

        if (!controllerDone){
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
        }
        controllerDone = true;
        //ui->menuLoopDiagram->setEnabled(true);
    } else {
        delete controllerDialog;
        controllerDialog = nullptr;
        stepBack(controllerDone);
    }
}

void MainWindow::on_loopButton_clicked()
{
    if (!loopDone){
        loopShapingDialog = new LoopShapingDialog(this);
        loopShapingViewer = new LoopShapingViewer(this);
    }


    runDialog(loopShapingDialog);

    if (loopShapingDialog->wasAccepted()){
        bool re = false;

        try {
            re = controller->computeLoopShaping(loopShapingDialog->epsilonValue(), loopShapingDialog->algorithmValue(), loopShapingDialog->range(),
                                                  loopShapingDialog->pointCountValue(), loopShapingDialog->initialisationValue());
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
            delete loopShapingDialog;
            loopShapingDialog = nullptr;
            delete loopShapingViewer;
            loopShapingViewer = nullptr;
            stepBack(loopDone);
            return;
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Loop Shaping"), e.what());
            delete loopShapingDialog;
            loopShapingDialog = nullptr;
            delete loopShapingViewer;
            loopShapingViewer = nullptr;
            stepBack(loopDone);
            return;
        }

        if (re){
            loopShapingViewer->setData(controller->unionBoundaries(),controller->omega()->values(),
                                      controller->loopShapingResult(), controller->plant(), loopShapingDialog->isLinSpace());

            loopShapingViewer->showDiagram();
            loopShapingViewer->show();

            if (!loopDone){
                progressPosition++;
                ui->progressBar->setValue(progressPosition);
            }
            loopDone = true;
        } else {
            delete loopShapingDialog;
            loopShapingDialog = nullptr;
            delete loopShapingViewer;
            loopShapingViewer = nullptr;
            stepBack(loopDone);
        }
    } else {
        delete loopShapingDialog;
        loopShapingDialog = nullptr;
        delete loopShapingViewer;
        loopShapingViewer = nullptr;
        stepBack(loopDone);
    }
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

        qftbx::StepSet leido;

        try {
            leido = controller->load(fileName.toStdString());
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Open project"), e.what());
            return;
        }

        //The previous session's dialogs are freed and the bar restarts:
        //every open used to leak the existing dialogs and the bar kept
        //accumulating steps across files.
        destroyDialogs();
        progressPosition = 0;
        ui->progressBar->setValue(0);
        ui->specificationsButton->setEnabled(false);
        ui->templatesButton->setEnabled(false);
        ui->boundariesButton->setEnabled(false);
        ui->loopButton->setEnabled(false);
        ui->bodeAction->setEnabled(false);

        //Save writes back to the file that was just opened.
        saveFilePath = fileName;

        //By name, from a typed set. It was leido.at(0) through leido.at(6)
        //against an eight-element vector whose eighth entry was not a step.
        plantDone = leido.has(qftbx::Step::Plant);
        specificationsDone = leido.has(qftbx::Step::Specifications);
        frequenciesDone = leido.has(qftbx::Step::Frequencies);
        templatesDone = leido.has(qftbx::Step::Templates);
        boundariesDone = leido.has(qftbx::Step::Boundaries);
        controllerDone = leido.has(qftbx::Step::Controller);
        loopDone = leido.has(qftbx::Step::LoopShaping);


        if (plantDone){
            plantDialog = new PlantDialog(this);
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
        }

        if (specificationsDone){
            specificationsDialog = new SpecificationsDialog(frequencyValues(),
                                                            controller->specifications(),
                                                            this);
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
        }

        if (frequenciesDone){
            frequenciesDialog = new FrequenciesDialog(this);
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
            ui->specificationsButton->setEnabled(true);
            ui->bodeAction->setEnabled(true);
        }

        if (templatesDone){
            templatesDialog = new TemplatesDialog(this);
            templateViewer = new TemplateViewer(this);
            installContourRecomputer();
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
        }

        if (boundariesDone){
            boundaryGridDialog = new BoundaryGridDialog(this);
            boundaryViewer = new BoundaryViewer(this);
            boundaryUnionViewer = new BoundaryUnionViewer (this);
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
        }

        if (controllerDone){
            controllerDialog = new ControllerDialog(this);
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
            //ui->menuLoopDiagram->setEnabled(true);
        }

        if (loopDone){
            loopShapingDialog = new LoopShapingDialog(this);
            loopShapingViewer = new LoopShapingViewer(this);
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
        }

        if (plantDone && frequenciesDone){
            ui->templatesButton->setEnabled(true);
        }

        if (templatesDone && specificationsDone){
            ui->boundariesButton->setEnabled(true);

        }

        if (boundariesDone && controllerDone){
            ui->loopButton->setEnabled(true);
        }

        ui->progressBar->setValue(progressPosition);
    }

}

void MainWindow::on_actionConsole_triggered()
{
    //Unreachable while the action is disabled in the .ui; kept so the
    //parked console has a caller the day its two paths exist. See
    //MuParserXConsole.
    QString missing;
    if (!MuParserXConsole::launch(&missing)) {
        tools::errorMessage(tr("The muParserX console could not be started: "
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
    if (plantDone && frequenciesDone){

        if(!bodeCreated)
            bodeViewer = new BodeViewer(this);

        bodeCreated = true;

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

void MainWindow::showLoopDiagrams(bool nichols, bool nyquistRadio){

    //Without boundaries and a controller structure there is no loop to
    //show (uninitialised DAOs used to be dereferenced).
    if (!boundariesDone || !controllerDone){
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
                 controller->controllerStructure(), nichols, nyquistRadio);

    ver.showDiagram();

    ver.exec();
}

void MainWindow::on_actionTemplates_triggered()
{
    //View-again action: with no computed templates there is nothing to
    //show (it used to mark the step done without data).
    if (!templatesDone){
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
    if (!boundariesDone){
        return;
    }

    boundaryUnionViewer->setData(controller->unionBoundaries(), controller->omega()->values());
    boundaryUnionViewer->showDiagram();
    boundaryUnionViewer->show();
}

void MainWindow::on_actionLoop_triggered()
{
    if (!loopDone){
        return;
    }

    loopShapingViewer->setData(controller->unionBoundaries(),controller->omega()->values(),
                              controller->loopShapingResult(), controller->plant(), loopShapingDialog->isLinSpace());

    loopShapingViewer->showDiagram();
    loopShapingViewer->show();
}
