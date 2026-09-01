#include "GUI/loop_boundaries_viewer.h"
#include "main_window.h"
#include "GUI/error_message.h"
#include "GUI/plot_palette.h"
#include "ui_main_window.h"

#include <QMessageBox>

#include "src/core/exception.h"

#include <iostream>

using namespace tools;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
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

    delete ui;
}

void MainWindow::createSession(){

    controller = new ProjectController();

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
    consoleCreated = false;

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

void MainWindow::destroyDialogs(){
    if (plantDone){
        delete plantDialog;
        plantDialog = nullptr;
    }

    if (specificationsDone){
        delete specificationsDialog;
        specificationsDialog = nullptr;
    }

    if (frequenciesDone){
        delete frequenciesDialog;
        frequenciesDialog = nullptr;
    }

    if (templatesDone){
        delete templatesDialog;
        templatesDialog = nullptr;
        delete templateViewer;
        templateViewer = nullptr;
    }

    if (boundariesDone){
        delete boundaryGridDialog;
        boundaryGridDialog = nullptr;
        delete boundaryViewer;
        boundaryViewer = nullptr;
        delete boundaryUnionViewer;
        boundaryUnionViewer = nullptr;
    }

    if (controllerDone){
        delete controllerDialog;
        controllerDialog = nullptr;
    }

    if (bodeCreated){
        delete bodeViewer;
        bodeViewer = nullptr;
        bodeCreated = false;
    }

    if (loopDone){
        delete loopShapingDialog;
        loopShapingDialog = nullptr;
        delete loopShapingViewer;
        loopShapingViewer = nullptr;
    }
}

void MainWindow::installContourRecomputer(){
    templateViewer->setContourRecomputer([this](QVector<qreal> epsilon) {
        recomputeContour(std::move(epsilon));
    });
}

void MainWindow::recomputeContour(QVector<qreal> epsilon){
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

const QVector<qreal> * MainWindow::frequencyValues() const{
    Omega * omega = controller->omega();

    if (omega == nullptr){
        return nullptr;
    }

    return omega->values();
}

void MainWindow::destroySession(){
    destroyDialogs();

    delete controller;
}

void MainWindow::on_plantButton_clicked()
{
    if (!plantDone){
        plantDialog = new PlantDialog(this);
    }

    plantDialog->exec();

    if (plantDialog->getTodoCorrecto()){
        //The dialogs only describe; publishing into the project is the
        //window's job, so a dialog never needs to know the facade.
        std::unique_ptr<LtiSystem> described = plantDialog->takePlant();
        const bool changed = described.get() != controller->plant();

        controller->setPlant(std::move(described));

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

    if (!specificationsDone){
        specificationsDialog = new SpecificationsDialog(frequencyValues(),
                                                        controller->specifications(),
                                                        this);

    }

    specificationsDialog->exec();

    if (specificationsDialog->getTodoCorrecto()){
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

    frequenciesDialog->exec();

    if (frequenciesDialog->getTodoCorrecto()){
        std::unique_ptr<Omega> described = frequenciesDialog->takeOmega();
        const bool changed = described.get() != controller->omega();

        controller->setOmega(std::move(described));

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

    templatesDialog->exec();

    if (templatesDialog->getTodoCorrecto()){

        this->setCursor(Qt::WaitCursor);

        bool templatesOk = false;

        try {
            templatesOk = controller->computeTemplates(templatesDialog->takeEpsilon(), templatesDialog->grids(),
                                                         templatesDialog->cudaSelected());
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

            templateViewer->setDatos(controller->templates(),
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

    boundaryGridDialog->exec();

    if (boundaryGridDialog->getTodoCorrecto()){

        this->setCursor(Qt::WaitCursor);

        bool boundariesOk = false;

        try {
            boundariesOk = controller->computeBoundaries(boundaryGridDialog->phaseRangeValue(),
                                                           boundaryGridDialog->phaseCountValue(), boundaryGridDialog->magnitudeRangeValue(),
                                                           boundaryGridDialog->magnitudeCountValue(), boundaryGridDialog->infinityValue(),
                                                           boundaryGridDialog->contourSelected(), boundaryGridDialog->cudaSelected());
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

        boundaryViewer->setDatos(controller->boundaries(), controller->omega()->values());
        boundaryViewer->showDiagram();
        boundaryViewer->show();

        boundaryUnionViewer->setDatos(controller->unionBoundaries(), controller->omega()->values());
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

    controllerDialog->exec();


    if (controllerDialog->getTodoCorrecto()){
        std::unique_ptr<LtiSystem> described = controllerDialog->takeControllerStructure();
        const bool changed = described.get() != controller->controllerStructure();

        controller->setControllerStructure(std::move(described));

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


    loopShapingDialog->exec();

    if (loopShapingDialog->getTodoCorrecto()){
        bool re = false;

        try {
            re = controller->computeLoopShaping(loopShapingDialog->epsilonValue(), loopShapingDialog->algorithmValue(), loopShapingDialog->range(),
                                                  loopShapingDialog->pointCountValue(), loopShapingDialog->initialisationValue());
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
            loopShapingViewer->setDatos(controller->unionBoundaries(),controller->omega()->values(),
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
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save file"),"plant",
                                                    tr("QFT Files (*.qft)"));


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
        controller->save(saveFilePath);
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Save file"), e.what());
    }
}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open project"),"plant",
                                                    tr("QFT Files (*.qft)"));

    if (!fileName.isEmpty()){

        std::vector<bool> leido;

        try {
            leido = controller->load(fileName);
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

        plantDone = leido.at(0);
        specificationsDone = leido.at(1);
        frequenciesDone = leido.at(2);
        templatesDone = leido.at(3);
        boundariesDone = leido.at(4);
        controllerDone = leido.at(5);
        loopDone = leido.at(6);


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
    consola con;
    con.mostrar();
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

    qreal maglineal = 0;

    BoundaryData * boundaries = controller->boundaries();

    //Nichols (phase in degrees, magnitude in dB) to Nyquist (the complex
    //plane). Six heap containers used to be built here and handed to a
    //BoundaryData that then had to be told whether it owned them.
    qftbx::UnionTraces nuevosBoundariesReun;
    qftbx::UnionBuckets nuevoHash_inter;

    for (const qftbx::Trace & vector : boundaries->unionBoundaries()) {

        qftbx::Trace nuevoVector;

        for (const QPointF & p : vector) {
            maglineal = pow(10,p.y()/20);

            nuevoVector.push_back(QPointF(maglineal * cos (p.x() * M_PI / 180),
                                          maglineal * sin (p.x() * M_PI / 180)));
        }

        //One empty bucket row per frequency: this view is only drawn, never
        //classified, so the buckets are not needed.
        nuevoHash_inter.push_back({});

        nuevosBoundariesReun.push_back(std::move(nuevoVector));
    }




    QPointF nuevoDatosFas ((boundaries->phaseRange().x() * M_PI) / 180, 0);

    QPointF datosMag = boundaries->magnitudeRange();

    QPointF nuevosDatosMag (pow(10,datosMag.x()/20), pow(10,datosMag.y()/20));

    BoundaryData nuevoBoundaries (boundaries->boundaries(), boundaries->openFlags(),
                                  boundaries->upperFlags(), boundaries->phaseCount(),
                                  nuevoDatosFas, std::move(nuevosBoundariesReun),
                                  std::move(nuevoHash_inter),
                                  boundaries->magnitudeCount(), nuevosDatosMag);


    LoopBoundariesViewer * ver = new LoopBoundariesViewer();

    ver->setDatos(boundaries, &nuevoBoundaries, controller->omega()->values(), controller->plant(),
                  controller->controllerStructure(), nichols, nyquistRadio);

    ver->showDiagram();

    ver->exec();

    //Nothing to free: the Nyquist boundaries and their buckets are held by
    //value and die with this scope. Twenty lines of nested deletion used to
    //stand here, and BoundaryData had to be told it did not own them.
    delete ver;
}

void MainWindow::on_actionTemplates_triggered()
{
    //View-again action: with no computed templates there is nothing to
    //show (it used to mark the step done without data).
    if (!templatesDone){
        return;
    }

    if (!controller->templates().empty() && !controller->contour().empty()){
        templateViewer->setDatos(controller->templates(),
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

    boundaryUnionViewer->setDatos(controller->unionBoundaries(), controller->omega()->values());
    boundaryUnionViewer->showDiagram();
    boundaryUnionViewer->show();
}

void MainWindow::on_actionLoop_triggered()
{
    if (!loopDone){
        return;
    }

    loopShapingViewer->setDatos(controller->unionBoundaries(),controller->omega()->values(),
                              controller->loopShapingResult(), controller->plant(), loopShapingDialog->isLinSpace());

    loopShapingViewer->showDiagram();
    loopShapingViewer->show();
}
