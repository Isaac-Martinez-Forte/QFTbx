#include "src/gui/plant/plant_form.h"
#include "ui_plant_form.h"

#include <QPixmap>
#include <QRadioButton>

#include "src/core/common/exception.h"
#include "src/core/math/expression_tree.h"
#include "src/core/system/system_formula.h"
#include "src/gui/application/error_message.h"
#include "src/gui/common/field_mark.h"
#include "src/gui/common/number_text.h"
#include "src/gui/common/system_description_writer.h"

namespace qftbx {

namespace {

//The figure of each family, which is what the right-hand side shows until
//there is a formula to show instead.
const char * figureOf(LtiSystem::SystemType type)
{
    switch (type) {
    case LtiSystem::SystemType::ZeroPoleGain:
        return ":/figures/kgan.png";
    case LtiSystem::SystemType::TimeConstantGain:
        return ":/figures/knogan.png";
    case LtiSystem::SystemType::PolynomialForm:
        return ":/figures/copol.png";
    case LtiSystem::SystemType::FreeForm:
        break;
    }

    return nullptr;
}

} // namespace

PlantForm::PlantForm(QWidget * parent) :
    StepPanel(parent),
    ui(std::make_unique<Ui::PlantForm>()),
    m_reader(tr("Plant"))
{
    ui->setupUi(this);
    setWindowTitle(tr("Plant"));

    ui->gainEdit->setText("1");
    ui->delayEdit->setText("0");

    //The uncertainty is a page of this form and not a window on top of it.
    m_uncertainty = new UncertaintyPanel(this);
    m_uncertainty->setTitle(tr("Uncertainty of the plant"));
    ui->uncertaintyLayout->addWidget(m_uncertainty);

    connect(m_uncertainty, &UncertaintyPanel::applied, this, [this] {
        m_uncertaintyCoefficients = currentCoefficients();
        ui->pageStack->setCurrentWidget(ui->dataPage);
        //The ranges are part of the plant: what was verified before them is
        //not what the form now describes.
        setVerified(nullptr);
    });
    connect(m_uncertainty, &UncertaintyPanel::cancelled, this, [this] {
        ui->pageStack->setCurrentWidget(ui->dataPage);
    });

    for (QRadioButton * radio : findChildren<QRadioButton *>()) {
        connect(radio, &QRadioButton::toggled, this, &PlantForm::familyChosen);
    }

    for (QLineEdit * field : {ui->nameEdit, ui->descriptionEdit, ui->numeratorEdit,
                              ui->denominatorEdit, ui->gainEdit, ui->delayEdit}) {
        //textChanged and not textEdited: a field written by anything other
        //than the keyboard - a paste, an undo, a clear - changes the plant
        //just the same. What the form writes into its own fields is fenced
        //off by m_filling instead.
        connect(field, &QLineEdit::textChanged, this, &PlantForm::fieldEdited);
    }

    showFamily();
}

PlantForm::~PlantForm()
{
    //The uncertainty panel is a Qt child of this form, so Qt frees it.
}

std::optional<LtiSystem::SystemType> PlantForm::selectedType() const
{
    if (ui->freeFormRadio->isChecked()) {
        return LtiSystem::SystemType::FreeForm;
    }
    if (ui->polynomialRadio->isChecked()) {
        return LtiSystem::SystemType::PolynomialForm;
    }
    if (ui->zpkRadio->isChecked()) {
        return LtiSystem::SystemType::ZeroPoleGain;
    }
    if (ui->tcgRadio->isChecked()) {
        return LtiSystem::SystemType::TimeConstantGain;
    }

    //The path is unfinished: a transfer function, and nothing said yet
    //about how it is written.
    return std::nullopt;
}

void PlantForm::showFamily()
{
    //One level at a time, and every level of the path stays marked: the
    //three are separate button groups for exactly that reason.
    const bool transfer = ui->transferFunctionRadio->isChecked();
    const bool factored = transfer && ui->zerosPolesRadio->isChecked();

    for (QWidget * widget : {static_cast<QWidget *>(ui->zerosPolesRadio),
                             static_cast<QWidget *>(ui->polynomialRadio),
                             static_cast<QWidget *>(ui->firstLine)}) {
        widget->setVisible(transfer);
    }
    for (QWidget * widget : {static_cast<QWidget *>(ui->zpkRadio),
                             static_cast<QWidget *>(ui->tcgRadio),
                             static_cast<QWidget *>(ui->secondLine)}) {
        widget->setVisible(factored);
    }

    const std::optional<LtiSystem::SystemType> type = selectedType();

    ui->numeratorEdit->setEnabled(type.has_value());
    ui->denominatorEdit->setEnabled(type.has_value());

    if (!type.has_value()) {
        ui->hintLabel->setText(tr("Choose how the plant is written."));
        ui->familyImage->setPixmap(QPixmap());
        ui->familyImage->setText(QString());
        return;
    }

    const bool factoredFamily = *type == LtiSystem::SystemType::ZeroPoleGain
            || *type == LtiSystem::SystemType::TimeConstantGain;

    ui->numeratorLabel->setText(factoredFamily ? tr("Zeros:") : tr("Numerator:"));
    ui->denominatorLabel->setText(factoredFamily ? tr("Poles:") : tr("Denominator:"));

    switch (*type) {
    case LtiSystem::SystemType::ZeroPoleGain:
        ui->hintLabel->setText(tr("One zero and one pole per value, separated by spaces: "
                                  "the factors are (s + a). A name instead of a number "
                                  "makes that root uncertain."));
        break;
    case LtiSystem::SystemType::TimeConstantGain:
        ui->hintLabel->setText(tr("One time constant per value, separated by spaces: "
                                  "the factors are (1 + s/T). A name instead of a number "
                                  "makes that constant uncertain."));
        break;
    case LtiSystem::SystemType::PolynomialForm:
        ui->hintLabel->setText(tr("The coefficients by descending power, separated by "
                                  "spaces. A name instead of a number makes that "
                                  "coefficient uncertain."));
        break;
    case LtiSystem::SystemType::FreeForm:
        ui->hintLabel->setText(tr("An expression in s. Every other name in it is an "
                                  "uncertain parameter."));
        break;
    }

    const char * figure = figureOf(*type);
    ui->familyImage->setPixmap(figure != nullptr ? QPixmap(figure) : QPixmap());
    ui->familyImage->setText(figure != nullptr ? QString()
                                               : tr("The formula appears here once verified."));
}

void PlantForm::familyChosen()
{
    showFamily();

    if (m_filling) {
        return;
    }


    //A family changed is a plant changed: back to the figure, and back to
    //having to verify.
    setVerified(nullptr);
}

void PlantForm::fieldEdited()
{
    if (m_filling) {
        return;
    }

    //What was verified described the fields as they were.
    setVerified(nullptr);

    //And the mistakes are marked as they are made, not saved up for the
    //button.
    CoefficientTable expressions;
    UncertainTable uncertain;
    readTables(expressions, uncertain);
}

void PlantForm::say(const QString & complaint)
{
    ui->statusLabel->setText(complaint);
    markWrong(ui->statusLabel, !complaint.isEmpty());
}

void PlantForm::setVerified(std::unique_ptr<LtiSystem> plant)
{
    m_verified = std::move(plant);

    if (m_verified == nullptr) {
        ui->figureStack->setCurrentWidget(ui->figurePage);
        ui->okButton->setText(tr("Verify"));
        return;
    }

    //The plant as it was understood, where the figure of the family was:
    //the one thing a line of coefficients cannot be checked against.
    ui->formulaView->setFormula(formulaOf(*m_verified, shownDigits()));

    //And under it the same plant with every uncertain coefficient replaced
    //by the interval it stands for: the plant as described, and the family
    //of plants the design will actually work over. A plant with nothing
    //uncertain would say the same thing twice, so it says it once.
    const bool uncertain = hasUncertainty(*m_verified);
    ui->rangeFormula->setVisible(uncertain);
    ui->formulaRule->setVisible(uncertain);
    if (uncertain) {
        ui->rangeFormula->setFormula(formulaOf(*m_verified, shownDigits(),
                                               ShowUncertain::ByRange));
    }

    ui->figureStack->setCurrentWidget(ui->formulaPage);
    ui->okButton->setText(tr("Apply"));
}

std::optional<CoefficientTable> PlantForm::readTables(CoefficientTable & expressionTable,
                                                      UncertainTable & uncertainTable)
{
    const std::optional<LtiSystem::SystemType> type = selectedType();

    markWrong(ui->nameEdit, false);
    markWrong(ui->numeratorEdit, false);
    markWrong(ui->denominatorEdit, false);
    markWrong(ui->gainEdit, false);
    markWrong(ui->delayEdit, false);

    if (!type.has_value()) {
        say(tr("Choose how the plant is written."));
        return std::nullopt;
    }

    CoefficientTable tables;
    bool valid = true;

    //Rows in the order the uncertainty panel expects: numerator,
    //denominator, gain, delay. Every field is read even after one fails, so
    //that every problem is marked at once.
    //A family written as factors has no factors when its field is empty;
    //only a polynomial reads an empty field as the constant 1.
    const bool factored = *type == LtiSystem::SystemType::ZeroPoleGain
            || *type == LtiSystem::SystemType::TimeConstantGain;

    const auto readPolynomial = [&](QLineEdit * field) {
        const bool read = *type == LtiSystem::SystemType::FreeForm
                ? m_reader.readFreeForm(field->text(), tables, expressionTable, uncertainTable)
                : m_reader.readCoefficients(field->text(), tables, expressionTable,
                                            uncertainTable, !factored);
        if (!read) {
            markWrong(field, true, m_reader.complaint());
            say(m_reader.complaint());
            valid = false;
        }
        return read;
    };

    const bool numeratorRead = readPolynomial(ui->numeratorEdit);
    const bool denominatorRead = readPolynomial(ui->denominatorEdit);

    const auto readScalar = [&](QLineEdit * field, const QString & what) {
        if (!m_reader.readScalar(field->text(), tables, expressionTable, uncertainTable)) {
            markWrong(field, true, m_reader.complaint());
            say(m_reader.complaint());
            valid = false;
            return;
        }
        //The gain and the delay are the two fields that hold a VALUE and
        //never a name: what makes them uncertain is the range given under
        //Uncertainty, and a name typed here would be a parameter nothing
        //asks the range of.
        const QString text = field->text().trimmed();
        if (uncertainTable.back().at(0)) {
            const QString complaint = tr("%1 is a value; its range is given under "
                                         "Uncertainty.").arg(what);
            markWrong(field, true, complaint);
            say(complaint);
            valid = false;
            return;
        }
        if (!text.isEmpty() && !m_reader.evaluate(text).has_value()) {
            const QString complaint = tr("%1 is not a number.").arg(what);
            markWrong(field, true, complaint);
            say(complaint);
            valid = false;
        }
    };

    readScalar(ui->gainEdit, tr("The gain"));
    readScalar(ui->delayEdit, tr("The delay"));

    //A free-form expression that the grammar cannot read is not an
    //expression, and the field that holds it is the one that is wrong.
    if (*type == LtiSystem::SystemType::FreeForm) {
        const auto readable = [&](QLineEdit * field) {
            if (field->text().trimmed().isEmpty()) {
                return true;
            }
            try {
                ExpressionTree parsed(field->text().toStdString());
                return true;
            } catch (const std::invalid_argument &) {
                const QString complaint = tr("This is not an expression the toolbox can read.");
                markWrong(field, true, complaint);
                say(complaint);
                return false;
            }
        };

        valid = readable(ui->numeratorEdit) && valid;
        valid = readable(ui->denominatorEdit) && valid;
    }

    if (!valid || !numeratorRead || !denominatorRead) {
        return std::nullopt;
    }

    say(QString());

    return tables;
}

bool PlantForm::uncertaintyIsCurrent() const
{
    //Either the panel was applied over these very coefficients, or they are
    //still the ones the project's plant was written back as, and then the
    //plant's own parameters answer for them.
    if (!m_uncertaintyCoefficients.isEmpty()
            && m_uncertaintyCoefficients == currentCoefficients()) {
        return true;
    }

    return !m_describedCoefficients.isEmpty()
            && m_describedCoefficients == currentCoefficients();
}

std::unique_ptr<LtiSystem> PlantForm::build()
{
    CoefficientTable expressionTable;
    UncertainTable uncertainTable;
    const std::optional<CoefficientTable> valueTable = readTables(expressionTable, uncertainTable);

    if (!valueTable.has_value()) {
        return nullptr;
    }

    //Asked for HERE and not while the fields are being read, which happens
    //at every keystroke: a plant is named when it is finished, and being
    //told it has no name while typing its numerator helps nobody.
    if (ui->nameEdit->text().trimmed().isEmpty()) {
        markWrong(ui->nameEdit, true, tr("The plant needs a name."));
        say(tr("The plant needs a name."));
        return nullptr;
    }

    //A coefficient that was given a name needs a range, and there is one
    //place to give it.
    const bool hasUncertainty = std::any_of(uncertainTable.begin(), uncertainTable.begin() + 2,
                                            [](const UncertainRow & row) {
                                                return std::find(row.begin(), row.end(), true)
                                                        != row.end();
                                            });

    if (hasUncertainty && !uncertaintyIsCurrent()) {
        say(tr("The coefficients that were given a name need a range: open Uncertainty."));
        markWrong(ui->uncertaintyButton, true,
                  tr("The coefficients that were given a name need a range."));
        return nullptr;
    }
    markWrong(ui->uncertaintyButton, false);

    //Gain and delay: a constant when the field holds a plain value, a
    //parameter over the range the uncertainty panel gave when it does not.
    const auto scalar = [&](std::size_t row, double fallback, const Range & range,
                            const char * name) -> std::optional<Parameter> {
        if (valueTable->at(row).empty()) {
            return Parameter(fallback);
        }
        const std::optional<double> value = m_reader.evaluate(expressionTable.at(row).at(0));
        if (!value.has_value()) {
            return std::nullopt;
        }
        //Uncertain only when a RANGE was given for it. The rule used to be
        //that the value had to match both ends, so a gain of 3 typed into a
        //plant whose uncertainty was never opened became a parameter over
        //[1, 1] with a nominal of 3 - swept at 1, drawn at 3.
        if (range.min == range.max) {
            return Parameter(*value);
        }
        return Parameter(name, range, *value, name);
    };

    //A form filled from the project answers from the plant's own parameters
    //while its fields still describe them - the first edit and they do not.
    //Field by field: editing a coefficient says nothing about the gain.
    const bool coefficientsFromProject = !m_describedCoefficients.isEmpty()
            && currentCoefficients() == m_describedCoefficients;
    const bool scalarsFromProject = !m_describedScalars.isEmpty()
            && currentScalars() == m_describedScalars;

    std::optional<Parameter> gain;
    std::optional<Parameter> delay;
    try {
        //The gain is "k" and the delay "delay": the plant has no field for
        //their names. "kv" and "ret" were the names of the days when the
        //expression parser owned the letter k as a unit multiplier.
        if (scalarsFromProject) {
            gain = m_projectGain;
            delay = m_projectDelay;
        } else {
            gain = scalar(2, 1.0, m_uncertainty->gain(), "k");
            delay = scalar(3, 0.0, m_uncertainty->delay(), "delay");
        }
    } catch (const qftbx::Exception & e) {
        //A value that parses but is not a number a model can use ("0/0" and
        //"1/0" evaluate to a NaN and an infinity, and Parameter refuses
        //those), or a range field that is not an expression.
        say(translated(e));
        return nullptr;
    }

    if (!gain.has_value()) {
        markWrong(ui->gainEdit, true, tr("The gain is not a number."));
        say(tr("The gain is not a number."));
        return nullptr;
    }
    if (!delay.has_value()) {
        markWrong(ui->delayEdit, true, tr("The delay is not a number."));
        say(tr("The delay is not a number."));
        return nullptr;
    }

    std::vector<Parameter> numerator;
    std::vector<Parameter> denominator;

    //The uncertainty only counts if its panel was APPLIED; the plant
    //receives COPIES, the panel keeps its own for further editing.
    if ((uncertaintyIsCurrent() || coefficientsFromProject) && m_uncertainty->wasAccepted()) {
        numerator = m_uncertainty->numerator();
        denominator = m_uncertainty->denominator();
    } else {
        std::optional<std::vector<Parameter>> readNumerator =
                m_reader.buildParameters(valueTable->at(0));
        std::optional<std::vector<Parameter>> readDenominator =
                m_reader.buildParameters(valueTable->at(1));

        if (!readNumerator.has_value()) {
            markWrong(ui->numeratorEdit, true, tr("A coefficient is not a number."));
            say(tr("A coefficient is not a number."));
            return nullptr;
        }
        if (!readDenominator.has_value()) {
            markWrong(ui->denominatorEdit, true, tr("A coefficient is not a number."));
            say(tr("A coefficient is not a number."));
            return nullptr;
        }
        numerator = std::move(*readNumerator);
        denominator = std::move(*readDenominator);
    }

    std::unique_ptr<LtiSystem> plant =
            SystemDescriptionReader::makeSystem(*selectedType(),
                                                ui->nameEdit->text().trimmed().toStdString(),
                                                std::move(numerator), std::move(denominator),
                                                std::move(*gain), std::move(*delay),
                                                ui->numeratorEdit->text().toStdString(),
                                                ui->denominatorEdit->text().toStdString());

    plant->setDescription(ui->descriptionEdit->text().trimmed().toStdString());

    return plant;
}

void PlantForm::on_okButton_clicked()
{
    //One button and two steps: nothing is applied that the user has not
    //been shown first.
    if (m_verified == nullptr) {
        setVerified(build());
        return;
    }

    m_applied = std::move(m_verified);
    setVerified(m_applied->clone());

    markAccepted();
}

void PlantForm::on_uncertaintyButton_clicked()
{
    CoefficientTable expressionTable;
    UncertainTable uncertainTable;
    std::optional<CoefficientTable> valueTable = readTables(expressionTable, uncertainTable);

    if (!valueTable.has_value()) {
        return;
    }

    m_uncertainty->launch(std::move(*valueTable), std::move(expressionTable),
                          std::move(uncertainTable), false);
    ui->pageStack->setCurrentWidget(ui->uncertaintyPage);
}

QString PlantForm::currentCoefficients() const
{
    return ui->numeratorEdit->text() + QLatin1Char('\n') + ui->denominatorEdit->text();
}

QString PlantForm::currentScalars() const
{
    return ui->gainEdit->text() + QLatin1Char('\n') + ui->delayEdit->text();
}

void PlantForm::setFromProject(LtiSystem * plant)
{
    if (plant == nullptr) {
        return;
    }

    const SystemDescription described = describeSystem(*plant);

    m_filling = true;

    ui->nameEdit->setText(described.name);
    ui->descriptionEdit->setText(QString::fromStdString(plant->description()));

    //The whole path, not only its last step: a plant written as zeros and
    //poles is a transfer function first.
    switch (described.type) {
    case LtiSystem::SystemType::PolynomialForm:
        ui->transferFunctionRadio->setChecked(true);
        ui->polynomialRadio->setChecked(true);
        break;
    case LtiSystem::SystemType::ZeroPoleGain:
        ui->transferFunctionRadio->setChecked(true);
        ui->zerosPolesRadio->setChecked(true);
        ui->zpkRadio->setChecked(true);
        break;
    case LtiSystem::SystemType::TimeConstantGain:
        ui->transferFunctionRadio->setChecked(true);
        ui->zerosPolesRadio->setChecked(true);
        ui->tcgRadio->setChecked(true);
        break;
    case LtiSystem::SystemType::FreeForm:
        ui->freeFormRadio->setChecked(true);
        break;
    }

    ui->numeratorEdit->setText(described.numerator);
    ui->denominatorEdit->setText(described.denominator);
    ui->gainEdit->setText(described.gain);
    ui->delayEdit->setText(described.delay);

    m_filling = false;
    showFamily();

    //The intervals travel with the plant, so the uncertainty does not have
    //to be typed again over a project that was just opened.
    m_uncertainty->setParameters(plant->numerator(), plant->denominator(),
                                 plant->gain().rawRange(), plant->delay().rawRange());

    m_projectGain = plant->gain();
    m_projectDelay = plant->delay();
    m_describedCoefficients = currentCoefficients();
    m_describedScalars = currentScalars();
    m_uncertaintyCoefficients = m_describedCoefficients;

    //A plant that came from a file is a plant already verified: it is shown
    //as what it is, and the button offers to apply it.
    setVerified(plant->clone());
    say(QString());
}

std::unique_ptr<LtiSystem> PlantForm::takePlant()
{
    return std::move(m_applied);
}

} // namespace qftbx
