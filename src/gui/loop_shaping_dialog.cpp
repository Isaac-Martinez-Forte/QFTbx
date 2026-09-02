#include "loop_shaping_dialog.h"
#include "ui_loop_shaping_dialog.h"

#include "src/gui/error_message.h"
#include "src/gui/plot_palette.h"

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

    accepted = false;
}

LoopShapingDialog::~LoopShapingDialog()
{
}

void LoopShapingDialog::showEvent(QShowEvent * event)
{
    //Reopening and cancelling must not relaunch the computation with the old data.
    accepted = false;
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
        plotRange.min = p.Eval().GetFloat();
        ui->startEdit->setStyleSheet("background : white");
    } catch (mup::ParserError &e){
        tools::errorMessage(tr("Invalid start-frequency expression."), tr("Loop Shaping"));
        ui->startEdit->setStyleSheet("background : red");
        return;
    }

    //Evaluate the end frequency:
    p.SetExpr(ui->endEdit->text().toStdString());

    try {
        plotRange.max = p.Eval().GetFloat();
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

    accepted = true;

    this->close();
}

bool LoopShapingDialog::wasAccepted(){
    return accepted;
}


qreal LoopShapingDialog::epsilonValue(){
    return epsilonEdit;
}

tools::LoopShapingAlgorithm LoopShapingDialog::algorithmValue(){
    return alg;
}

qftbx::Range LoopShapingDialog::range(){
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
    //Page 1 is page_nand, which holds NK's starting-point choice. This
    //asked for page 2 and there are only two pages: Qt ignores an
    //out-of-range index in silence, so the panel was never shown and the
    //choice could not be made - the local search always started at the
    //centre.
    ui->algorithmStack->setCurrentIndex(1);
}

void LoopShapingDialog::on_mrRadio_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

//MC1 and MC (thesis) have no options of their own yet: page 0 is the empty
//one. Without these two the panel kept whatever the previous algorithm had
//put there.
void LoopShapingDialog::on_mc1Radio_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

void LoopShapingDialog::on_mcThesisRadio_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

