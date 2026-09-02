#include "boundary_grid_dialog.h"
#include "ui_boundary_grid_dialog.h"

#include "src/gui/error_message.h"


using namespace tools;
using namespace std;

BoundaryGridDialog::BoundaryGridDialog(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::BoundaryGridDialog>())
{
    ui->setupUi(this);

    accepted_once = false;

    ui->phaseStart->setValidator(new QDoubleValidator(this));
    ui->phaseEnd->setValidator(new QDoubleValidator(this));
    ui->magnitudeStart->setValidator(new QDoubleValidator(this));
    ui->magnitudeEnd->setValidator(new QDoubleValidator(this));

    ui->phasePoints->setValidator(new QIntValidator(this));
    ui->magnitudePoints->setValidator(new QIntValidator(this));

    ui->infinityEdit->setValidator(new QDoubleValidator(this));

    ui->phaseStart->setText("-360");
    ui->phaseEnd->setText("0");
    ui->phasePoints->setText("361");

    ui->magnitudeStart->setText("-60");
    ui->magnitudeEnd->setText("60");
    ui->magnitudePoints->setText("121");

    cudaCheck = false;

    setWindowTitle(tr("Boundary grid input"));

#ifndef CUDA_AVAILABLE
    ui->cudaCheck->setVisible(false);
#endif

    accepted = false;
}

BoundaryGridDialog::~BoundaryGridDialog()
{
}

QPointF BoundaryGridDialog::phaseRangeValue(){
    return phaseRange;
}

QPointF BoundaryGridDialog::magnitudeRangeValue(){
    return magnitudeRange;
}

qint32 BoundaryGridDialog::phaseCountValue(){
    return phaseCount;
}

qint32 BoundaryGridDialog::magnitudeCountValue(){
    return magnitudeCount;
}

qreal BoundaryGridDialog::infinityValue(){
    return infinityEdit;
}

bool BoundaryGridDialog::contourSelected(){

    if (ui->fullTemplateRadio->isChecked()){
        return false;
    }

    return true;
}

void BoundaryGridDialog::on_buttonBox_accepted()
{
    if (ui->infinityEdit->text().isEmpty()){
        infinityEdit = -1;
    }else{
        infinityEdit = ui->infinityEdit->text().toDouble();
    }

    phaseRange = QPointF(ui->phaseStart->text().toDouble(),ui->phaseEnd->text().toDouble());
    magnitudeRange = QPointF(ui->magnitudeStart->text().toDouble(),ui->magnitudeEnd->text().toDouble());

    phaseCount = ui->phasePoints->text().toInt();
    magnitudeCount = ui->magnitudePoints->text().toInt();

    //The grid must make sense before launching the computation: increasing
    //ranges and at least two points per axis (any value used to go straight
    //into the engine).
    if (phaseRange.x() >= phaseRange.y() || magnitudeRange.x() >= magnitudeRange.y() ||
            phaseCount < 2 || magnitudeCount < 2){
        tools::errorMessage(tr("The grid ranges must be increasing, with at least 2 points per axis."), tr("Boundary grid input"));
        accepted = false;
        return;
    }

    accepted_once = true;

    //Direct read: the old latch left CUDA enabled forever once checked.
    cudaCheck = ui->cudaCheck->isChecked();

    accepted = true;
}

void BoundaryGridDialog::showEvent(QShowEvent * event)
{
    //Reopening and cancelling must not relaunch the computation with the old data.
    accepted = false;
    QDialog::showEvent(event);
}

bool BoundaryGridDialog::cudaSelected(){
    return cudaCheck;
}

bool BoundaryGridDialog::wasAccepted(){
    return accepted;
}
