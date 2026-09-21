/**
 * @file
 * @brief Reads, verifies and lists the specifications.
 *
 * The band and the magnitude are checked where they are typed, a finite
 * band with 0 <= start <= end and a finite positive linear magnitude,
 * since the specification constructors only see them when the boundaries
 * are computed and an inverted band would quietly apply to no frequency.
 * The frequencies are ticks: the band runs from the first tick to the last,
 * what is unticked between them is the exception list, and a specification
 * not yet entered applies everywhere. Bounds are entered in decibels and
 * stored linear. Each type has one slot, replaced rather than added to, and
 * removing one empties the record, system included. A missing frequency
 * set is refused before the widget tree is built.
 */

#include "src/gui/specifications/specifications_form.h"
#include "ui_specifications_form.h"

#include <cmath>
#include <stdexcept>

#include <QHeaderView>
#include <QRadioButton>
#include <QTableWidgetItem>

#include <algorithm>

#include "src/core/common/exception.h"
#include "src/core/common/text_tokens.h"
#include "src/core/math/constants.h"
#include "src/core/specifications/specification.h"
#include "src/core/specifications/specification_formula.h"
#include "src/core/system/system_formula.h"
#include "src/gui/common/expression_field.h"
#include "src/gui/common/field_mark.h"
#include "src/gui/common/formula_delegate.h"
#include "src/gui/common/number_text.h"

namespace qftbx {

namespace {

const int kFrequencyHeight = 110;

bool bandIsUsable(double start, double end)
{
    return std::isfinite(start) && std::isfinite(end) &&
            start >= 0.0 && end >= start;
}

bool magnitudeIsUsable(double magnitude)
{
    return std::isfinite(magnitude) && magnitude > 0.0;
}

QString coefficientsText(std::vector<Parameter> & parameters)
{
    QString text;
    for (Parameter & parameter : parameters) {
        text += qftbx::numberText(parameter.nominal()) + " ";
    }
    return text.trimmed();
}

bool isFactored(LtiSystem * system)
{
    return system->type() == LtiSystem::SystemType::ZeroPoleGain
            || system->type() == LtiSystem::SystemType::TimeConstantGain;
}

QString numeratorText(LtiSystem * system)
{
    if (system->type() == LtiSystem::SystemType::FreeForm) {
        return QString::fromStdString(system->numeratorString());
    }
    if (system->numerator().empty() && !isFactored(system)) {
        return QStringLiteral("1");
    }
    return coefficientsText(system->numerator());
}

QString denominatorText(LtiSystem * system)
{
    if (system->type() == LtiSystem::SystemType::FreeForm) {
        return QString::fromStdString(system->denominatorString());
    }
    return coefficientsText(system->denominator());
}

}

SpecificationsForm::SpecificationsForm(const std::vector<double> * frequencies,
                                       const qftbx::SpecificationRecords * loaded,
                                       QWidget * parent) :
    StepPanel(parent),
    m_reader(tr("Specifications"))
{
    if (frequencies == nullptr || frequencies->empty()) {
        throw qftbx::InvalidInput("The design frequencies must be entered "
                                  "before the specifications.");
    }

    m_design = frequencies;

    ui = std::make_unique<Ui::SpecificationsForm>();
    ui->setupUi(this);

    setWindowTitle(tr("Specifications"));

    ui->typeCombo->addItem(tr("Tracking, lower bound"));
    ui->typeCombo->addItem(tr("Tracking, upper bound"));
    ui->typeCombo->addItem(tr("Stability"));
    ui->typeCombo->addItem(tr("Sensor noise"));
    ui->typeCombo->addItem(tr("Output disturbance"));
    ui->typeCombo->addItem(tr("Input disturbance"));
    ui->typeCombo->addItem(tr("Control effort"));

    ui->specificationsTable->setColumnCount(3);
    ui->specificationsTable->setHorizontalHeaderLabels(
        {tr("Specification"), tr("Band (rad/s)"), tr("Bound")});
    ui->specificationsTable->verticalHeader()->setVisible(false);
    ui->specificationsTable->horizontalHeader()->setStretchLastSection(true);
    ui->specificationsTable->setItemDelegateForColumn(2, new FormulaDelegate(this));

    if (loaded != nullptr) {
        for (std::size_t i = 0; i < kSpecificationCount; ++i) {
            m_records.at(i) = loaded->at(i).clone();
        }
    }

    connect(ui->typeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChosen()));

