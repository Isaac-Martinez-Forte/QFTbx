#include "boundary_grid_dialog.h"
#include "ui_boundary_grid_dialog.h"

#include "src/gui/error_message.h"


using namespace tools;
using namespace std;

BoundaryGridDialog::BoundaryGridDialog(QWidget *parent) :
    StepDialog(parent),
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

}

BoundaryGridDialog::~BoundaryGridDialog()
{
}

qftbx::Range BoundaryGridDialog::phaseRangeValue(){
    return phaseRange;
}

qftbx::Range BoundaryGridDialog::magnitudeRangeValue(){
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

namespace {
//Ten million cells is far past any sensible Nichols grid and still a
//comfortable allocation.
constexpr std::int64_t kMaxGridCells = 10000000;
}

void BoundaryGridDialog::on_buttonBox_accepted()
{
    if (ui->infinityEdit->text().isEmpty()){
        infinityEdit = -1;
    }else{
        infinityEdit = ui->infinityEdit->text().toDouble();
    }

    phaseRange = qftbx::Range(ui->phaseStart->text().toDouble(),ui->phaseEnd->text().toDouble());
    magnitudeRange = qftbx::Range(ui->magnitudeStart->text().toDouble(),ui->magnitudeEnd->text().toDouble());

    phaseCount = ui->phasePoints->text().toInt();
    magnitudeCount = ui->magnitudePoints->text().toInt();

    //The grid must make sense before launching the computation: increasing
    //ranges and at least two points per axis (any value used to go straight
    //into the engine).
    if (phaseRange.min >= phaseRange.max || magnitudeRange.min >= magnitudeRange.max ||
            phaseCount < 2 || magnitudeCount < 2){
        tools::errorMessage(tr("The grid ranges must be increasing, with at least 2 points per axis."), tr("Boundary grid input"));
        return;
    }

    //And a ceiling. There was none: the counts only had to be >= 2, so an
    //extra couple of zeros in either field reached tools::linspace, which
    //reserves that many doubles - a std::bad_alloc, which is not the
    //qftbx::Exception the computation is wrapped in, so the application went
    //down on a typo. The budget guards against that typo; it is not a
    //control-design limit, and it can be raised. For scale, a 1-degree phase
    //grid over 360 degrees is 360 points per axis.
    if (static_cast<std::int64_t>(phaseCount) * magnitudeCount > kMaxGridCells){
        tools::errorMessage(tr("The grid asks for %1 cells, and the limit is "
                               "%2. Reduce the number of points per axis.")
                                .arg(static_cast<std::int64_t>(phaseCount) * magnitudeCount)
                                .arg(kMaxGridCells),
                            tr("Boundary grid input"));
        return;
    }

    accepted_once = true;

    //Direct read: the old latch left CUDA enabled forever once checked.
    cudaCheck = ui->cudaCheck->isChecked();

    markAccepted();
}

void BoundaryGridDialog::showEvent(QShowEvent * event)
{
    //Reopening and cancelling must not relaunch the computation with the old data.
    QDialog::showEvent(event);
}

bool BoundaryGridDialog::cudaSelected(){
    return cudaCheck;
}

