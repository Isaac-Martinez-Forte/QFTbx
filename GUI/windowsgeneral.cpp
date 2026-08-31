#include "windowsgeneral.h"
#include "GUI/menerror.h"
#include "GUI/plot_palette.h"
#include "ui_windowsgeneral.h"

#include <QMessageBox>

#include "Modelo/Herramientas/exception.h"

#include <iostream>

using namespace tools;

WindowsGeneral::WindowsGeneral(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WindowsGeneral)
{

    //QQmlApplicationEngine engine;
    //engine.load(QUrl(QStringLiteral("windowsgeneral.qml")));
    
    ui->setupUi(this);
    setWindowTitle("QFT: Quantitative feedback theory");

    //7 pasos reales: con rango 0-8 la barra nunca llegaba al 100%.
    ui->barraprogreso->setRange(0,7);

    crear();
}

WindowsGeneral::~WindowsGeneral()
{
    destruir();

    delete ui;
}

void WindowsGeneral::crear(){

    controlador = new Controlador();

    ui->barraprogreso->setValue(0);
    posBarra = 0;

    paso1 = false;  //planta
    paso2 = false;  //especificaciones
    paso3 = false;  //omega
    paso4 = false;  //templates
    paso5 = false;  //estructura del controlador
    paso6 = false;  //boundaries
    paso7 = false;  //lazo

    digBode = false;
    digconsola = false;

    ui->BEspi->setEnabled(false);
    ui->BTemp->setEnabled(false);
    ui->BBoun->setEnabled(false);
    ui->BDiLaz->setEnabled(false);
    ui->barraDiagramaBode->setEnabled(false);

    //Sin esto, "Guardar" tras "Nuevo" sobreescribia el ultimo fichero
    //abierto con el proyecto vacio.
    ficheroGuardar.clear();
}

//Retrocede un paso: la barra debe reflejarlo (antes se quedaba contando
//pasos que ya no existian).
void WindowsGeneral::retrocederPaso(bool & paso){
    if (paso){
        posBarra--;
        ui->barraprogreso->setValue(posBarra);
    }
    paso = false;
}

void WindowsGeneral::destruirDialogos(){
    if (paso1){
        delete intPlanta;
        intPlanta = nullptr;
    }

    if (paso2){
        delete especificaciones;
        especificaciones = nullptr;
    }

    if (paso3){
        delete intOmega;
        intOmega = nullptr;
    }

    if (paso4){
        delete vTemplates;
        vTemplates = nullptr;
        delete graficoTemplate;
        graficoTemplate = nullptr;
    }

    if (paso5){
        delete datosBoun;
        datosBoun = nullptr;
        delete viewBound;
        viewBound = nullptr;
        delete viewBoundReun;
        viewBoundReun = nullptr;
    }

    if (paso6){
        delete eControlador;
        eControlador = nullptr;
    }

    if (digBode){
        delete diagramaBode;
        diagramaBode = nullptr;
        digBode = false;
    }

    if (paso7){
        delete loopShaping;
        loopShaping = nullptr;
        delete viewLoopShaping;
        viewLoopShaping = nullptr;
    }
}

void WindowsGeneral::destruir(){
    destruirDialogos();

    delete controlador;
}

void WindowsGeneral::on_BDefiPlanta_clicked()
{
    if (!paso1){
        intPlanta = new IntroducirPlanta(controlador, this);
    }

    intPlanta->exec();

    if (intPlanta->getTodoCorrecto()){
        if (paso3){
            ui->BTemp->setEnabled(true);
        }

        if (!paso1){
            posBarra++;
            ui->barraprogreso->setValue(posBarra);
        }

        paso1 = true;
    } else {
        delete intPlanta;
        intPlanta = nullptr;
        retrocederPaso(paso1);
    }
}

void WindowsGeneral::on_BEspi_clicked()
{

    if (!paso2){
        especificaciones = new IntEspecificaciones(controlador, this);

    }

    especificaciones->exec();

    if (especificaciones->getTodoCorrecto()){

        if (paso4){
            ui->BBoun->setEnabled(true);
        }

        if (!paso2){
            posBarra++;
            ui->barraprogreso->setValue(posBarra);
        }
        paso2 = true;
    } else {
        delete especificaciones;
        especificaciones = nullptr;
        retrocederPaso(paso2);
    }
}

void WindowsGeneral::on_BFrec_clicked()
{
    if (!paso3){
        intOmega = new IntOmega(controlador,this);
    }

    intOmega->exec();

    if (intOmega->getTodoCorrecto()){

        if (paso1){
            ui->BTemp->setEnabled(true);
        }

        ui->BEspi->setEnabled(true);
        ui->barraDiagramaBode->setEnabled(true);

        if (!paso3){
            posBarra++;
            ui->barraprogreso->setValue(posBarra);
        }

        paso3 = true;

    } else {
        delete intOmega;
        intOmega = nullptr;
        retrocederPaso(paso3);
    }
}