    for (QRadioButton * radio : findChildren<QRadioButton *>()) {
        connect(radio, &QRadioButton::toggled, this, &SpecificationsForm::showBound);
    }

    for (QLineEdit * field : {ui->magnitudeEdit, ui->numeratorEdit, ui->denominatorEdit,
                              ui->k, ui->delayEdit}) {
        connect(field, &QLineEdit::textChanged, this, &SpecificationsForm::fieldEdited);
    }

    m_frequencies = new FrequencyLegend(ui->frequencyHolder);
    m_frequencies->setBare(true);
    m_frequencies->setMaximumHeight(kFrequencyHeight);
    ui->frequencyLayout->addWidget(m_frequencies);
    connect(m_frequencies, &FrequencyLegend::rowToggled, this, &SpecificationsForm::fieldEdited);

    buildFrequencyRows();

    ui->constantRadio->setChecked(true);
    ui->decibelsRadio->setChecked(true);
    ui->polynomialRadio->setChecked(true);

    showRecord(SpecificationType::TrackingLower);
    showTable();
}

SpecificationsForm::~SpecificationsForm()
{
}

void SpecificationsForm::setFrequencies(const std::vector<double> * frequencies)
{
    m_design = frequencies;

    buildFrequencyRows();
    showFrequenciesOf(m_records.at(std::size_t(selectedType())));
}

void SpecificationsForm::buildFrequencyRows()
{
    m_frequencies->clear();

    if (m_design == nullptr) {
        return;
    }

    const QColor ink = palette().color(QPalette::WindowText);

    for (double frequency : *m_design) {
        m_frequencies->addRow(qftbx::shownText(frequency), ink);
    }
}

void SpecificationsForm::showFrequenciesOf(const qftbx::SpecificationRecord & record)
{
    if (m_design == nullptr) {
        return;
    }

    for (int i = 0; i < m_frequencies->rowCount() && i < int(m_design->size()); ++i) {
        const double frequency = m_design->at(std::size_t(i));

        const bool applies = !record.used
                || (record.omegaStart <= frequency && frequency <= record.omegaEnd
                    && std::find(record.skipped.begin(), record.skipped.end(), frequency)
                       == record.skipped.end());

        m_frequencies->setRowChecked(i, applies);
    }
}

bool SpecificationsForm::readFrequencies(qftbx::SpecificationRecord & record)
{
    if (m_design == nullptr || m_design->empty()) {
        return false;
    }

    int first = -1;
    int last = -1;
    for (int i = 0; i < m_frequencies->rowCount() && i < int(m_design->size()); ++i) {
        if (m_frequencies->isRowChecked(i)) {
            first = first < 0 ? i : first;
            last = i;
        }
    }

    if (first < 0) {
        return false;
    }

    record.omegaStart = m_design->at(std::size_t(first));
    record.omegaEnd = m_design->at(std::size_t(last));

    record.skipped.clear();
    for (int i = first + 1; i < last; ++i) {
        if (!m_frequencies->isRowChecked(i)) {
            record.skipped.push_back(m_design->at(std::size_t(i)));
        }
    }

    return true;
}

SpecificationType SpecificationsForm::selectedType() const
{
    return static_cast<SpecificationType>(ui->typeCombo->currentIndex());
}

void SpecificationsForm::say(const QString & complaint)
{
    ui->statusLabel->setText(complaint);
    markWrong(ui->statusLabel, !complaint.isEmpty());
}

void SpecificationsForm::showBound()
{
    ui->boundStack->setCurrentWidget(ui->constantRadio->isChecked() ? ui->constantPage
                                                                    : ui->systemPage);

    const bool factored = ui->zpkRadio->isChecked() || ui->tcgRadio->isChecked();
    ui->numeratorLabel->setText(factored ? tr("Zeros:") : tr("Numerator:"));
    ui->denominatorLabel->setText(factored ? tr("Poles:") : tr("Denominator:"));

    if (m_filling) {
        return;
    }

    setVerified(std::nullopt);
}

