#include "loop_shaping_dialog.h"
#include "ui_loop_shaping_dialog.h"

#include "GUI/error_message.h"
#include "GUI/plot_palette.h"

using namespace mup;

LoopShapingDialog::LoopShapingDialog(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::LoopShapingDialog>())
{
    ui->setupUi(this);

    setWindowTitle(tr("Loop-shaping input"));

    ui->startEdit->setText("10^-9");
    ui->endEdit->setText("10^1");
    ui->pointCountEdit->setText("100");

    linLogSpace = false;

    todoCorrecto = false;
}

LoopShapingDialog::~LoopShapingDialog()
{
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

        alg = tools::nk;

        initialisation = ui->upperInit->isChecked() ? 1 : 0;

    } else if (ui->mrRadio->isChecked()){
        alg = tools::mr;
    }else if (ui->mc1Radio->isChecked()){
        alg = tools::mc1;
    } else if (ui->mcThesisRadio->isChecked()){
        alg = tools::mc_thesis;
    } else {
        alg = tools::nt;
    }

    //Direct read: the old latch left linspace selected forever once
    //checked.
    linLogSpace = ui->linspaceRadio->isChecked();

    todoCorrecto = true;

    this->close();
}

bool LoopShapingDialog::getTodoCorrecto(){
    return todoCorrecto;
}


qreal LoopShapingDialog::epsilonValue(){
    return epsilonEdit;
}

tools::LoopShapingAlgorithm LoopShapingDialog::algorithmValue(){
    return alg;
}

QPointF LoopShapingDialog::range(){
    return plotRange;
}

qreal LoopShapingDialog::pointCountValue(){
    return pointCountEdit;
}

bool LoopShapingDialog::isLinSpace(){
    return linLogSpace;
}

qint32 LoopShapingDialog::initialisationValue(){
    return initialisation;
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

void LoopShapingDialog::on_ntRadio_clicked()
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