void WindowsGeneral::on_BTemp_clicked()
{

    if (!paso4){
        vTemplates = new IntroducirTemplates(this);
        graficoTemplate = new ViewTemplates(this);
    }

    vTemplates->lanzarViewTemp(controlador->getPlanta(), controlador->getOmega()->getValores()->size());

    vTemplates->exec();

    if (vTemplates->getTodoCorrecto()){

        this->setCursor(Qt::WaitCursor);

        bool templatesOk = false;

        try {
            templatesOk = controlador->calcularTemplates(vTemplates->getEpsilon(), vTemplates->getMapa(),
                                                         vTemplates->getElecCUDA());
        } catch (const qftbx::Exception & e) {
            this->setCursor(Qt::ArrowCursor);
            QMessageBox::critical(this, tr("Calculo de Templates"), e.what());
            delete vTemplates;
            vTemplates = nullptr;
            delete graficoTemplate;
            graficoTemplate = nullptr;
            retrocederPaso(paso4);
            return;
        }

        if (templatesOk){

            this->setCursor(Qt::ArrowCursor);


            if (paso2){
                ui->BBoun->setEnabled(true);
            }

            graficoTemplate->setDatos(controlador);
            graficoTemplate->pintarGrafico(vTemplates->getElecDiagram());

            graficoTemplate->show();

            if (!paso4){
                posBarra++;
                ui->barraprogreso->setValue(posBarra);
            }

            paso4 = true;
        } else {
            delete vTemplates;
            vTemplates = nullptr;
            delete graficoTemplate;
            graficoTemplate = nullptr;
            retrocederPaso(paso4);
        }
        this->setCursor(Qt::ArrowCursor);
    } else {
        delete vTemplates;
        vTemplates = nullptr;
        delete graficoTemplate;
        graficoTemplate = nullptr;
        retrocederPaso(paso4);
    }
}

void WindowsGeneral::on_BBoun_clicked()
{

    if (!paso5){
        datosBoun = new IntDatosBoundaries(this);
        viewBound = new ViewBound(this);
        viewBoundReun = new ViewBoundReun(this);
    }

    datosBoun->exec();

    if (datosBoun->getTodoCorrecto()){

        this->setCursor(Qt::WaitCursor);

        bool boundariesOk = false;

        try {
            boundariesOk = controlador->calcularBoundaries(datosBoun->getDatosFas(),
                                                           datosBoun->getPuntosFas(), datosBoun->getDatosMag(),
                                                           datosBoun->getPuntosMag(), datosBoun->getInfinito(),
                                                           datosBoun->isContornoSelect(), datosBoun->getCUDA());
        } catch (const qftbx::Exception & e) {
            this->setCursor(Qt::ArrowCursor);
            QMessageBox::critical(this, tr("Calculo de Boundaries"), e.what());
            delete datosBoun;
            datosBoun = nullptr;
            delete viewBound;
            viewBound = nullptr;
            delete viewBoundReun;
            viewBoundReun = nullptr;
            retrocederPaso(paso5);
            return;
        }

        if (!boundariesOk){
            this->setCursor(Qt::ArrowCursor);

            delete datosBoun;
            datosBoun = nullptr;
            delete viewBound;
            viewBound = nullptr;
            delete viewBoundReun;
            viewBoundReun = nullptr;
            retrocederPaso(paso5);

            return;
        }

        this->setCursor(Qt::ArrowCursor);

        viewBound->setDatos(controlador->getBound(), controlador->getOmega()->getValores());
        viewBound->mostrarDiagrama();
        viewBound->show();

        viewBoundReun->setDatos(controlador->unionBoundaries(), controlador->getOmega()->getValores());
        viewBoundReun->mostrar_diagrama();
        viewBoundReun->show();

        if (!paso5){
            posBarra++;
            ui->barraprogreso->setValue(posBarra);
        }

        paso5 = true;

        if (paso6 && paso5){
            ui->BDiLaz->setEnabled(true);
        }

    }
}


void WindowsGeneral::on_BECont_clicked()
{

    if (!paso6){
        eControlador = new introducirEContr(controlador, this);
    }

    eControlador->exec();


    if (eControlador->getTodoCorrecto()){
        if (paso5){
            ui->BDiLaz->setEnabled(true);
        }

        if (!paso6){
            posBarra++;
            ui->barraprogreso->setValue(posBarra);
        }
        paso6 = true;
        //ui->menuDiagrama_Lazo->setEnabled(true);
    } else {
        delete eControlador;
        eControlador = nullptr;
        retrocederPaso(paso6);
    }
}

