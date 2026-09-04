#include "src/gui/plant/plant_dialog.h"
#include "ui_plant_dialog.h"

#include "src/core/common/exception.h"
#include "src/gui/application/error_message.h"

namespace qftbx {

PlantDialog::PlantDialog(QWidget *parent) :
    StepDialog(parent),
    ui(std::make_unique<Ui::PlantDialog>()),
    m_reader(tr("Plant input"))
{
    ui->setupUi(this);
    setWindowTitle(tr("Plant input"));

    //Hide the buttons that do not apply yet.
    ui->zerosPolesRadio->setVisible(false);
    ui->polynomialRadio->setVisible(false);
    ui->zpkRadio->setVisible(false);
    ui->tcgRadio->setVisible(false);

    ui->polyGain->setText("1");
    ui->polyDelay->setText("0");
    ui->zpkGain->setText("1");
    ui->zpkDelay->setText("0");
    ui->tcgGain->setText("1");
    ui->tcgDelay->setText("0");
    ui->freeGain->setText("1");
    ui->freeDelay->setText("0");

    //The uncertainty dialog is created up front and reused.
    uncertaintyDialog = new UncertaintyDialog (this);

    ui->zpkImage->setPixmap(QPixmap(":/figures/kgan.png"));
    ui->tcgImage->setPixmap(QPixmap(":/figures/knogan.png"));
    ui->polyImage->setPixmap(QPixmap(":/figures/copol.png"));

    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
}

PlantDialog::~PlantDialog()
{
    //The uncertainty dialog is a Qt child of this one, so Qt frees it.
}

void PlantDialog::on_zerosPolesRadio_toggled(bool checked)
{
    ui->zpkRadio->setVisible(checked);
    ui->tcgRadio->setVisible(checked);
    ui->zerosPolesRadio->setVisible(checked);
    ui->polynomialRadio->setVisible(checked);

    if (checked)
        ui->formStack->setCurrentIndex(0);
}

void PlantDialog::on_transferFunctionRadio_toggled(bool checked)
{
    ui->zerosPolesRadio->setVisible(checked);
    ui->polynomialRadio->setVisible(checked);

    if (checked)
        ui->formStack->setCurrentIndex(0);
}

void PlantDialog::on_zpkRadio_toggled(bool checked)
{
    ui->zerosPolesRadio->setVisible(checked);
    ui->polynomialRadio->setVisible(checked);
    ui->zpkRadio->setVisible(checked);
    ui->tcgRadio->setVisible(checked);

    if (checked)
        ui->formStack->setCurrentIndex(3);
}

void PlantDialog::on_tcgRadio_toggled(bool checked)
{
    ui->zerosPolesRadio->setVisible(checked);
    ui->polynomialRadio->setVisible(checked);
    ui->zpkRadio->setVisible(checked);
    ui->tcgRadio->setVisible(checked);

    if (checked)
        ui->formStack->setCurrentIndex(4);
}

void PlantDialog::on_polynomialRadio_toggled(bool checked)
{
    ui->zerosPolesRadio->setVisible(checked);
    ui->polynomialRadio->setVisible(checked);

    if (checked)
        ui->formStack->setCurrentIndex(2);
}

void PlantDialog::on_freeFormRadio_clicked()
{
    ui->formStack->setCurrentIndex(1);
}

LtiSystem::SystemType PlantDialog::selectedType() const
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

std::optional<CoefficientTable> PlantDialog::readTables(CoefficientTable & expressionTable,
                                                        UncertainTable & uncertainTable)
{
    //Rows in the order the uncertainty dialog expects: numerator,
    //denominator, gain, delay. && on the RIGHT so every field is read and
    //every problem is reported.
    CoefficientTable tables;
    bool valid = true;

    switch (selectedType()) {
    case LtiSystem::SystemType::PolynomialForm:
        valid = m_reader.readCoefficients(ui->polyNumerator->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readCoefficients(ui->polyDenominator->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readScalar(ui->polyGain->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readScalar(ui->polyDelay->text(), tables, expressionTable, uncertainTable) && valid;
        break;
    case LtiSystem::SystemType::ZeroPoleGain:
        valid = m_reader.readCoefficients(ui->zpkNumerator->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readCoefficients(ui->zpkDenominator->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readScalar(ui->zpkGain->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readScalar(ui->zpkDelay->text(), tables, expressionTable, uncertainTable) && valid;
        break;
    case LtiSystem::SystemType::TimeConstantGain:
        valid = m_reader.readCoefficients(ui->tcgNumerator->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readCoefficients(ui->tcgDenominator->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readScalar(ui->tcgGain->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readScalar(ui->tcgDelay->text(), tables, expressionTable, uncertainTable) && valid;
        break;
    case LtiSystem::SystemType::FreeForm:
        if (!ui->freeFormRadio->isChecked()) {
            //No family chosen yet.
            return std::nullopt;
        }
        valid = m_reader.readFreeForm(ui->freeNumerator->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readFreeForm(ui->freeDenominator->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readScalar(ui->freeGain->text(), tables, expressionTable, uncertainTable) && valid;
        valid = m_reader.readScalar(ui->freeDelay->text(), tables, expressionTable, uncertainTable) && valid;
        break;
    }

    if (!valid) {
        return std::nullopt;
    }

    return tables;
}

bool PlantDialog::nameIsPresent()
{
    //The name is required on both paths: only the uncertainty path used to
    //validate it, and nameless plants could be saved.
    if (ui->nameEdit->text().isEmpty()) {
        errorMessage(tr("The plant name is missing."), tr("Plant input"));
        ui->nameEdit->setStyleSheet("background : red");
        return false;
    }
    ui->nameEdit->setStyleSheet("background : white");
    return true;
}

void PlantDialog::on_okButton_clicked()
{
    if (!nameIsPresent()) {
        return;
    }

    CoefficientTable expressionTable;
    UncertainTable uncertainTable;
    const std::optional<CoefficientTable> valueTable = readTables(expressionTable, uncertainTable);

    if (!valueTable.has_value()) {
        errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
        return;
    }

    //Gain and delay: a constant when the field holds a plain value, a
    //parameter over the range the uncertainty dialog gave when it does not.
    const auto scalar = [&](std::size_t row, double fallback, const Range & range,
                            const char * name) -> std::optional<Parameter> {
        if (valueTable->at(row).empty()) {
            return Parameter(fallback);
        }
        const std::optional<double> value = m_reader.evaluate(expressionTable.at(row).at(0));
        if (!value.has_value()) {
            return std::nullopt;
        }
        if (*value == range.min && *value == range.max) {
            return Parameter(*value);
        }
        return Parameter(name, range, *value, name);
    };

    std::optional<Parameter> gain;
    std::optional<Parameter> delay;
    try {
        gain = scalar(2, 1.0, uncertaintyDialog->gain(), "kv");
        delay = scalar(3, 0.0, uncertaintyDialog->delay(), "ret");
    } catch (mup::ParserError &) {
        //The ranges come from the uncertainty dialog's own fields.
        errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
        return;
    } catch (const qftbx::Exception & e) {
        //A value that parses but is not a number a model can use: muParserX
        //answers "0/0" and "1/0" with a NaN and an infinity, and Parameter
        //refuses those.
        errorMessage(e.what(), tr("Plant input"));
        return;
    }

    if (!gain.has_value() || !delay.has_value()) {
        errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
        return;
    }

    //A second accept replaces the answer of the first one; whoever took it
    //already owns that one.
    plant.reset();

    std::vector<Parameter> numerator;
    std::vector<Parameter> denominator;

    //The uncertainty only counts if its dialog was ACCEPTED; the plant
    //receives COPIES, the dialog keeps its own for further editing.
    if (uncertaintyEntered && uncertaintyDialog->wasAccepted()) {
        numerator = uncertaintyDialog->numerator();
        denominator = uncertaintyDialog->denominator();
    } else {
        std::optional<std::vector<Parameter>> readNumerator = m_reader.buildParameters(valueTable->at(0));
        std::optional<std::vector<Parameter>> readDenominator = m_reader.buildParameters(valueTable->at(1));

        if (!readNumerator.has_value() || !readDenominator.has_value()) {
            errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
            return;
        }
        numerator = std::move(*readNumerator);
        denominator = std::move(*readDenominator);
    }

    plant = SystemDescriptionReader::makeSystem(selectedType(), ui->nameEdit->text().toStdString(),
                                                std::move(numerator), std::move(denominator),
                                                std::move(*gain), std::move(*delay),
                                                ui->freeNumerator->text().toStdString(),
                                                ui->freeDenominator->text().toStdString());

    markAccepted();
    close();
}

void PlantDialog::on_uncertaintyButton_clicked()
{
    CoefficientTable expressionTable;
    UncertainTable uncertainTable;
    std::optional<CoefficientTable> valueTable = readTables(expressionTable, uncertainTable);

    if (!valueTable.has_value()) {
        errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
        return;
    }

    if (!nameIsPresent()) {
        return;
    }

    uncertaintyDialog->launch(std::move(*valueTable), std::move(expressionTable),
                              std::move(uncertainTable), false);
    uncertaintyDialog->show();
    uncertaintyEntered = true;
}

std::unique_ptr<LtiSystem> PlantDialog::takePlant()
{
    return std::move(plant);
}

} // namespace qftbx
