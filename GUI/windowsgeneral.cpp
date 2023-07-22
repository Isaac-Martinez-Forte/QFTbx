#include "windowsgeneral.h"
#include "ui_windowsgeneral.h"

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

    ui->barraprogreso->setRange(0,8);

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
}

void WindowsGeneral::destruir(){
    if (paso1){
        delete intPlanta;
    }

    if (paso2){
        delete especificaciones;
    }

    if (paso3){
        delete intOmega;
    }

    if (paso4){
        delete vTemplates;
        delete graficoTemplate;
    }

    if (paso5){
        delete datosBoun;
        delete viewBound;
        delete viewBoundReun;
    }

    if (paso6){
        delete eControlador;
    }

    if (digBode){
        delete diagramaBode;
    }

    if (paso7){
        delete loopShaping;
        delete viewLoopShaping;
    }

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
        paso1 = false;
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
        paso2 = false;
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
        paso3 = false;
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

        if (controlador->calcularTemplates(vTemplates->getEpsilon(), vTemplates->getMapa(),
                                           vTemplates->getElecCUDA())){

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
            delete graficoTemplate;
            paso4 = false;
        }
        this->setCursor(Qt::ArrowCursor);
    } else {
        delete vTemplates;
        delete graficoTemplate;
        paso4 = false;
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

        if (datosBoun->isContornoSelect()){

            if (!controlador->calcularBoundaries(datosBoun->getDatosFas(),
                                                 datosBoun->getPuntosFas(), datosBoun->getDatosMag(),
                                                 datosBoun->getPuntosMag(), datosBoun->getInfinito(), true, datosBoun->getCUDA())){
                this->setCursor(Qt::ArrowCursor);

                delete datosBoun;
                delete viewBound;
                delete viewBoundReun;
                paso5 = false;

                return;
            }
        }else {
            if (!controlador->calcularBoundaries(datosBoun->getDatosFas(),
                                                 datosBoun->getPuntosFas(), datosBoun->getDatosMag(),
                                                 datosBoun->getPuntosMag(), datosBoun->getInfinito(), false, datosBoun->getCUDA())){
                this->setCursor(Qt::ArrowCursor);

                delete datosBoun;
                delete viewBound;
                delete viewBoundReun;
                paso5 = false;

                return;
            }
        }

        this->setCursor(Qt::ArrowCursor);

        viewBound->setDatos(controlador->getBound(), controlador->getOmega()->getValores());
        viewBound->mostrarDiagrama();
        viewBound->show();

        viewBoundReun->setDatos(controlador->getBoundariesReun(), controlador->getOmega()->getValores());
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
        paso6 = false;
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
        bool re = controlador->calcularLoopShaping(loopShaping->getEpsilon(), loopShaping->getAlg(), loopShaping->getRango(),
                                                   loopShaping->getNPuntos(), loopShaping->getDepuracion(),
                                                   loopShaping->getDelta(), loopShaping->getInicializacion(),
                                                   loopShaping->getHilos(), loopShaping->getBisectionAvanced(),
                                                   loopShaping->getDeteccionAvanced(), loopShaping->getAcelerated());

        if (re){
            viewLoopShaping->setDatos(controlador->getBoundariesReun(),controlador->getOmega()->getValores(),
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
            delete viewLoopShaping;
            paso7 = false;
        }
    } else {
        delete loopShaping;
        delete viewLoopShaping;
        paso7 = false;
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
    controlador->guardarSistema(ficheroGuardar);
}

void WindowsGeneral::on_actionAbrir_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Abrir Fichero"),"planta",
                                                    tr("QFT Files (*.qft)"));

    if (!fileName.isEmpty()){

        QVector <bool> * leido = controlador->cargarSistema(fileName);

        paso1 = leido->value(0);
        paso2 = leido->value(1);
        paso3 = leido->value(2);
        paso4 = leido->value(3);
        paso5 = leido->value(4);
        paso6 = leido->value(5);
        paso7 = leido->value(6);


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

    qreal maglineal = 0;

    DatosBound * boundaries = controlador->getBound();

    QVector< QVector<QPointF> * > * boun = boundaries->getBoundariesReun();

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




    QPointF nuevoDatosFas ((boundaries->getDatosFas().x() * M_PI) / 180, 0);

    QPointF datosMag = boundaries->getDatosMag();

    QPointF nuevosDatosMag (pow(10,datosMag.x()/20), pow(10,datosMag.y()/20));

    DatosBound * nuevoBoundaries = new DatosBound (boundaries->getBoundaries(), boundaries->getMetaDatosAbierta(),
                                                   boundaries->getMetaDatosArriba(), boundaries->getTamFas(),
                                                   nuevoDatosFas, nuevosBoundariesReun,
                                                   nuevoHash_inter,
                                                   boundaries->getTamMag(), nuevosDatosMag);


    verBoundaries * ver = new verBoundaries();

    ver->setDatos(boundaries, nuevoBoundaries, controlador->getOmega()->getValores(), controlador->getPlanta(),
                  controlador->getControlador(), nichols, nyquist);

    ver->mostrar_diagrama();

    ver->exec();

    delete nuevoBoundaries;
    delete ver;
}

void WindowsGeneral::on_actionTemplates_triggered()
{
    if (!paso4){
        vTemplates = new IntroducirTemplates(this);
        graficoTemplate = new ViewTemplates(this);
    }

    if (controlador->getTemplate() != nullptr && controlador->getContorno() != nullptr){
        graficoTemplate->setDatos(controlador);
        graficoTemplate->pintarGrafico(true);

        graficoTemplate->show();
    }

    if (!paso4){
        posBarra++;
        ui->barraprogreso->setValue(posBarra);
    }

    paso4 = true;
}

void WindowsGeneral::on_actionBoundaries_triggered()
{
    if (!paso5){
        datosBoun = new IntDatosBoundaries(this);
        viewBound = new ViewBound(this);
        viewBoundReun = new ViewBoundReun(this);
    }

    /*viewBound->setDatos(controlador->getBound(), controlador->getOmega()->getValores());
    viewBound->mostrarDiagrama();
    viewBound->show();*/

    //viewBoundReun->setDatos(controlador->getBoundariesReunHash(), controlador->getOmega()->getValores(), controlador->getBoundariesReun()->last());
    viewBoundReun->setDatos(controlador->getBoundariesReun(), controlador->getOmega()->getValores());
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

void WindowsGeneral::on_actionLazo_triggered()
{
    if (!paso7){
        return;
    }

    viewLoopShaping->setDatos(controlador->getBoundariesReun(),controlador->getOmega()->getValores(),
                              controlador->getLoopShaping(), controlador->getPlanta(), loopShaping->getLinLogSpace());

    viewLoopShaping->mostrar_diagrama();
    viewLoopShaping->show();

    if (!paso7){
        posBarra++;
        ui->barraprogreso->setValue(posBarra);
    }
    paso7 = true;

}
