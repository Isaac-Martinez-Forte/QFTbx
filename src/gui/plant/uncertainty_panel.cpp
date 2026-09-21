/**
 * @file
 * @brief Builds the rows of the uncertainty panel and reads them back.
 *
 * Rows are built for the distinct uncertain names in order of appearance,
 * numerator first, under one heading so the fields say which is which
 * once; a name the loaded system already knows opens on its interval.
 * Rows leave the layout by destroying their widgets. Every row is read so
 * that every mistake is marked at once, and the parameter carries the
 * reparametrising expression of the first appearance of its name. A
 * coefficient that is not uncertain is the number its field evaluates to,
 * an expression included. An empty gain range is the unit gain and an
 * empty delay range is zero. Number fields are kept narrow, since a
 * field as wide as the card reads worse.
 */

#include "src/gui/plant/uncertainty_panel.h"
#include "ui_uncertainty_panel.h"

#include <algorithm>
#include <optional>

#include <QFont>
#include <QGridLayout>
#include <QLabel>

#include "src/core/common/exception.h"
#include "src/gui/common/expression_field.h"
#include "src/gui/common/field_mark.h"
#include "src/gui/common/number_text.h"

namespace qftbx {

namespace {

const std::size_t kNumerator = 0;
const std::size_t kDenominator = 1;

const int kFieldWidth = 110;

}

UncertaintyPanel::UncertaintyPanel(QWidget * parent)
    : QWidget(parent),
      ui(std::make_unique<Ui::UncertaintyPanel>())
{
    ui->setupUi(this);

    ui->emptyLabel->setVisible(false);

    QFont heading = ui->uncertaintyTitle->font();
    heading.setBold(true);
    ui->uncertaintyTitle->setFont(heading);

    for (QLineEdit * field : {ui->rangeGainStart, ui->rangeGainEnd,
                              ui->rangeDelayStart, ui->rangeDelayEnd}) {
        field->setMaximumWidth(kFieldWidth);
    }
    ui->scalarsLayout->setColumnStretch(4, 1);
}

UncertaintyPanel::~UncertaintyPanel()
{
}

void UncertaintyPanel::setTitle(const QString & title)
{
    ui->uncertaintyTitle->setText(title);
}

bool UncertaintyPanel::launch(CoefficientTable valueTable, CoefficientTable expressionTable,
                              UncertainTable uncertainTable, bool rangeOnly)
{
    m_values = std::move(valueTable);
    m_expressions = std::move(expressionTable);
    m_uncertain = std::move(uncertainTable);
    m_rangeOnly = rangeOnly;

    ui->scalarsWidget->setVisible(!rangeOnly);

    say(QString());
    buildRows();

    return true;
}

void UncertaintyPanel::setParameters(const std::vector<Parameter> & numerator,
                                     const std::vector<Parameter> & denominator,
                                     const Range & gain, const Range & delay)
{
    m_numerator = numerator;
    m_denominator = denominator;

    m_known.clear();
    for (const std::vector<Parameter> * polynomial : {&numerator, &denominator}) {
        for (const Parameter & parameter : *polynomial) {
            if (parameter.isUncertain()) {
                m_known.push_back(parameter);
            }
        }
    }

    ui->rangeGainStart->setText(numberText(gain.min));
    ui->rangeGainEnd->setText(numberText(gain.max));
    ui->rangeDelayStart->setText(numberText(delay.min));
    ui->rangeDelayEnd->setText(numberText(delay.max));

    m_accepted = true;
}

std::vector<QString> UncertaintyPanel::uncertainNames() const
{
    std::vector<QString> names;

    for (std::size_t slot : {kNumerator, kDenominator}) {
        if (slot >= m_values.size() || slot >= m_uncertain.size()) {
            continue;
        }
        for (std::size_t i = 0; i < m_values.at(slot).size(); ++i) {
            const QString & name = m_values.at(slot).at(i);
            if (i < m_uncertain.at(slot).size() && m_uncertain.at(slot).at(i)
                    && std::find(names.begin(), names.end(), name) == names.end()) {
                names.push_back(name);
            }
        }
    }

    return names;
}

void UncertaintyPanel::buildRows()
{
    for (QWidget * row : m_rowWidgets) {
        delete row;
    }
    m_rowWidgets.clear();
    m_rows.clear();

    QGridLayout * table = ui->parametersLayout;

    const std::vector<QString> names = uncertainNames();

    ui->emptyLabel->setVisible(names.empty());

    if (names.empty()) {
        return;
    }

    int row = 0;
    const auto heading = [&](int column, const QString & text) {
        QLabel * label = new QLabel(text, ui->parametersContent);
        label->setObjectName("heading");
        table->addWidget(label, row, column);
        m_rowWidgets.push_back(label);
    };

    heading(0, tr("Parameter"));
    heading(1, tr("Minimum"));
    if (!m_rangeOnly) {
        heading(2, tr("Nominal"));
    }
    heading(3, tr("Maximum"));

    for (const QString & name : names) {
        ++row;

        Row entry;
        entry.name = name;

        QLabel * label = new QLabel(name, ui->parametersContent);
        entry.minimum = new QLineEdit(ui->parametersContent);
        entry.nominal = new QLineEdit(ui->parametersContent);
        entry.maximum = new QLineEdit(ui->parametersContent);

        entry.minimum->setObjectName("rangeMinimum");
        entry.nominal->setObjectName("rangeNominal");
        entry.maximum->setObjectName("rangeMaximum");

        entry.minimum->setToolTip(tr("The smallest value \"%1\" takes. The templates are swept "
                                     "over the whole interval.").arg(name));
        entry.maximum->setToolTip(tr("The largest value \"%1\" takes.").arg(name));
        entry.nominal->setToolTip(tr("The value \"%1\" has in the NOMINAL plant, the one the "
                                     "loop is shaped on. It has to lie inside the interval.")
                                  .arg(name));

        for (QLineEdit * field : {entry.minimum, entry.nominal, entry.maximum}) {
            field->setMaximumWidth(kFieldWidth);
        }

        table->addWidget(label, row, 0);
        table->addWidget(entry.minimum, row, 1);
        table->addWidget(entry.nominal, row, 2);
        table->addWidget(entry.maximum, row, 3);
        table->setColumnStretch(4, 1);

        entry.nominal->setVisible(!m_rangeOnly);

        for (const Parameter & parameter : m_known) {
            if (parameter.name() == name.toStdString()) {
                entry.minimum->setText(numberText(parameter.rawRange().min));
                entry.maximum->setText(numberText(parameter.rawRange().max));
                entry.nominal->setText(numberText(parameter.rawNominal()));
                break;
            }
        }

        m_rowWidgets.push_back(label);
        m_rowWidgets.push_back(entry.minimum);
        m_rowWidgets.push_back(entry.nominal);
        m_rowWidgets.push_back(entry.maximum);

        m_rows.push_back(entry);
    }
}

std::optional<double> UncertaintyPanel::valueOf(QLineEdit * field)
{
    if (field == nullptr || field->text().trimmed().isEmpty()) {
        return std::nullopt;
    }

    return evaluateNumber(field->text());
}

void UncertaintyPanel::say(const QString & complaint)
{
    ui->uncertaintyStatus->setText(complaint);
    markWrong(ui->uncertaintyStatus, !complaint.isEmpty());
}

bool UncertaintyPanel::readRanges()
{
    bool valid = true;

    std::vector<Parameter> named;

    for (Row & row : m_rows) {
        const std::optional<double> minimum = valueOf(row.minimum);
        const std::optional<double> maximum = valueOf(row.maximum);

        if (m_rangeOnly && minimum.has_value() && maximum.has_value()) {
            row.nominal->setText(numberText((*minimum + *maximum) / 2.0));
        }

        const std::optional<double> nominal = valueOf(row.nominal);

        const QString complaint = tr("The range of \"%1\" is not a pair of numbers with the "
                                     "nominal value between them.").arg(row.name);

        const bool readable = minimum.has_value() && maximum.has_value() && nominal.has_value();
        const bool ordered = readable && *minimum <= *nominal && *nominal <= *maximum;

        markWrong(row.minimum, !readable || !ordered, complaint);
        markWrong(row.maximum, !readable || !ordered, complaint);
        markWrong(row.nominal, !readable || !ordered, complaint);

        if (!ordered) {
            say(complaint);
            valid = false;
            continue;
        }

        std::string expression = row.name.toStdString();
        for (std::size_t slot : {kNumerator, kDenominator}) {
            if (slot >= m_values.size()) {
                continue;
            }
            for (std::size_t i = 0; i < m_values.at(slot).size(); ++i) {
                if (m_values.at(slot).at(i) == row.name && slot < m_expressions.size()
                        && i < m_expressions.at(slot).size()) {
                    expression = m_expressions.at(slot).at(i).toStdString();
                    break;
                }
            }
        }

        try {
            named.push_back(Parameter(row.name.toStdString(), Range(*minimum, *maximum),
                                      *nominal, expression));
        } catch (const qftbx::Exception &) {
            markWrong(row.minimum, true, complaint);
            markWrong(row.maximum, true, complaint);
            markWrong(row.nominal, true, complaint);
            say(complaint);
            valid = false;
        }
    }

    if (!valid) {
        return false;
    }

    std::vector<Parameter> numerator = parametersOf(kNumerator, named, valid);
    std::vector<Parameter> denominator = parametersOf(kDenominator, named, valid);

    if (!valid) {
        say(tr("A coefficient that is not uncertain is not a number either."));
        return false;
    }

    m_numerator = std::move(numerator);
    m_denominator = std::move(denominator);

    say(QString());

    return true;
}

std::vector<Parameter> UncertaintyPanel::parametersOf(std::size_t slot,
                                                      const std::vector<Parameter> & named,
                                                      bool & valid)
{
    std::vector<Parameter> parameters;

    if (slot >= m_values.size()) {
        return parameters;
    }

    for (std::size_t i = 0; i < m_values.at(slot).size(); ++i) {
        const QString & token = m_values.at(slot).at(i);

        if (slot < m_uncertain.size() && i < m_uncertain.at(slot).size()
                && m_uncertain.at(slot).at(i)) {
            const auto found = std::find_if(named.begin(), named.end(),
                                            [&token](const Parameter & parameter) {
                                                return parameter.name() == token.toStdString();
                                            });
            if (found != named.end()) {
                parameters.push_back(*found);
                continue;
            }
        }

        const std::optional<double> value = evaluateNumber(token);
        if (!value.has_value()) {
            valid = false;
            return parameters;
        }

        try {
            parameters.push_back(Parameter(*value));
        } catch (const qftbx::Exception &) {
            valid = false;
            return parameters;
        }
    }

    return parameters;
}

std::vector<Parameter> & UncertaintyPanel::numerator()
{
    return m_numerator;
}

std::vector<Parameter> & UncertaintyPanel::denominator()
{
    return m_denominator;
}

Range UncertaintyPanel::gain()
{
    if (ui->rangeGainStart->text().isEmpty() || ui->rangeGainEnd->text().isEmpty()) {
        ui->rangeGainStart->setText("1");
        ui->rangeGainEnd->setText("1");
    }

    const std::optional<double> start = valueOf(ui->rangeGainStart);
    const std::optional<double> end = valueOf(ui->rangeGainEnd);

    if (!start.has_value() || !end.has_value()) {
        throw qftbx::InvalidInput("the gain range is not a pair of numbers");
    }

    return Range(*start, *end);
}

Range UncertaintyPanel::delay()
{
    if (ui->rangeDelayStart->text().isEmpty() || ui->rangeDelayEnd->text().isEmpty()) {
        ui->rangeDelayStart->setText("0");
        ui->rangeDelayEnd->setText("0");
    }

    const std::optional<double> start = valueOf(ui->rangeDelayStart);
    const std::optional<double> end = valueOf(ui->rangeDelayEnd);

    if (!start.has_value() || !end.has_value()) {
        throw qftbx::InvalidInput("the delay range is not a pair of numbers");
    }

    return Range(*start, *end);
}

bool UncertaintyPanel::wasAccepted() const
{
    return m_accepted;
}

void UncertaintyPanel::on_applyButton_clicked()
{
    if (!readRanges()) {
        return;
    }

    m_accepted = true;
    emit applied();
}

void UncertaintyPanel::on_backButton_clicked()
{
    emit cancelled();
}

}