void SpecificationsForm::fieldEdited()
{
    if (m_filling) {
        return;
    }

    setVerified(std::nullopt);
}

void SpecificationsForm::typeChosen()
{
    if (m_filling) {
        return;
    }

    showRecord(selectedType());
}

Formula SpecificationsForm::boundOf(const qftbx::SpecificationRecord & record) const
{
    if (record.constant) {
        return formula::number(qftbx::shownText(qftbx::linearToDb(record.height)).toStdString()
                               + " dB");
    }

    if (record.system == nullptr) {
        return Formula();
    }

    return formulaOf(*record.system, shownDigits());
}

Formula SpecificationsForm::formulaOfRecord(SpecificationType type,
                                            const qftbx::SpecificationRecord & record) const
{
    return requirementOf(type, boundOf(record));
}

void SpecificationsForm::setVerified(std::optional<qftbx::SpecificationRecord> record)
{
    m_verified = std::move(record);

    if (!m_verified.has_value()) {
        ui->boundFormula->setFormula(requirementOf(selectedType()));
        ui->addButton->setText(tr("Verify"));
        return;
    }

    ui->boundFormula->setFormula(formulaOfRecord(selectedType(), *m_verified));

    ui->addButton->setText(m_records.at(std::size_t(ui->typeCombo->currentIndex())).used
                           ? tr("Update") : tr("Add"));
}

void SpecificationsForm::showRecord(SpecificationType type)
{
    m_filling = true;

    ui->typeCombo->setCurrentIndex(int(type));

    const SpecificationRecord & record = m_records.at(std::size_t(type));

    showFrequenciesOf(record);

    ui->magnitudeEdit->clear();
    ui->numeratorEdit->clear();
    ui->denominatorEdit->clear();
    ui->k->setText("1");
    ui->delayEdit->setText("0");

    if (record.used && record.constant) {
        ui->constantRadio->setChecked(true);
        ui->decibelsRadio->setChecked(true);
        ui->magnitudeEdit->setText(qftbx::numberText(qftbx::linearToDb(record.height)));
    } else if (record.used && record.system != nullptr) {
        ui->systemRadio->setChecked(true);

        switch (record.system->type()) {
        case LtiSystem::SystemType::PolynomialForm: ui->polynomialRadio->setChecked(true); break;
        case LtiSystem::SystemType::ZeroPoleGain:   ui->zpkRadio->setChecked(true); break;
        case LtiSystem::SystemType::TimeConstantGain: ui->tcgRadio->setChecked(true); break;
        case LtiSystem::SystemType::FreeForm:       ui->freeFormRadio->setChecked(true); break;
        }

        ui->numeratorEdit->setText(numeratorText(record.system.get()));
        ui->denominatorEdit->setText(denominatorText(record.system.get()));
        ui->k->setText(qftbx::numberText(record.system->gain().nominal()));
        ui->delayEdit->setText(qftbx::numberText(record.system->delay().nominal()));
    }

    for (QWidget * field : {static_cast<QWidget *>(ui->magnitudeEdit),
                            static_cast<QWidget *>(ui->numeratorEdit),
                            static_cast<QWidget *>(ui->denominatorEdit),
                            static_cast<QWidget *>(ui->k),
                            static_cast<QWidget *>(ui->delayEdit)}) {
        markWrong(field, false);
    }

    m_filling = false;

    showBound();
    say(QString());

    setVerified(record.used ? std::optional<SpecificationRecord>(record.clone())
                            : std::nullopt);
}

