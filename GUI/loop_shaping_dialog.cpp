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

    ui->startEdit->setText("10^-9");
    ui->endEdit->setText("10^1");
    ui->pointCountEdit->setText("100");

    ui->deltaEdit->setText("10");

    //ui->bisectionCheck->setCheckState(Qt::Checked);

#ifndef DEBUG
   // ui->debugCheck->setVisible(false);
#endif

    linLogSpace = false;
    debugCheck = false;

    todoCorrecto = false;
}

LoopShapingDialog::~LoopShapingDialog()
{
    delete ui;
}

void LoopShapingDialog::showEvent(QShowEvent * event)
{
    //Reopening and cancelling must not relaunch the computation with the old data.
    todoCorrecto = false;
    QDialog::showEvent(event);
}

void LoopShapingDialog::setEpsilonValue(qreal epsilonEdit){
    ui->epsilonEdit->setText(QString::number(epsilonEdit));
}

void LoopShapingDialog::on_cancelButton_clicked()
{
    this->close();
}

void LoopShapingDialog::on_okButton_clicked()
{
    ParserX p (mup::pckALL_NON_COMPLEX);

    //Evaluate epsilon.
    p.SetExpr(ui->epsilonEdit->text().toStdString());

    try {
        epsilonEdit = p.Eval().GetFloat();
        ui->epsilonEdit->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::errorMessage(tr("Invalid epsilonEdit expression."), tr("Loop Shaping"));
        ui->epsilonEdit->setStyleSheet("background : red");
        return;
    }

    //Evaluate the start frequency:
    p.SetExpr(ui->startEdit->text().toStdString());

    try {
        plotRange.setX(p.Eval().GetFloat());
        ui->startEdit->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::errorMessage(tr("Invalid start-frequency expression."), tr("Loop Shaping"));
        ui->startEdit->setStyleSheet("background : red");
        return;
    }

    //Evaluate the end frequency:
    p.SetExpr(ui->endEdit->text().toStdString());

    try {
        plotRange.setY(p.Eval().GetFloat());
        ui->endEdit->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::errorMessage(tr("Invalid end-frequency expression."), tr("Loop Shaping"));
        ui->endEdit->setStyleSheet("background : red");
        return;
    }

    //Evaluate the point count:
    p.SetExpr(ui->pointCountEdit->text().toStdString());

    try {
        pointCountEdit = p.Eval().GetFloat();
        ui->pointCountEdit->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::errorMessage(tr("Invalid point-count expression."), tr("Loop Shaping"));
        ui->pointCountEdit->setStyleSheet("background : red");
        return;
    }

    if (ui->nkRadio->isChecked()){

        //NOTE (phase 8): the duplicated inner condition made
        //nandkishor_primeraversion unreachable; decide there which control
        //should select it.
        alg = tools::nandkishor;

        if (ui->randomInit->isChecked()){
            initialisation = 2;
        } else if (ui->upperInit->isChecked()){
            initialisation = 1;
        } else {
            initialisation = 0;
        }

        //Evaluate delta:
        p.SetExpr(ui->deltaEdit->text().toStdString());

        try {
            deltaEdit = p.Eval().GetFloat();
            ui->deltaEdit->setStyleSheet("background : white");
        } catch (mup::ParserError &e){
            tools::errorMessage(tr("Invalid deltaEdit expression."), tr("Loop Shaping"));
            ui->deltaEdit->setStyleSheet("background : red");
            return;
        }

    } else if (ui->mrRadio->isChecked()){
        alg = tools::rambabu;
    }else if (ui->mcRadio->isChecked()){
        alg = tools::primer_articulo;
    } else if (ui->ntRadio->isChecked()){
        alg = tools::segundo_articulo;
    } else {
        alg = tools::sachin;
    }

    //Direct read: the old latch left linspace selected forever once
    //checked.
    linLogSpace = ui->linspaceRadio->isChecked();

    //if (ui->debugCheck->isChecked()){
     //   debugCheck = true;
    //}

    /*if (ui->threadsCheck->isChecked()){
        threadsCheck = true;
    } else {*/
        threadsCheck = false;
    //}

    todoCorrecto = true;

    this->close();
}

bool LoopShapingDialog::threadsValue(){
    return threadsCheck;
}

bool LoopShapingDialog::getTodoCorrecto(){
    return todoCorrecto;
}


qreal LoopShapingDialog::epsilonValue(){
    return epsilonEdit;
}

tools::alg_loop_shaping LoopShapingDialog::algorithmValue(){
    return alg;
}

QPointF LoopShapingDialog::range(){
    return plotRange;
}

qreal LoopShapingDialog::pointCountValue(){
    return pointCountEdit;
}

qreal LoopShapingDialog::deltaValue(){
    return deltaEdit;
}

bool LoopShapingDialog::isLinSpace(){
    return linLogSpace;
}

bool LoopShapingDialog::debugValue(){
    return debugCheck;
}

qint32 LoopShapingDialog::initialisationValue(){
    return initialisation;
}

bool LoopShapingDialog::bisectionValue () {
    return ui->bisectionCheck->checkState() == Qt::Checked;
}

bool LoopShapingDialog::detectionValue() {
    return ui->detectionCheck->checkState() == Qt::Checked;
}

bool LoopShapingDialog::acceleratedValue() {
    return ui->acceleratedCheck->checkState() == Qt::Checked;
}

void LoopShapingDialog::on_linspaceRadio_clicked()
{
    ui->startEdit->setText("10^-4");
    ui->endEdit->setText("10^4");
    ui->pointCountEdit->setText("1000");
}

void LoopShapingDialog::on_logspaceRadio_clicked()
{
    ui->startEdit->setText("10^-6");
    ui->endEdit->setText("10^1");
    ui->pointCountEdit->setText("1000");
}

void LoopShapingDialog::on_sachin_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

void LoopShapingDialog::on_nkRadio_clicked()
{
    ui->algorithmStack->setCurrentIndex(2);
}

void LoopShapingDialog::on_mrRadio_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

//NOTE (phase 8): dead since the original code - there is no radio wired to
//the MC-prev algorithm (see the unreachable nandkishor_primeraversion in
//on_okButton_clicked); decide there whether to add the radio or remove the
//algorithm variant.