void WindowsGeneral::on_BDiLaz_clicked()
{
    if (!paso7){
        loopShaping = new IntLoopShaping(this);
        viewLoopShaping = new ViewLoopShaping(this);
    }


    loopShaping->exec();

    if (loopShaping->getTodoCorrecto()){
        bool re = false;

        try {
            re = controlador->calcularLoopShaping(loopShaping->getEpsilon(), loopShaping->getAlg(), loopShaping->range(),
                                                  loopShaping->getNPuntos(), loopShaping->getDepuracion(),
                                                  loopShaping->getDelta(), loopShaping->getInicializacion(),
                                                  loopShaping->getHilos(), loopShaping->getBisectionAvanced(),
                                                  loopShaping->getDeteccionAvanced(), loopShaping->getAcelerated());
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Loop Shaping"), e.what());
            delete loopShaping;
            loopShaping = nullptr;
            delete viewLoopShaping;
            viewLoopShaping = nullptr;
            retrocederPaso(paso7);
            return;
        }

        if (re){
            viewLoopShaping->setDatos(controlador->unionBoundaries(),controlador->getOmega()->getValores(),
                                      controlador->getLoopShaping(), controlador->getPlanta(), loopShaping->getLinLogSpace());

            viewLoopShaping->mostrar_diagrama();
            viewLoopShaping->show();

            if (!paso7){
                posBarra++;
                ui->barraprogreso->setValue(posBarra);
            }
            paso7 = true;
        } else {
            delete loopShaping;
            loopShaping = nullptr;
            delete viewLoopShaping;
            viewLoopShaping = nullptr;
            retrocederPaso(paso7);
        }
    } else {
        delete loopShaping;
        loopShaping = nullptr;
        delete viewLoopShaping;
        viewLoopShaping = nullptr;
        retrocederPaso(paso7);
    }
}

void WindowsGeneral::on_actionGuardar_triggered()
{
    if(ficheroGuardar.isEmpty()){
        on_actionGuardar_como_triggered();
    } else {
        guardar();
    }
}

void WindowsGeneral::on_actionGuardar_como_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Guardar Fichero"),"planta",
                                                    tr("QFT Files (*.qft)"));


    if (!fileName.isEmpty()){

        if (fileName.right(4) != ".qft"){
            ficheroGuardar = fileName+".qft";
        } else {
            ficheroGuardar = fileName;
        }
        guardar();
    }
}

void WindowsGeneral::guardar(){
    try {
        controlador->guardarSistema(ficheroGuardar);
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Guardar Fichero"), e.what());
    }
}

void WindowsGeneral::on_actionAbrir_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Abrir Fichero"),"planta",
                                                    tr("QFT Files (*.qft)"));

    if (!fileName.isEmpty()){

        QVector <bool> * leido;

        try {
            leido = controlador->cargarSistema(fileName);
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Cargar Fichero"), e.what());
            return;
        }

        //Los dialogos de la sesion anterior se liberan y la barra se
        //reinicia: antes cada apertura fugaba los dialogos existentes y la
        //barra acumulaba pasos entre ficheros.
        destruirDialogos();
        posBarra = 0;
        ui->barraprogreso->setValue(0);
        ui->BEspi->setEnabled(false);
        ui->BTemp->setEnabled(false);
        ui->BBoun->setEnabled(false);
        ui->BDiLaz->setEnabled(false);
        ui->barraDiagramaBode->setEnabled(false);

        //"Guardar" vuelve a escribir sobre el fichero recien abierto.
        ficheroGuardar = fileName;

        paso1 = leido->value(0);
        paso2 = leido->value(1);
        paso3 = leido->value(2);
        paso4 = leido->value(3);
        paso5 = leido->value(4);
        paso6 = leido->value(5);
        paso7 = leido->value(6);

        delete leido;


        if (paso1){
            intPlanta = new IntroducirPlanta(controlador, this);
            posBarra++;
            ui->barraprogreso->setValue(posBarra);
        }

        if (paso2){
            especificaciones = new IntEspecificaciones(controlador, this);
            posBarra++;
            ui->barraprogreso->setValue(posBarra);
        }

        if (paso3){
            intOmega = new IntOmega(controlador,this);
            posBarra++;
            ui->barraprogreso->setValue(posBarra);
            ui->BEspi->setEnabled(true);
            ui->barraDiagramaBode->setEnabled(true);
        }

        if (paso4){
            vTemplates = new IntroducirTemplates(this);
            graficoTemplate = new ViewTemplates(this);
            posBarra++;
            ui->barraprogreso->setValue(posBarra);
        }

        if (paso5){
            datosBoun = new IntDatosBoundaries(this);
            viewBound = new ViewBound(this);
            viewBoundReun = new ViewBoundReun (this);
            posBarra++;
            ui->barraprogreso->setValue(posBarra);
        }

        if (paso6){
            eControlador = new introducirEContr(controlador, this);
            posBarra++;
            ui->barraprogreso->setValue(posBarra);
            //ui->menuDiagrama_Lazo->setEnabled(true);
        }

        if (paso7){
            loopShaping = new IntLoopShaping(this);
            viewLoopShaping = new ViewLoopShaping(this);
            posBarra++;
            ui->barraprogreso->setValue(posBarra);
        }

        if (paso1 && paso3){
            ui->BTemp->setEnabled(true);
        }

        if (paso4 && paso2){
            ui->BBoun->setEnabled(true);

        }

        if (paso5 && paso6){
            ui->BDiLaz->setEnabled(true);
        }

        ui->barraprogreso->setValue(posBarra);
    }

}

