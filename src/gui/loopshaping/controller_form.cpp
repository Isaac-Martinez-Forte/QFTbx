/**
 * @file
 * @brief Reads the controller structure and its freedom from the form.
 *
 * Every field is read even after one fails so that every problem is marked
 * at once. The gain is a constant when both ends agree and the parameter
 * `k` over them otherwise, in either order; its ends are numbers and never
 * names. A named coefficient needs a range, and the range only counts when
 * the freedom panel was applied over the coefficients now on screen, in
 * range-only mode since a structure has no nominal value of its own. A
 * form filled from the project answers from the structure's own
 * parameters while its fields still describe them, coefficients and gain
 * separately. The structure receives copies; the panel keeps its own for
 * further editing.
 */

#include "src/gui/loopshaping/controller_form.h"
#include "ui_controller_form.h"

#include <algorithm>

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

}

ControllerForm::ControllerForm(QWidget * parent) :
    StepPanel(parent),
    ui(std::make_unique<Ui::ControllerForm>()),
    m_reader(tr("Controller structure"))
{
    ui->setupUi(this);
    setWindowTitle(tr("Controller structure"));

    ui->gainStart->setText("1");
    ui->gainEnd->setText("1");

    m_freedom = new UncertaintyPanel(this);
    m_freedom->setTitle(tr("Search range of every parameter of the structure"));
    ui->uncertaintyLayout->addWidget(m_freedom);

    connect(m_freedom, &UncertaintyPanel::applied, this, [this] {
        m_freedomCoefficients = currentCoefficients();
        ui->pageStack->setCurrentWidget(ui->dataPage);
        setVerified(nullptr);
    });
    connect(m_freedom, &UncertaintyPanel::cancelled, this, [this] {
        ui->pageStack->setCurrentWidget(ui->dataPage);
    });

    for (QRadioButton * radio : findChildren<QRadioButton *>()) {
        connect(radio, &QRadioButton::toggled, this, &ControllerForm::familyChosen);
    }

    for (QLineEdit * field : {ui->numeratorEdit, ui->denominatorEdit,
                              ui->gainStart, ui->gainEnd}) {
        connect(field, &QLineEdit::textChanged, this, &ControllerForm::fieldEdited);
    }

    showFamily();
}

ControllerForm::~ControllerForm()
{
}

std::optional<LtiSystem::SystemType> ControllerForm::selectedType() const
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

    return std::nullopt;
}

void ControllerForm::showFamily()
{
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

    const std::optional<LtiSystem::SystemType> chosen = selectedType();

    ui->numeratorEdit->setEnabled(chosen.has_value());
    ui->denominatorEdit->setEnabled(chosen.has_value());

    if (!chosen.has_value()) {
        ui->hintLabel->setText(tr("Choose how the structure is written."));
        ui->familyImage->setPixmap(QPixmap());
        ui->familyImage->setText(QString());
        return;
    }

    const LtiSystem::SystemType type = *chosen;

    const bool factoredFamily = type == LtiSystem::SystemType::ZeroPoleGain
            || type == LtiSystem::SystemType::TimeConstantGain;

    ui->numeratorLabel->setText(factoredFamily ? tr("Zeros:") : tr("Numerator:"));
    ui->denominatorLabel->setText(factoredFamily ? tr("Poles:") : tr("Denominator:"));

    switch (type) {
    case LtiSystem::SystemType::ZeroPoleGain:
    case LtiSystem::SystemType::TimeConstantGain:
        ui->hintLabel->setText(tr("One name per zero and per pole: each of them is searched "
                                  "for over the range given under Controller freedom."));
        break;
    case LtiSystem::SystemType::PolynomialForm:
        ui->hintLabel->setText(tr("The coefficients by descending power. A name is searched "
                                  "for over the range given under Controller freedom."));
        break;
    case LtiSystem::SystemType::FreeForm:
        ui->hintLabel->setText(tr("An expression in s. Every other name in it is searched "
                                  "for over its range."));
        break;
    }

    const char * figure = figureOf(type);
    ui->familyImage->setPixmap(figure != nullptr ? QPixmap(figure) : QPixmap());
    ui->familyImage->setText(figure != nullptr ? QString()
                                               : tr("The structure appears here once verified."));
}

