/**
 * @file
 * @brief Reads and validates the loop-shaping inputs.
 *
 * One helper evaluates a field as an expression and accepts it only when
 * finite and within the caller's bounds, since an expression that parses
 * can still be useless and a NaN fails the range test. The complaint is
 * the caller's words, on the field and in a message box, because it stops
 * a computation just asked for. The epsilon label says which quantity is
 * meant: for MR the width of the controller's parameter box (Rambabu and
 * Nataraj), for the others the diameter of the Nichols box. The sweep is
 * logarithmic unless chosen otherwise, both range modes prefill the same
 * configured range in rad/s, and algorithms run only from the benchmark
 * have no radio.
 */

#include "src/gui/loopshaping/loop_shaping_form.h"
#include "src/gui/common/expression_field.h"
#include "src/gui/common/field_mark.h"
#include "src/gui/common/number_text.h"
#include "ui_loop_shaping_form.h"

#include "src/gui/application/error_message.h"

#include <QLineEdit>
#include <QRadioButton>

#include <cmath>
#include <limits>

namespace qftbx {

namespace {

bool readField(QLineEdit * field, const QString & complaint,
               double lowest, double highest, double & value)
{
    const double parsed = evaluateNumber(field->text()).value_or(std::numeric_limits<double>::quiet_NaN());

    if (!std::isfinite(parsed) || parsed < lowest || parsed > highest) {
        qftbx::markWrong(field, true, complaint);
        qftbx::errorMessage(complaint, QObject::tr("Loop-shaping input"));
        return false;
    }

    qftbx::markWrong(field, false);
    value = parsed;
    return true;
}

}

LoopShapingForm::LoopShapingForm(QWidget *parent) :
    StepPanel(parent),
    ui(std::make_unique<Ui::LoopShapingForm>())
{
    ui->setupUi(this);

    ui->logspaceRadio->setChecked(true);

    setWindowTitle(tr("Loop-shaping input"));

    applyDefaults(qftbx::Settings().defaults);

    for (QRadioButton * radio : {ui->ntRadio, ui->nkRadio, ui->mc1Radio,
                                 ui->mc2Radio, ui->mrRadio}) {
        connect(radio, &QRadioButton::toggled,
                this, &LoopShapingForm::updateEpsilonLabel);
    }

    updateEpsilonLabel();
}

LoopShapingForm::~LoopShapingForm()
{
}

void LoopShapingForm::updateEpsilonLabel()
{
    ui->epsilonLabel->setText(ui->mrRadio->isChecked()
                                  ? tr("Epsilon (controller parameter box width):")
                                  : tr("Epsilon (Nichols box diameter):"));
}

void LoopShapingForm::on_okButton_clicked()
{
    if (!readField(ui->epsilonEdit,
                   tr("The epsilon must be a positive real number."),
                   std::numeric_limits<double>::denorm_min(), m_maxMagnitude,
                   epsilonEdit)){
        return;
    }

    if (!readField(ui->startEdit,
                   tr("The start frequency must be a real number."),
                   -m_maxMagnitude, m_maxMagnitude, plotRange.min)){
        return;
    }

    if (!readField(ui->endEdit,
                   tr("The end frequency must be a real number."),
                   -m_maxMagnitude, m_maxMagnitude, plotRange.max)){
        return;
    }

    if (!readField(ui->pointCountEdit,
                   tr("The point count must be a whole number of at least 1."),
                   1.0, m_maxPointCount, pointCountEdit)){
        return;
    }

    if (ui->nkRadio->isChecked()){

        alg = qftbx::nk;

        initialisation = ui->upperInit->isChecked() ? 1 : 0;

    } else if (ui->mrRadio->isChecked()){
        alg = qftbx::mr;
    }else if (ui->mc1Radio->isChecked()){
        alg = qftbx::mc1;
    } else if (ui->mc2Radio->isChecked()){
        alg = qftbx::mc2;
    } else {
        alg = qftbx::nt;
    }

    linLogSpace = ui->linspaceRadio->isChecked();

    markAccepted();
}

qreal LoopShapingForm::epsilonValue(){
    return epsilonEdit;
}

bool LoopShapingForm::conservativeColumns() const {
    return ui->conservativeColumnsCheck->isChecked();
}

void LoopShapingForm::setConservativeColumns(bool on) {
    ui->conservativeColumnsCheck->setChecked(on);
}

qftbx::LoopShapingAlgorithm LoopShapingForm::algorithmValue(){
    return alg;
}

qftbx::Range LoopShapingForm::range(){
    return plotRange;
}

qreal LoopShapingForm::pointCountValue(){
    return pointCountEdit;
}

bool LoopShapingForm::isLinSpace(){
    return linLogSpace;
}

qint32 LoopShapingForm::initialisationValue(){
    return initialisation;
}

void LoopShapingForm::on_linspaceRadio_clicked()
{
    applyDefaults(m_defaults);
}

void LoopShapingForm::on_logspaceRadio_clicked()
{
    applyDefaults(m_defaults);
}

void LoopShapingForm::setFromProject(const qftbx::LoopShapingResult * result)
{
    if (result == nullptr) {
        return;
    }

    ui->startEdit->setText(qftbx::numberText(result->range().min));
    ui->endEdit->setText(qftbx::numberText(result->range().max));
    ui->pointCountEdit->setText(qftbx::numberText(result->pointCount()));

    const qftbx::LoopShapingResult::Run & run = result->run();
    if (run.epsilon > 0.0) {
        ui->epsilonEdit->setText(qftbx::numberText(run.epsilon));
    }
    ui->conservativeColumnsCheck->setChecked(run.conservativeColumns);

    switch (run.algorithm) {
    case qftbx::nk:  ui->nkRadio->setChecked(true);  break;
    case qftbx::mr:  ui->mrRadio->setChecked(true);  break;
    case qftbx::mc1: ui->mc1Radio->setChecked(true); break;
    case qftbx::mc2: ui->mc2Radio->setChecked(true); break;
    default:         ui->ntRadio->setChecked(true);  break;
    }
}

void LoopShapingForm::applyDefaults(const qftbx::Settings::Defaults & defaults)
{
    m_defaults = defaults;

    ui->startEdit->setText(qftbx::numberText(defaults.loopStart));
    ui->endEdit->setText(qftbx::numberText(defaults.loopEnd));
    ui->pointCountEdit->setText(qftbx::numberText(defaults.loopPointCount));
}

void LoopShapingForm::on_ntRadio_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

void LoopShapingForm::on_nkRadio_clicked()
{
    ui->algorithmStack->setCurrentIndex(1);
}

void LoopShapingForm::on_mrRadio_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

void LoopShapingForm::on_mc1Radio_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

void LoopShapingForm::on_mc2Radio_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

}
