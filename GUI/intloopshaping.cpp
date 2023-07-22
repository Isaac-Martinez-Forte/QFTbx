#include "intloopshaping.h"
#include "ui_intloopshaping.h"

using namespace mup;

IntLoopShaping::IntLoopShaping(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IntLoopShaping)
{
    ui->setupUi(this);

    setWindowTitle("Introducir Datos Loop Shaping");

    ui->inicio->setText("10^-9");
    ui->final_2->setText("10^1");
    ui->nPuntos->setText("100");

    ui->delta->setText("10");

    //ui->bisectionAvanced->setCheckState(Qt::Checked);

#ifndef DEBUG
   // ui->depuracion->setVisible(false);
#endif

    linLogSpace = false;
    depuracion = false;

    todoCorrecto = false;
}

IntLoopShaping::~IntLoopShaping()
{
    delete ui;
}

void IntLoopShaping::setDatos(qreal epsilon){
    ui->epsilon->setText(QString::number(epsilon));
}

void IntLoopShaping::on_cancel_clicked()
{
    this->close();
}

void IntLoopShaping::on_ok_clicked()
{
    ParserX p (mup::pckALL_NON_COMPLEX);

    //Resolvemos epsilón.
    p.SetExpr(ui->epsilon->text().toStdString());

    try {
        epsilon = p.Eval().GetFloat();
        ui->epsilon->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::menerror("Error en la expresión matemática de Epsilón.", "Loop Shaping");
        ui->epsilon->setStyleSheet("background : red");
        return;
    }

    //Resolvemos inicio frecuencias:
    p.SetExpr(ui->inicio->text().toStdString());

    try {
        rango.setX(p.Eval().GetFloat());
        ui->inicio->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::menerror("Error en la expresión matemática del inicio de la frecuencia.", "Loop Shaping");
        ui->inicio->setStyleSheet("background : red");
        return;
    }

    //Resolvemos final frecuencias:
    p.SetExpr(ui->final_2->text().toStdString());

    try {
        rango.setY(p.Eval().GetFloat());
        ui->final_2->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::menerror("Error en la expresión matemática del final de la frecuencia.", "Loop Shaping");
        ui->final_2->setStyleSheet("background : red");
        return;
    }

    //Resolvemos número de puntos:
    p.SetExpr(ui->nPuntos->text().toStdString());

    try {
        nPuntos = p.Eval().GetFloat();
        ui->nPuntos->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::menerror("Error en la expresión matemática del número de puntos de la frecuencia.", "Loop Shaping");
        ui->nPuntos->setStyleSheet("background : red");
        return;
    }

    if (ui->nand->isChecked()){

        if (ui->nand->isChecked()){
            alg = tools::nandkishor;
        } else {
            alg = tools::nandkishor_primeraversion;
        }

        if (ui->aleatorio->isChecked()){
            inicializacion = 2;
        } else if (ui->superior->isChecked()){
            inicializacion = 1;
        } else {
            inicializacion = 0;
        }

        //Resolvemos el delta:
        p.SetExpr(ui->delta->text().toStdString());

        try {
            delta = p.Eval().GetFloat();
            ui->delta->setStyleSheet("background : white");
        } catch (mup::ParserError &e){
            tools::menerror("Error en la expresión matemática de delta.", "Loop Shaping");
            ui->delta->setStyleSheet("background : red");
            return;
        }

    } else if (ui->ram->isChecked()){
        alg = tools::rambabu;
    }else if (ui->primero->isChecked()){
        alg = tools::primer_articulo;
    } else if (ui->segundo->isChecked()){
        alg = tools::segundo_articulo;
    } else {
        alg = tools::sachin;
    }

    if(ui->linspace->isChecked()){
        linLogSpace = true;
    }

    //if (ui->depuracion->isChecked()){
     //   depuracion = true;
    //}

    /*if (ui->hilos->isChecked()){
        hilos = true;
    } else {*/
        hilos = false;
    //}

    todoCorrecto = true;

    this->close();
}

bool IntLoopShaping::getHilos(){
    return hilos;
}

bool IntLoopShaping::getTodoCorrecto(){
    return todoCorrecto;
}


qreal IntLoopShaping::getEpsilon(){
    return epsilon;
}

tools::alg_loop_shaping IntLoopShaping::getAlg(){
    return alg;
}

QPointF IntLoopShaping::getRango(){
    return rango;
}

qreal IntLoopShaping::getNPuntos(){
    return nPuntos;
}

qreal IntLoopShaping::getDelta(){
    return delta;
}

bool IntLoopShaping::getLinLogSpace(){
    return linLogSpace;
}

bool IntLoopShaping::getDepuracion(){
    return depuracion;
}

qint32 IntLoopShaping::getInicializacion(){
    return inicializacion;
}

bool IntLoopShaping::getBisectionAvanced () {
    return ui->bisectionAvanced->checkState() == Qt::Checked;
}

bool IntLoopShaping::getDeteccionAvanced() {
    return ui->deteccionAvanced->checkState() == Qt::Checked;
}

bool IntLoopShaping::getAcelerated() {
    return ui->aceleratedAvanced->checkState() == Qt::Checked;
}

void IntLoopShaping::on_linspace_clicked()
{
    ui->inicio->setText("10^-4");
    ui->final_2->setText("10^4");
    ui->nPuntos->setText("1000");
}

void IntLoopShaping::on_logspace_clicked()
{
    ui->inicio->setText("10^-6");
    ui->final_2->setText("10^1");
    ui->nPuntos->setText("1000");
}

void IntLoopShaping::on_sachin_clicked()
{
    ui->datosAlg->setCurrentIndex(0);
}

void IntLoopShaping::on_nand_clicked()
{
    ui->datosAlg->setCurrentIndex(2);
}

void IntLoopShaping::on_ram_clicked()
{
    ui->datosAlg->setCurrentIndex(0);
}

void IntLoopShaping::on_isaac_clicked()
{
    ui->datosAlg->setCurrentIndex(1);
}
