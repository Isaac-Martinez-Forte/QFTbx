#include "loop_shaping_dialog.h"
#include "ui_loop_shaping_dialog.h"

#include "GUI/error_message.h"
#include "GUI/plot_palette.h"

using namespace mup;

LoopShapingDialog::LoopShapingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoopShapingDialog)
{
    ui->setupUi(this);

    setWindowTitle(tr("Loop-shaping input"));

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

LoopShapingDialog::~LoopShapingDialog()
{
    delete ui;
}

void LoopShapingDialog::showEvent(QShowEvent * event)
{
    //Reabrir y cancelar no debe relanzar el calculo con los datos antiguos.
    todoCorrecto = false;
    QDialog::showEvent(event);
}

void LoopShapingDialog::setDatos(qreal epsilon){
    ui->epsilon->setText(QString::number(epsilon));
}

void LoopShapingDialog::on_cancel_clicked()
{
    this->close();
}

void LoopShapingDialog::on_ok_clicked()
{
    ParserX p (mup::pckALL_NON_COMPLEX);

    //Resolvemos epsilón.
    p.SetExpr(ui->epsilon->text().toStdString());

    try {
        epsilon = p.Eval().GetFloat();
        ui->epsilon->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::errorMessage(tr("Invalid epsilon expression."), tr("Loop Shaping"));
        ui->epsilon->setStyleSheet("background : red");
        return;
    }

    //Resolvemos inicio frecuencias:
    p.SetExpr(ui->inicio->text().toStdString());

    try {
        rango.setX(p.Eval().GetFloat());
        ui->inicio->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::errorMessage(tr("Invalid start-frequency expression."), tr("Loop Shaping"));
        ui->inicio->setStyleSheet("background : red");
        return;
    }

    //Resolvemos final frecuencias:
    p.SetExpr(ui->final_2->text().toStdString());

    try {
        rango.setY(p.Eval().GetFloat());
        ui->final_2->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::errorMessage(tr("Invalid end-frequency expression."), tr("Loop Shaping"));
        ui->final_2->setStyleSheet("background : red");
        return;
    }

    //Resolvemos número de puntos:
    p.SetExpr(ui->nPuntos->text().toStdString());

    try {
        nPuntos = p.Eval().GetFloat();
        ui->nPuntos->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::errorMessage(tr("Invalid point-count expression."), tr("Loop Shaping"));
        ui->nPuntos->setStyleSheet("background : red");
        return;
    }

    if (ui->nand->isChecked()){

        //NOTA (fase 8): la condicion interior duplicada hace inalcanzable
        //nandkishor_primeraversion; decidir alli que control debe elegirla.
        alg = tools::nandkishor;

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
            tools::errorMessage(tr("Invalid delta expression."), tr("Loop Shaping"));
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

    //Lectura directa: el latch anterior dejaba linspace activado para
    //siempre tras marcarlo una vez.
    linLogSpace = ui->linspace->isChecked();

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

bool LoopShapingDialog::getHilos(){
    return hilos;
}

bool LoopShapingDialog::getTodoCorrecto(){
    return todoCorrecto;
}


qreal LoopShapingDialog::getEpsilon(){
    return epsilon;
}

tools::alg_loop_shaping LoopShapingDialog::getAlg(){
    return alg;
}

QPointF LoopShapingDialog::range(){
    return rango;
}

qreal LoopShapingDialog::getNPuntos(){
    return nPuntos;
}

qreal LoopShapingDialog::getDelta(){
    return delta;
}

bool LoopShapingDialog::getLinLogSpace(){
    return linLogSpace;
}

bool LoopShapingDialog::getDepuracion(){
    return depuracion;
}

qint32 LoopShapingDialog::getInicializacion(){
    return inicializacion;
}

bool LoopShapingDialog::getBisectionAvanced () {
    return ui->bisectionAvanced->checkState() == Qt::Checked;
}

bool LoopShapingDialog::getDeteccionAvanced() {
    return ui->deteccionAvanced->checkState() == Qt::Checked;
}

bool LoopShapingDialog::getAcelerated() {
    return ui->aceleratedAvanced->checkState() == Qt::Checked;
}

void LoopShapingDialog::on_linspace_clicked()
{
    ui->inicio->setText("10^-4");
    ui->final_2->setText("10^4");
    ui->nPuntos->setText("1000");
}

void LoopShapingDialog::on_logspace_clicked()
{
    ui->inicio->setText("10^-6");
    ui->final_2->setText("10^1");
    ui->nPuntos->setText("1000");
}

void LoopShapingDialog::on_sachin_clicked()
{
    ui->datosAlg->setCurrentIndex(0);
}

void LoopShapingDialog::on_nand_clicked()
{
    ui->datosAlg->setCurrentIndex(2);
}

void LoopShapingDialog::on_ram_clicked()
{
    ui->datosAlg->setCurrentIndex(0);
}

void LoopShapingDialog::on_isaac_clicked()
{
    ui->datosAlg->setCurrentIndex(1);
}
