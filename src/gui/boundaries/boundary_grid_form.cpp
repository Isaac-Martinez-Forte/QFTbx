#include "src/gui/boundaries/boundary_grid_form.h"
#include "src/gui/common/number_text.h"
#include "ui_boundary_grid_form.h"

#include "src/gui/application/error_message.h"


namespace qftbx {

BoundaryGridForm::BoundaryGridForm(QWidget *parent) :
    StepPanel(parent),
    ui(std::make_unique<Ui::BoundaryGridForm>())
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

BoundaryGridForm::~BoundaryGridForm()
{
}

qftbx::Range BoundaryGridForm::phaseRangeValue(){
    return phaseRange;
}

qftbx::Range BoundaryGridForm::magnitudeRangeValue(){
    return magnitudeRange;
}

qint32 BoundaryGridForm::phaseCountValue(){
    return phaseCount;
}

qint32 BoundaryGridForm::magnitudeCountValue(){
    return magnitudeCount;
}

qreal BoundaryGridForm::infinityValue(){
    return infinityEdit;
}

bool BoundaryGridForm::contourSelected(){

    if (ui->fullTemplateRadio->isChecked()){
        return false;
    }

    return true;
}

void BoundaryGridForm::applyDefaults(const qftbx::Settings::Defaults & defaults)
{
    ui->phaseStart->setText(qftbx::numberText(defaults.phaseStart));
    ui->phaseEnd->setText(qftbx::numberText(defaults.phaseEnd));
    ui->phasePoints->setText(qftbx::numberText(defaults.phasePoints));

    ui->magnitudeStart->setText(qftbx::numberText(defaults.magnitudeStart));
    ui->magnitudeEnd->setText(qftbx::numberText(defaults.magnitudeEnd));
    ui->magnitudePoints->setText(qftbx::numberText(defaults.magnitudePoints));

    //Which of the two the run starts on; the user still chooses per run.
    if (defaults.boundariesFromCloud) {
        ui->fullTemplateRadio->setChecked(true);
    } else {
        ui->contorno->setChecked(true);
    }
}

void BoundaryGridForm::setFromProject(const qftbx::BoundaryData * boundaries)
{
    if (boundaries == nullptr) {
        return;
    }

    ui->phaseStart->setText(qftbx::numberText(boundaries->phaseRange().min));
    ui->phaseEnd->setText(qftbx::numberText(boundaries->phaseRange().max));
    ui->phasePoints->setText(QString::number(boundaries->phaseCount()));

    ui->magnitudeStart->setText(qftbx::numberText(boundaries->magnitudeRange().min));
    ui->magnitudeEnd->setText(qftbx::numberText(boundaries->magnitudeRange().max));
    ui->magnitudePoints->setText(QString::number(boundaries->magnitudeCount()));
}

void BoundaryGridForm::on_okButton_clicked()
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
    //ranges and at least two points per axis: what is here goes straight
    //into the engine.
    if (phaseRange.min >= phaseRange.max || magnitudeRange.min >= magnitudeRange.max ||
            phaseCount < 2 || magnitudeCount < 2){
        qftbx::errorMessage(tr("The grid ranges must be increasing, with at least 2 points per axis."), tr("Boundary grid input"));
        return;
    }

    //And a ceiling. There was none: the counts only had to be >= 2, so an
    //extra couple of zeros in either field reached qftbx::linspace, which
    //reserves that many doubles - a std::bad_alloc, which is not the
    //qftbx::Exception the computation is wrapped in, so the application went
    //down on a typo. The budget guards against that typo; it is not a
    //control-design limit, and it can be raised. For scale, a 1-degree phase
    //grid over 360 degrees is 360 points per axis. It comes from the
    //settings now, so it can be moved without a rebuild.
    if (static_cast<std::int64_t>(phaseCount) * magnitudeCount > m_maxGridCells){
        qftbx::errorMessage(tr("The grid asks for %1 cells, and the limit is "
                               "%2. Reduce the number of points per axis.")
                                .arg(static_cast<std::int64_t>(phaseCount) * magnitudeCount)
                                .arg(m_maxGridCells),
                            tr("Boundary grid input"));
        return;
    }

    //Read directly, not latched, so unchecking it takes effect.
    cudaCheck = ui->cudaCheck->isChecked();

    markAccepted();
}

bool BoundaryGridForm::cudaSelected(){
    return cudaCheck;
}

} // namespace qftbx
