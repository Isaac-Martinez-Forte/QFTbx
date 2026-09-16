#include "src/gui/loopshaping/controller_dialog.h"
#include "ui_controller_dialog.h"

#include <algorithm>

#include "src/core/common/exception.h"
#include "src/gui/application/error_message.h"
#include "src/gui/common/number_text.h"
#include "src/gui/common/system_description_writer.h"

namespace qftbx {

ControllerDialog::ControllerDialog(QWidget *parent) :
    StepPanel(parent),
    ui(std::make_unique<Ui::ControllerDialog>()),
    m_reader(tr("Controller input"))
{
    ui->setupUi(this);
    setWindowTitle(tr("Controller structure input"));

    ui->zpkImage->setPixmap(QPixmap(":/figures/kgan.png"));
    ui->tcgImage->setPixmap(QPixmap(":/figures/knogan.png"));
    ui->polyImage->setPixmap(QPixmap(":/figures/copol.png"));

    ui->gainStart->setText("1");
    ui->gainEnd->setText("1");

    //The uncertainty dialog is created up front and reused.
    uncertaintyDialog = new UncertaintyDialog (this);
}

ControllerDialog::~ControllerDialog()
{
}

void ControllerDialog::on_polynomialRadio_clicked()
{
    ui->figureStack->setCurrentIndex(1);
}

void ControllerDialog::on_zpkRadio_clicked()
{
    ui->figureStack->setCurrentIndex(2);
}

void ControllerDialog::on_tcgRadio_clicked()
{
    ui->figureStack->setCurrentIndex(3);
}

LtiSystem::SystemType ControllerDialog::selectedType() const
{
    if (ui->zpkRadio->isChecked()) {
        return LtiSystem::SystemType::ZeroPoleGain;
    }
    if (ui->tcgRadio->isChecked()) {
        return LtiSystem::SystemType::TimeConstantGain;
    }
    if (ui->polynomialRadio->isChecked()) {
        return LtiSystem::SystemType::PolynomialForm;
    }
    return LtiSystem::SystemType::FreeForm;
}

QString ControllerDialog::currentCoefficients() const
{
    return ui->numeratorEdit->text() + QLatin1Char('\n') + ui->denominatorEdit->text();
}

QString ControllerDialog::currentGain() const
{
    return ui->gainStart->text() + QLatin1Char('\n') + ui->gainEnd->text();
}

void ControllerDialog::setFromProject(LtiSystem * structure)
{
    if (structure == nullptr) {
        return;
    }

    const SystemDescription described = describeSystem(*structure);

    switch (described.type) {
    case LtiSystem::SystemType::PolynomialForm:
        ui->polynomialRadio->setChecked(true);
        ui->figureStack->setCurrentIndex(1);
        break;
    case LtiSystem::SystemType::ZeroPoleGain:
        ui->zpkRadio->setChecked(true);
        ui->figureStack->setCurrentIndex(2);
        break;
    case LtiSystem::SystemType::TimeConstantGain:
        ui->tcgRadio->setChecked(true);
        ui->figureStack->setCurrentIndex(3);
        break;
    case LtiSystem::SystemType::FreeForm:
        ui->freeFormRadio->setChecked(true);
        break;
    }

    ui->numeratorEdit->setText(described.numerator);
    ui->denominatorEdit->setText(described.denominator);

    //The gain is a search box, not a value: its two ends are the fields.
    const Range gain = structure->gain().rawRange();
    ui->gainStart->setText(numberText(gain.min));
    ui->gainEnd->setText(numberText(gain.max));

    uncertaintyDialog->setParameters(structure->numerator(), structure->denominator(),
                                     gain, structure->delay().rawRange());

    m_projectGain = structure->gain();
    m_describedCoefficients = currentCoefficients();
    m_describedGain = currentGain();
}

std::optional<CoefficientTable> ControllerDialog::readTables(CoefficientTable & expressionTable,
                                                             UncertainTable & uncertainTable)
{
    //Rows in the order the uncertainty dialog expects: numerator,
    //denominator, gain range. && on the RIGHT so every field is read and
    //every problem is reported.
    CoefficientTable tables;
    bool valid = true;

    if (ui->freeFormRadio->isChecked()) {
        valid = m_reader.readFreeForm(ui->numeratorEdit->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readFreeForm(ui->denominatorEdit->text(), tables, expressionTable, uncertainTable) && valid;
    } else {
        valid = m_reader.readCoefficients(ui->numeratorEdit->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readCoefficients(ui->denominatorEdit->text(), tables, expressionTable, uncertainTable) && valid;
    }
    valid = m_reader.readGainRange(ui->gainStart->text(), ui->gainEnd->text(), tables, expressionTable, uncertainTable) && valid;

    if (!valid) {
        return std::nullopt;
    }

    return tables;
}

void ControllerDialog::on_uncertaintyButton_clicked()
{
    CoefficientTable expressionTable;
    UncertainTable uncertainTable;
    std::optional<CoefficientTable> valueTable = readTables(expressionTable, uncertainTable);

    if (!valueTable.has_value()) {
        errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
        return;
    }

    uncertaintyDialog->launch(std::move(*valueTable), std::move(expressionTable),
                              std::move(uncertainTable), true);
    uncertaintyDialog->show();
    uncertaintyEntered = true;
}

void ControllerDialog::on_okButton_clicked()
{
    CoefficientTable expressionTable;
    UncertainTable uncertainTable;
    const std::optional<CoefficientTable> valueTable = readTables(expressionTable, uncertainTable);

    if (!valueTable.has_value()) {
        errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
        return;
    }

    //A form filled from the project answers from the structure's own
    //parameters while its fields still describe them, field by field.
    const bool coefficientsFromProject = !m_describedCoefficients.isEmpty()
            && currentCoefficients() == m_describedCoefficients;
    const bool gainFromProject = !m_describedGain.isEmpty() && currentGain() == m_describedGain;

    //The gain: a constant when both ends agree, the search box "k" over
    //them otherwise (in either order).
    std::optional<Parameter> gain;
    try {
        if (gainFromProject) {
            gain = m_projectGain;
        } else {
            const std::optional<double> start = m_reader.evaluate(expressionTable.at(2).at(0));
            const std::optional<double> end = m_reader.evaluate(expressionTable.at(2).at(1));
            if (!start.has_value() || !end.has_value()) {
                errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
                return;
            }
            if (*start == *end) {
                gain = Parameter(*start);
            } else {
                const Range range(std::min(*start, *end), std::max(*start, *end));
                gain = Parameter("k", range, range.middle());
            }
        }
    } catch (const qftbx::Exception & e) {
        //A value that parses but is not a number a model can use: "0/0" and
        //"1/0" evaluate to a NaN and an infinity, and Parameter refuses
        //those.
        errorMessage(translated(e), tr("Controller input"));
        return;
    }

    //A second accept replaces the answer of the first one; whoever took it
    //already owns that one.
    controllerSystem.reset();

    std::vector<Parameter> numerator;
    std::vector<Parameter> denominator;

    //The uncertainty only counts if its dialog was ACCEPTED; the controller
    //receives COPIES, the dialog keeps its own for further editing.
    if ((uncertaintyEntered || coefficientsFromProject) && uncertaintyDialog->wasAccepted()) {
        numerator = uncertaintyDialog->numerator();
        denominator = uncertaintyDialog->denominator();
    } else {
        std::optional<std::vector<Parameter>> readNumerator = m_reader.buildParameters(valueTable->at(0));
        std::optional<std::vector<Parameter>> readDenominator = m_reader.buildParameters(valueTable->at(1));

        if (!readNumerator.has_value() || !readDenominator.has_value()) {
            errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
            return;
        }
        numerator = std::move(*readNumerator);
        denominator = std::move(*readDenominator);
    }

    controllerSystem = SystemDescriptionReader::makeSystem(selectedType(), "",
                                                           std::move(numerator), std::move(denominator),
                                                           std::move(*gain), Parameter(0.0),
                                                           ui->numeratorEdit->text().toStdString(),
                                                           ui->denominatorEdit->text().toStdString());

    markAccepted();
}

std::unique_ptr<LtiSystem> ControllerDialog::takeControllerStructure()
{
    return std::move(controllerSystem);
}

} // namespace qftbx