void ControllerForm::familyChosen()
{
    showFamily();

    if (m_filling) {
        return;
    }

    setVerified(nullptr);
}

void ControllerForm::fieldEdited()
{
    if (m_filling) {
        return;
    }

    setVerified(nullptr);

    CoefficientTable expressions;
    UncertainTable uncertain;
    readTables(expressions, uncertain);
}

void ControllerForm::say(const QString & complaint)
{
    ui->statusLabel->setText(complaint);
    markWrong(ui->statusLabel, !complaint.isEmpty());
}

void ControllerForm::setVerified(std::unique_ptr<LtiSystem> structure)
{
    m_verified = std::move(structure);

    if (m_verified == nullptr) {
        ui->figureStack->setCurrentWidget(ui->figurePage);
        ui->okButton->setText(tr("Verify"));
        return;
    }

    ui->formulaView->setFormula(formulaOf(*m_verified, shownDigits()));

    const bool free = hasUncertainty(*m_verified);
    ui->rangeFormula->setVisible(free);
    ui->formulaRule->setVisible(free);
    if (free) {
        ui->rangeFormula->setFormula(formulaOf(*m_verified, shownDigits(),
                                               ShowUncertain::ByRange));
    }

    ui->figureStack->setCurrentWidget(ui->formulaPage);
    ui->okButton->setText(tr("Apply"));
}