void SpecificationsForm::showTable()
{
    ui->specificationsTable->setRowCount(0);

    for (std::size_t i = 0; i < kSpecificationCount; ++i) {
        const SpecificationRecord & record = m_records.at(i);
        if (!record.used) {
            continue;
        }

        const int row = ui->specificationsTable->rowCount();
        ui->specificationsTable->insertRow(row);

        QTableWidgetItem * name = new QTableWidgetItem(ui->typeCombo->itemText(int(i)));
        name->setData(Qt::UserRole, int(i));
        ui->specificationsTable->setItem(row, 0, name);

        QString band = tr("%1 to %2").arg(qftbx::shownText(record.omegaStart),
                                          qftbx::shownText(record.omegaEnd));
        if (!record.skipped.empty()) {
            band += tr(" (%n out)", "", int(record.skipped.size()));
        }
        ui->specificationsTable->setItem(row, 1, new QTableWidgetItem(band));

        QTableWidgetItem * bound = new QTableWidgetItem();
        bound->setData(FormulaDelegate::formulaRole,
                       QVariant::fromValue(formulaOfRecord(static_cast<SpecificationType>(i),
                                                           record)));

        if (i == std::size_t(SpecificationType::TrackingLower)
                || i == std::size_t(SpecificationType::TrackingUpper)) {
            bound->setToolTip(tr("The loop shaping bounds the spread of the closed loop over "
                                 "the plant family against the difference between the two "
                                 "tracking bounds: the prefilter F shifts the band and cannot "
                                 "narrow it, and it is designed afterwards."));
        }

        ui->specificationsTable->setItem(row, 2, bound);
    }

    ui->specificationsTable->resizeColumnsToContents();
    ui->specificationsTable->resizeRowsToContents();

    const bool anything = ui->specificationsTable->rowCount() > 0;
    ui->editButton->setEnabled(anything);
    ui->removeButton->setEnabled(anything);
    ui->okButton->setEnabled(anything);
}

std::optional<std::vector<Parameter>> SpecificationsForm::parametersFrom(const QString & text)
{
    CoefficientRow numbers;
    for (const std::string & token : qftbx::text::tokens(text.toStdString())) {
        numbers.push_back(QString::fromStdString(token));
    }

    return m_reader.buildParameters(numbers);
}

std::optional<Parameter> SpecificationsForm::scalarFrom(const QString & text, double fallback)
{
    if (text.trimmed().isEmpty()) {
        return Parameter(fallback);
    }

    const std::optional<double> value = m_reader.evaluate(text);
    if (!value.has_value()) {
        return std::nullopt;
    }

    try {
        return Parameter(*value);
    } catch (const qftbx::Exception &) {
        return std::nullopt;
    }
}

std::optional<qftbx::SpecificationRecord> SpecificationsForm::build()
{
    const auto refuse = [this](QWidget * field, const QString & complaint) {
        markWrong(field, true, complaint);
        say(complaint);
        return std::optional<SpecificationRecord>();
    };

    for (QWidget * field : {static_cast<QWidget *>(ui->magnitudeEdit),
                            static_cast<QWidget *>(ui->numeratorEdit),
                            static_cast<QWidget *>(ui->denominatorEdit),
                            static_cast<QWidget *>(ui->k),
                            static_cast<QWidget *>(ui->delayEdit)}) {
        markWrong(field, false);
    }

    if (m_design == nullptr || m_design->empty()) {
        say(tr("The design frequencies must be entered before the specifications."));
        return std::nullopt;
    }

    SpecificationRecord record;
    record.name = specificationName(selectedType());

    if (!readFrequencies(record)) {
        return refuse(m_frequencies, tr("A specification applies at least at one frequency."));
    }
    markWrong(m_frequencies, false);

    if (!bandIsUsable(record.omegaStart, record.omegaEnd)) {
        return refuse(m_frequencies, tr("The band needs 0 <= start <= end."));
    }

    if (ui->constantRadio->isChecked()) {
        const std::optional<double> magnitude = evaluateNumber(ui->magnitudeEdit->text());
        if (!magnitude.has_value()) {
            return refuse(ui->magnitudeEdit, tr("The bound is a magnitude."));
        }

        record.constant = true;
        record.height = ui->decibelsRadio->isChecked() ? qftbx::dbToLinear(*magnitude)
                                                       : *magnitude;

        if (!magnitudeIsUsable(record.height)) {
            return refuse(ui->magnitudeEdit, tr("The magnitude must be a finite number, "
                                                "and positive in linear units."));
        }

        record.used = true;
        say(QString());

        return record;
    }

    LtiSystem::SystemType type = LtiSystem::SystemType::FreeForm;
    if (ui->zpkRadio->isChecked()) {
        type = LtiSystem::SystemType::ZeroPoleGain;
    } else if (ui->tcgRadio->isChecked()) {
        type = LtiSystem::SystemType::TimeConstantGain;
    } else if (ui->polynomialRadio->isChecked()) {
        type = LtiSystem::SystemType::PolynomialForm;
    }

    const std::optional<Parameter> gain = scalarFrom(ui->k->text(), 1.0);
    if (!gain.has_value()) {
        return refuse(ui->k, tr("The gain is not a number."));
    }

    const std::optional<Parameter> delay = scalarFrom(ui->delayEdit->text(), 0.0);
    if (!delay.has_value()) {
        return refuse(ui->delayEdit, tr("The delay is not a number."));
    }

    std::vector<Parameter> numerator;
    std::vector<Parameter> denominator;

    if (type == LtiSystem::SystemType::FreeForm) {
        for (QLineEdit * field : {ui->numeratorEdit, ui->denominatorEdit}) {
            if (field->text().trimmed().isEmpty()) {
                return refuse(field, tr("A free-form bound needs both expressions."));
            }
            try {
                ExpressionTree parsed(field->text().toStdString());
            } catch (const std::invalid_argument &) {
                return refuse(field, tr("This is not an expression the toolbox can read."));
            }
        }
    } else {
        if (ui->denominatorEdit->text().trimmed().isEmpty()) {
            return refuse(ui->denominatorEdit, tr("The bound needs a denominator."));
        }

        std::optional<std::vector<Parameter>> readNumerator =
                parametersFrom(ui->numeratorEdit->text());
        if (!readNumerator.has_value()) {
            return refuse(ui->numeratorEdit, tr("A coefficient is not a number."));
        }

        std::optional<std::vector<Parameter>> readDenominator =
                parametersFrom(ui->denominatorEdit->text());
        if (!readDenominator.has_value()) {
            return refuse(ui->denominatorEdit, tr("A coefficient is not a number."));
        }

        numerator = std::move(*readNumerator);
        denominator = std::move(*readDenominator);
    }

    record.constant = false;
    record.system = SystemDescriptionReader::makeSystem(
        type, record.name, std::move(numerator), std::move(denominator), *gain, *delay,
        ui->numeratorEdit->text().toStdString(), ui->denominatorEdit->text().toStdString());
    record.used = true;

    say(QString());

    return record;
}