void WindowsGeneral::on_actionConsola_triggered()
{
    consola con;
    con.mostrar();
}

void WindowsGeneral::on_actionNuevo_triggered()
{
    destruir();
    crear();
}

/*void WindowsGeneral::on_actionDiagrama_de_Bode_2_triggered()
{
    if (paso1 && paso3){

        if(!digBode)
            diagramaBode = new DiagramaBode(this);

        digBode = true;

        diagramaBode->dibujarBode(controlador->getPlanta(),controlador->getOmega());
        diagramaBode->show();
    }else{
        menerror("Para poder ver el Diagrama de Bode tiene que introducir primero una planta válida y un conjunto de frecuencias Omega", "QFT");
    }
}*/

void WindowsGeneral::on_actionDiagrama_Lazo_Nichols_2_triggered()
{
    mostrarLazo(true, false);
}

void WindowsGeneral::on_actionDiagrama_Lazo_Nyquist_triggered()
{
    mostrarLazo(false, true);
}

void WindowsGeneral::on_actionTodos_los_Diagramas_2_triggered()
{
    mostrarLazo(true, true);
}

void WindowsGeneral::mostrarLazo(bool nichols, bool nyquist){

    //Sin boundaries y estructura del controlador no hay lazo que mostrar
    //(antes se desreferenciaban DAOs sin inicializar).
    if (!paso5 || !paso6){
        menerror("Para ver el diagrama del lazo hay que calcular antes los boundaries e introducir la estructura del controlador.", "QFT");
        return;
    }

    qreal maglineal = 0;

    BoundaryData * boundaries = controlador->getBound();

    QVector< QVector<QPointF> * > * boun = boundaries->unionBoundaries();

    QVector< QVector<QPointF> * > * nuevosBoundariesReun =
            new QVector< QVector<QPointF> * > ();


    QVector< QVector< QVector<QPointF> * > * > * nuevoHash_inter = new QVector< QVector< QVector<QPointF> * > * > ();

    foreach (auto vector, *boun) {

        QVector<QPointF> * nuevoVector = new QVector<QPointF>  ();

        QVector <QVector <QPointF> * > * nuevoHash = new QVector <QVector <QPointF> *> ();

        foreach (auto p, *vector) {
            maglineal = pow(10,p.y()/20);

            QPointF punto (maglineal * cos (p.x() * M_PI / 180),
                           maglineal * sin (p.x() * M_PI / 180));

            nuevoVector->append(punto);


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


    verBoundaries * ver = new verBoundaries();

    ver->setDatos(boundaries, nuevoBoundaries, controlador->getOmega()->getValores(), controlador->getPlanta(),
                  controlador->getControlador(), nichols, nyquist);

    ver->mostrar_diagrama();

    ver->exec();

    //BoundaryData es una vista no propietaria: los contenedores temporales
    //construidos aqui se liberan aparte (antes se abandonaban).
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

void WindowsGeneral::on_actionTemplates_triggered()
{
    //Accion de "volver a ver": si no hay templates calculados no hay nada
    //que mostrar (antes marcaba el paso como hecho sin datos).
    if (!paso4){
        return;
    }

    if (controlador->getTemplate() != nullptr && controlador->getContorno() != nullptr){
        graficoTemplate->setDatos(controlador);
        graficoTemplate->pintarGrafico(true);

        graficoTemplate->show();
    }
}

void WindowsGeneral::on_actionBoundaries_triggered()
{
    if (!paso5){
        return;
    }

    viewBoundReun->setDatos(controlador->unionBoundaries(), controlador->getOmega()->getValores());
    viewBoundReun->mostrar_diagrama();
    viewBoundReun->show();
}

void WindowsGeneral::on_actionLazo_triggered()
{
    if (!paso7){
        return;
    }

    viewLoopShaping->setDatos(controlador->unionBoundaries(),controlador->getOmega()->getValores(),
                              controlador->getLoopShaping(), controlador->getPlanta(), loopShaping->getLinLogSpace());

    viewLoopShaping->mostrar_diagrama();
    viewLoopShaping->show();
}