std::optional<CoefficientTable> ControllerForm::readTables(CoefficientTable & expressionTable,
                                                          UncertainTable & uncertainTable)
{
    markWrong(ui->numeratorEdit, false);
    markWrong(ui->denominatorEdit, false);
    markWrong(ui->gainStart, false);
    markWrong(ui->gainEnd, false);

    const std::optional<LtiSystem::SystemType> chosen = selectedType();

    if (!chosen.has_value()) {
        say(tr("Choose how the structure is written."));
        return std::nullopt;
    }

    const LtiSystem::SystemType type = *chosen;

    const bool factored = type == LtiSystem::SystemType::ZeroPoleGain
            || type == LtiSystem::SystemType::TimeConstantGain;

    CoefficientTable tables;
    bool valid = true;

    const auto readPolynomial = [&](QLineEdit * field) {
        const bool read = type == LtiSystem::SystemType::FreeForm
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

    m_reader.readGainRange(ui->gainStart->text(), ui->gainEnd->text(),
                           tables, expressionTable, uncertainTable);

    for (QLineEdit * field : {ui->gainStart, ui->gainEnd}) {
        if (!m_reader.evaluate(field->text()).has_value()) {
            const QString complaint = tr("The ends of the gain range are numbers.");
            markWrong(field, true, complaint);
            say(complaint);
            valid = false;
        }
    }

    if (type == LtiSystem::SystemType::FreeForm) {
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

bool ControllerForm::freedomIsCurrent() const
{
    if (!m_freedomCoefficients.isEmpty() && m_freedomCoefficients == currentCoefficients()) {
        return true;
    }

    return !m_describedCoefficients.isEmpty()
            && m_describedCoefficients == currentCoefficients();
}

std::unique_ptr<LtiSystem> ControllerForm::build()
{
    CoefficientTable expressionTable;
    UncertainTable uncertainTable;
    const std::optional<CoefficientTable> valueTable = readTables(expressionTable, uncertainTable);

    if (!valueTable.has_value()) {
        return nullptr;
    }

    const bool hasFreedom = std::any_of(uncertainTable.begin(), uncertainTable.begin() + 2,
                                        [](const UncertainRow & row) {
                                            return std::find(row.begin(), row.end(), true)
                                                    != row.end();
                                        });

    if (hasFreedom && !freedomIsCurrent()) {
        say(tr("The parameters of the structure need a range: open Controller freedom."));
        markWrong(ui->uncertaintyButton, true,
                  tr("The parameters of the structure need a range."));
        return nullptr;
    }
    markWrong(ui->uncertaintyButton, false);

    const bool coefficientsFromProject = !m_describedCoefficients.isEmpty()
            && currentCoefficients() == m_describedCoefficients;
    const bool gainFromProject = !m_describedGain.isEmpty() && currentGain() == m_describedGain;

    std::optional<Parameter> gain;
    try {
        if (gainFromProject) {
            gain = m_projectGain;
        } else {
            const std::optional<double> start = m_reader.evaluate(expressionTable.at(2).at(0));
            const std::optional<double> end = m_reader.evaluate(expressionTable.at(2).at(1));
            if (!start.has_value() || !end.has_value()) {
                say(tr("The ends of the gain range are numbers."));
                return nullptr;
            }
            if (*start == *end) {
                gain = Parameter(*start);
            } else {
                const Range range(std::min(*start, *end), std::max(*start, *end));
                gain = Parameter("k", range, range.middle());
            }
        }
    } catch (const qftbx::Exception & e) {
        say(translated(e));
        return nullptr;
    }

    std::vector<Parameter> numerator;
    std::vector<Parameter> denominator;

    if ((freedomIsCurrent() || coefficientsFromProject) && m_freedom->wasAccepted()) {
        numerator = m_freedom->numerator();
        denominator = m_freedom->denominator();
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

    return SystemDescriptionReader::makeSystem(*selectedType(), "",
                                               std::move(numerator), std::move(denominator),
                                               std::move(*gain), Parameter(0.0),
                                               ui->numeratorEdit->text().toStdString(),
                                               ui->denominatorEdit->text().toStdString());
}

void ControllerForm::on_okButton_clicked()
{
    if (m_verified == nullptr) {
        setVerified(build());
        return;
    }

    m_applied = std::move(m_verified);
    setVerified(m_applied->clone());

    markAccepted();
}

void ControllerForm::on_uncertaintyButton_clicked()
{
    CoefficientTable expressionTable;
    UncertainTable uncertainTable;
    std::optional<CoefficientTable> valueTable = readTables(expressionTable, uncertainTable);

    if (!valueTable.has_value()) {
        return;
    }

    m_freedom->launch(std::move(*valueTable), std::move(expressionTable),
                      std::move(uncertainTable), true);
    ui->pageStack->setCurrentWidget(ui->uncertaintyPage);
}

QString ControllerForm::currentCoefficients() const
{
    return ui->numeratorEdit->text() + QLatin1Char('\n') + ui->denominatorEdit->text();
}

QString ControllerForm::currentGain() const
{
    return ui->gainStart->text() + QLatin1Char('\n') + ui->gainEnd->text();
}

void ControllerForm::setFromProject(LtiSystem * structure)
{
    if (structure == nullptr) {
        return;
    }

    const SystemDescription described = describeSystem(*structure);

    m_filling = true;

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

    const Range gain = structure->gain().rawRange();
    ui->gainStart->setText(numberText(gain.min));
    ui->gainEnd->setText(numberText(gain.max));

    m_filling = false;
    showFamily();

    m_freedom->setParameters(structure->numerator(), structure->denominator(),
                             gain, structure->delay().rawRange());

    m_projectGain = structure->gain();
    m_describedCoefficients = currentCoefficients();
    m_describedGain = currentGain();
    m_freedomCoefficients = m_describedCoefficients;

    setVerified(structure->clone());
    say(QString());
}

std::unique_ptr<LtiSystem> ControllerForm::takeControllerStructure()
{
    return std::move(m_applied);
}

}
