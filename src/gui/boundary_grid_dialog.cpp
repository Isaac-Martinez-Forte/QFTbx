#include "boundary_grid_dialog.h"
#include "src/gui/number_text.h"
#include "ui_boundary_grid_dialog.h"

#include "src/gui/error_message.h"


using namespace tools;

BoundaryGridDialog::BoundaryGridDialog(QWidget *parent) :
    StepDialog(parent),
    ui(std::make_unique<Ui::BoundaryGridDialog>())
{
    ui->setupUi(this);

    ui->phaseStart->setValidator(new QDoubleValidator(this));
    ui->phaseEnd->setValidator(new QDoubleValidator(this));
    ui->magnitudeStart->setValidator(new QDoubleValidator(this));
    ui->magnitudeEnd->setValidator(new QDoubleValidator(this));

    ui->phasePoints->setValidator(new QIntValidator(this));
    ui->magnitudePoints->setValidator(new QIntValidator(this));

    ui->infinityEdit->setValidator(new QDoubleValidator(this));

    //Prefilled from the settings, which the window applies right after
    //construction; these are what stands until it does.
    applyDefaults(qftbx::Settings().defaults);

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

void BoundaryGridDialog::applyDefaults(const qftbx::Settings::Defaults & defaults)
{
    ui->phaseStart->setText(tools::numberText(defaults.phaseStart));
    ui->phaseEnd->setText(tools::numberText(defaults.phaseEnd));
    ui->phasePoints->setText(tools::numberText(defaults.phasePoints));

    ui->magnitudeStart->setText(tools::numberText(defaults.magnitudeStart));
    ui->magnitudeEnd->setText(tools::numberText(defaults.magnitudeEnd));
    ui->magnitudePoints->setText(tools::numberText(defaults.magnitudePoints));
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
    //grid over 360 degrees is 360 points per axis. It comes from the
    //settings now, so it can be moved without a rebuild.
    if (static_cast<std::int64_t>(phaseCount) * magnitudeCount > m_maxGridCells){
        tools::errorMessage(tr("The grid asks for %1 cells, and the limit is "
                               "%2. Reduce the number of points per axis.")
                                .arg(static_cast<std::int64_t>(phaseCount) * magnitudeCount)
                                .arg(m_maxGridCells),
                            tr("Boundary grid input"));
        return;
    }

    //Direct read: the old latch left CUDA enabled forever once checked.
    cudaCheck = ui->cudaCheck->isChecked();

    markAccepted();
}

bool BoundaryGridDialog::cudaSelected(){
    return cudaCheck;
}

