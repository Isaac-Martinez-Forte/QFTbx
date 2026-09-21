/**
 * @file
 * @brief Builds a design frequency set from whichever mode is on screen.
 *
 * The counts take integer validators with the settings' ceiling, which is
 * re-applied rather than stored because the validators hold it; a
 * fractional or enormous count would otherwise be truncated or overflow the
 * 32-bit count the sequence generators take. Both range modes ask for
 * rad/s and the logarithmic one converts to exponents here, since the
 * generator takes exponents; a zero or negative end is refused before the
 * logarithm. Whatever the mode, the result is checked once: non-empty and
 * every value a positive real. A set from a file is shown as its values,
 * not its path, since the project carries the numbers and the file may be
 * gone.
 */

#include <QIntValidator>
#include "src/gui/frequencies/frequencies_form.h"
#include "src/gui/common/number_text.h"
#include "src/core/common/text_tokens.h"
#include "ui_frequencies_form.h"
#include "src/gui/common/field_mark.h"

#include <vector>
#include <cmath>

#include <QMessageBox>

#include "src/gui/application/error_message.h"
#include "src/core/common/exception.h"

namespace qftbx {

FrequenciesForm::FrequenciesForm(QWidget *parent) :
    StepPanel(parent),
    ui(std::make_unique<Ui::FrequenciesForm>())
{

    ui->setupUi(this);
    setWindowTitle(tr("Design frequencies input"));

    ui->logEnd->setValidator(new QDoubleValidator(this));
    ui->logStart->setValidator(new QDoubleValidator(this));
    applyFrequencyCountLimit(qftbx::Settings().limits.maxFrequencyCount);

    ui->linEnd->setValidator(new QDoubleValidator(this));
    ui->linStart->setValidator(new QDoubleValidator(this));

}

FrequenciesForm::~FrequenciesForm()
{
}
void FrequenciesForm::on_fileButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()){
        filePath=fileName;
        ui->filePathLabel->setText(filePath);
    }
}

void FrequenciesForm::applyFrequencyCountLimit(std::int32_t count)
{
    m_maxFrequencyCount = count;

    ui->logCount->setValidator(new QIntValidator(1, m_maxFrequencyCount, this));
    ui->linCount->setValidator(new QIntValidator(1, m_maxFrequencyCount, this));
}

void FrequenciesForm::on_okButton_clicked()
{
    qreal start = 0;
    qreal end = 0;
    Omega::GenerationType type = Omega::Manual;
    std::vector<double> frequencies;

    if (ui->modeStack->currentIndex() == 0){
        const std::optional<std::vector<double>> parsed =
                qftbx::text::reals(ui->manualValues->text().toStdString());
        type = Omega::Manual;
        if (!parsed.has_value()){
            markWrong(ui->manualValues, true,
                      tr("These are not numbers separated by spaces."));
            return;
        }
        frequencies = parsed.value();
        markWrong(ui->manualValues, false);

    } else if (ui->modeStack->currentIndex() == 1) {
        start = ui->logStart->text().toDouble();
        end = ui->logEnd->text().toDouble();

        if (start <= 0.0 || end <= 0.0){
            errorMessage(tr("A logarithmic range needs both ends greater "
                            "than zero, in rad/s."),
                         tr("Design frequencies input"));
            return;
        }

        frequencies = logspace(std::log10(start), std::log10(end),
                               ui->logCount->text().toInt());
        type = Omega::LogSpace;

    }else if (ui->modeStack->currentIndex() == 2) {

        frequencies = linspace(ui->linStart->text().toDouble(),ui->linEnd->text().toDouble(),
                               ui->linCount->text().toInt());

        start = ui->linStart->text().toDouble();
        end = ui->linEnd->text().toDouble();
        type = Omega::LinSpace;

    } else {
        try {
            frequencies = Omega::valuesFromFile(filePath.toStdString());
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Design frequencies input"), translated(e));
            return;
        }
        type = Omega::File;
    }

    if (frequencies.empty()){
        errorMessage(tr("Enter at least one design frequency."),
                     tr("Design frequencies input"));
        return;
    }

    for (const qreal frequency : frequencies){
        if (!std::isfinite(frequency) || frequency <= 0.0){
            errorMessage(tr("A design frequency must be a positive real, and "
                            "%1 is not.").arg(frequency),
                         tr("Design frequencies input"));
            return;
        }
    }

    const qint32 pointCount = static_cast<qint32>(frequencies.size());

    m_omega = std::make_unique<Omega>(start, end, pointCount, std::move(frequencies), type);

    markAccepted();
}

void FrequenciesForm::setFromProject(const Omega * omega)
{
    if (omega == nullptr || omega->values() == nullptr || omega->values()->empty()) {
        return;
    }

    const std::vector<double> & values = *omega->values();

    QStringList listed;
    listed.reserve(static_cast<int>(values.size()));
    for (const double w : values) {
        listed << numberText(w);
    }
    ui->manualValues->setText(listed.join(" "));

    switch (omega->type()) {
    case Omega::LogSpace:
        ui->logStart->setText(numberText(omega->start()));
        ui->logEnd->setText(numberText(omega->end()));
        ui->logCount->setText(QString::number(omega->pointCount()));
        ui->modeStack->setCurrentIndex(1);
        break;
    case Omega::LinSpace:
        ui->linStart->setText(numberText(omega->start()));
        ui->linEnd->setText(numberText(omega->end()));
        ui->linCount->setText(QString::number(omega->pointCount()));
        ui->modeStack->setCurrentIndex(2);
        break;
    case Omega::File:
    case Omega::Manual:
    default:
        ui->modeStack->setCurrentIndex(0);
        break;
    }
}

std::unique_ptr<Omega> FrequenciesForm::takeOmega(){
    return std::move(m_omega);
}

}