void SpecificationsForm::on_addButton_clicked()
{
    if (!m_verified.has_value()) {
        setVerified(build());
        return;
    }

    m_records.at(std::size_t(selectedType())) = m_verified->clone();

    showTable();

    setVerified(std::nullopt);
    say(tr("Added: %1.").arg(ui->typeCombo->currentText()));
    markWrong(ui->statusLabel, false);
}

void SpecificationsForm::on_clearButton_clicked()
{
    m_filling = true;
    ui->magnitudeEdit->clear();
    ui->numeratorEdit->clear();
    ui->denominatorEdit->clear();
    ui->k->setText("1");
    ui->delayEdit->setText("0");
    m_filling = false;

    setVerified(std::nullopt);
    say(QString());
}

void SpecificationsForm::on_editButton_clicked()
{
    const int row = ui->specificationsTable->currentRow();
    if (row < 0) {
        say(tr("Choose a specification in the list first."));
        return;
    }

    const int slot = ui->specificationsTable->item(row, 0)->data(Qt::UserRole).toInt();

    showRecord(static_cast<SpecificationType>(slot));
}

void SpecificationsForm::on_removeButton_clicked()
{
    const int row = ui->specificationsTable->currentRow();
    if (row < 0) {
        say(tr("Choose a specification in the list first."));
        return;
    }

    const int slot = ui->specificationsTable->item(row, 0)->data(Qt::UserRole).toInt();

    m_records.at(std::size_t(slot)) = SpecificationRecord();

    showTable();

    if (int(selectedType()) == slot) {
        showRecord(static_cast<SpecificationType>(slot));
    }
}

void SpecificationsForm::on_okButton_clicked()
{
    qftbx::SpecificationRecords published;
    for (std::size_t i = 0; i < kSpecificationCount; ++i) {
        published.at(i) = m_records.at(i).clone();
    }

    m_published = std::move(published);

    markAccepted();
}

std::optional<qftbx::SpecificationRecords> SpecificationsForm::takeSpecifications()
{
    std::optional<qftbx::SpecificationRecords> published = std::move(m_published);
    m_published.reset();

    return published;
}

}
