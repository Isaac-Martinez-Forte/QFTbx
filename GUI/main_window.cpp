#include "GUI/loop_boundaries_viewer.h"
#include "main_window.h"
#include "GUI/error_message.h"
#include "GUI/plot_palette.h"
#include "ui_main_window.h"

#include <QMessageBox>

#include "Modelo/Herramientas/exception.h"

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

    controller = new Controlador();

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

void MainWindow::destroySession(){
    destroyDialogs();

    delete controller;
}

void MainWindow::on_plantButton_clicked()
{
    if (!plantDone){
        plantDialog = new PlantDialog(controller, this);
    }

    plantDialog->exec();

    if (plantDialog->getTodoCorrecto()){
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
        specificationsDialog = new SpecificationsDialog(controller, this);

    }

    specificationsDialog->exec();

    if (specificationsDialog->getTodoCorrecto()){

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
        frequenciesDialog = new FrequenciesDialog(controller,this);
    }

    frequenciesDialog->exec();

    if (frequenciesDialog->getTodoCorrecto()){

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
    }

    templatesDialog->launch(controller->getPlanta(), controller->getOmega()->getValores()->size());

    templatesDialog->exec();

    if (templatesDialog->getTodoCorrecto()){

        this->setCursor(Qt::WaitCursor);

        bool templatesOk = false;

        try {
            templatesOk = controller->calcularTemplates(templatesDialog->getEpsilon(), templatesDialog->grids(),
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

            templateViewer->setDatos(controller);
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
            boundariesOk = controller->calcularBoundaries(boundaryGridDialog->phaseRangeValue(),
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

        boundaryViewer->setDatos(controller->getBound(), controller->getOmega()->getValores());
        boundaryViewer->showDiagram();
        boundaryViewer->show();

        boundaryUnionViewer->setDatos(controller->unionBoundaries(), controller->getOmega()->getValores());
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
        controllerDialog = new ControllerDialog(controller, this);
    }

    controllerDialog->exec();


    if (controllerDialog->getTodoCorrecto()){
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
            re = controller->calcularLoopShaping(loopShapingDialog->epsilonValue(), loopShapingDialog->algorithmValue(), loopShapingDialog->range(),
                                                  loopShapingDialog->pointCountValue(), loopShapingDialog->debugValue(),
                                                  loopShapingDialog->deltaValue(), loopShapingDialog->initialisationValue(),
                                                  loopShapingDialog->threadsValue(), loopShapingDialog->bisectionValue(),
                                                  loopShapingDialog->detectionValue(), loopShapingDialog->acceleratedValue());
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
            loopShapingViewer->setDatos(controller->unionBoundaries(),controller->getOmega()->getValores(),
                                      controller->getLoopShaping(), controller->getPlanta(), loopShapingDialog->isLinSpace());

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
        controller->guardarSistema(saveFilePath);
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Save file"), e.what());
    }
}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open project"),"plant",
                                                    tr("QFT Files (*.qft)"));

    if (!fileName.isEmpty()){

        QVector <bool> * leido;

        try {
            leido = controller->cargarSistema(fileName);
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

        plantDone = leido->value(0);
        specificationsDone = leido->value(1);
        frequenciesDone = leido->value(2);
        templatesDone = leido->value(3);
        boundariesDone = leido->value(4);
        controllerDone = leido->value(5);
        loopDone = leido->value(6);

        delete leido;


        if (plantDone){
            plantDialog = new PlantDialog(controller, this);
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
        }

        if (specificationsDone){
            specificationsDialog = new SpecificationsDialog(controller, this);
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
        }

        if (frequenciesDone){
            frequenciesDialog = new FrequenciesDialog(controller,this);
            progressPosition++;
            ui->progressBar->setValue(progressPosition);
            ui->specificationsButton->setEnabled(true);
            ui->bodeAction->setEnabled(true);
        }

        if (templatesDone){
            templatesDialog = new TemplatesDialog(this);
            templateViewer = new TemplateViewer(this);
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
            controllerDialog = new ControllerDialog(controller, this);
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

/*void MainWindow::on_actionBodeDiagram_triggered()
{
    if (plantDone && frequenciesDone){

        if(!bodeCreated)
            bodeViewer = new BodeViewer(this);

        bodeCreated = true;

        bodeViewer->dibujarBode(controller->getPlanta(),controller->getOmega());
        bodeViewer->show();
    }else{
        errorMessage(tr("To show the Bode diagram, first enter a valid plant and a set of design frequencies"), tr("QFT"));
    }
}*/

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

    BoundaryData * boundaries = controller->getBound();

    QVector< QVector<QPointF> * > * boun = boundaries->unionBoundaries();

    QVector< QVector<QPointF> * > * nuevosBoundariesReun =
            new QVector< QVector<QPointF> * > ();


    QVector< QVector< QVector<QPointF> * > * > * nuevoHash_inter = new QVector< QVector< QVector<QPointF> * > * > ();

    foreach (auto vector, *boun) {

        QVector<QPointF> * nuevoVector = new QVector<QPointF>  ();

        QVector <QVector <QPointF> * > * nuevoHash = new QVector <QVector <QPointF> *> ();

        foreach (auto p, *vector) {
            maglineal = pow(10,p.y()/20);

            QPointF range_point (maglineal * cos (p.x() * M_PI / 180),
                           maglineal * sin (p.x() * M_PI / 180));

            nuevoVector->append(range_point);


        }

        nuevoHash_inter->append(nuevoHash);

        nuevosBoundariesReun->append(nuevoVector);
    }




    QPointF nuevoDatosFas ((boundaries->phaseRange().x() * M_PI) / 180, 0);

    QPointF datosMag = boundaries->magnitudeRange();

    QPointF nuevosDatosMag (pow(10,datosMag.x()/20), pow(10,datosMag.y()/20));

    BoundaryData * nuevoBoundaries = new BoundaryData (boundaries->boundaries(), boundaries->openFlags(),
                                                   boundaries->upperFlags(), boundaries->phaseCount(),
                                                   nuevoDatosFas, nuevosBoundariesReun,
                                                   nuevoHash_inter,
                                                   boundaries->magnitudeCount(), nuevosDatosMag);


    LoopBoundariesViewer * ver = new LoopBoundariesViewer();

    ver->setDatos(boundaries, nuevoBoundaries, controller->getOmega()->getValores(), controller->getPlanta(),
                  controller->getControlador(), nichols, nyquistRadio);

    ver->showDiagram();

    ver->exec();

    //BoundaryData is a non-owning view: the temporary containers built
    //here are freed separately (they used to be abandoned).
    delete nuevoBoundaries;

    foreach (QVector<QPointF> * vector, *nuevosBoundariesReun) {
        delete vector;
    }
    delete nuevosBoundariesReun;

    foreach (auto * porFrecuencia, *nuevoHash_inter) {
        foreach (QVector<QPointF> * cubeta, *porFrecuencia) {
            delete cubeta;
        }
        delete porFrecuencia;
    }
    delete nuevoHash_inter;

    delete ver;
}

void MainWindow::on_actionTemplates_triggered()
{
    //View-again action: with no computed templates there is nothing to
    //show (it used to mark the step done without data).
    if (!templatesDone){
        return;
    }

    if (controller->getTemplate() != nullptr && controller->getContorno() != nullptr){
        templateViewer->setDatos(controller);
        templateViewer->plotDiagram(true);

        templateViewer->show();
    }
}

void MainWindow::on_actionBoundaries_triggered()
{
    if (!boundariesDone){
        return;
    }

    boundaryUnionViewer->setDatos(controller->unionBoundaries(), controller->getOmega()->getValores());
    boundaryUnionViewer->showDiagram();
    boundaryUnionViewer->show();
}

void MainWindow::on_actionLoop_triggered()
{
    if (!loopDone){
        return;
    }

    loopShapingViewer->setDatos(controller->unionBoundaries(),controller->getOmega()->getValores(),
                              controller->getLoopShaping(), controller->getPlanta(), loopShapingDialog->isLinSpace());

    loopShapingViewer->showDiagram();
    loopShapingViewer->show();
}
