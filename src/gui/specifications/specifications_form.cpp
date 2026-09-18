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

//How much of the form the tick boxes are worth: enough for three rows of
//them, and a scroll bar for a design with thirty frequencies.
const int kFrequencyHeight = 110;

//The band is checked here, where it was typed. Specification::constant and
//fromSystem both refuse an inverted or non-finite band, but that throw only
//happens when the records are turned into specifications, which is when the
//BOUNDARIES are computed: the form accepted "start 10, end 1" without a
//word and the complaint arrived several steps later, naming no
//specification. Worse than the message was the silence when it did not
//throw at all - a band is only used through appliesAt(), which answers
//min <= omega && omega <= max, so an inverted one quietly applied to no
//frequency and the requirement did nothing.
bool bandIsUsable(double start, double end)
{
    return std::isfinite(start) && std::isfinite(end) &&
            start >= 0.0 && end >= start;
}

//A bound's magnitude, in linear units by the time it gets here. Same story
//as the band: Specification::constant refuses a non-finite or non-positive
//magnitude, but only when the records become specifications. And a NaN gets
//here easily - "0/0" evaluates quietly to one.
bool magnitudeIsUsable(double magnitude)
{
    return std::isfinite(magnitude) && magnitude > 0.0;
}

//Nominal coefficients in the format the reader expects (space separated).
//The families other than the free form have no textual representation of
//their own, and painting numeratorString()=="" makes the specification
//vanish when it is opened again.
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

} // namespace

SpecificationsForm::SpecificationsForm(const std::vector<double> * frequencies,
                                       const qftbx::SpecificationRecords * loaded,
                                       QWidget * parent) :
    StepPanel(parent),
    m_reader(tr("Specifications"))
{
    //The step order of the main window guarantees a frequency set here, but
    //an empty one reaches front()/back() below.
    //
    //This runs BEFORE the widget tree is built on purpose: a constructor
    //that throws gets no destructor, so anything allocated before the throw
    //would be lost.
    if (frequencies == nullptr || frequencies->empty()) {
        throw qftbx::InvalidInput("The design frequencies must be entered "
                                  "before the specifications.");
    }

    m_design = frequencies;

    ui = std::make_unique<Ui::SpecificationsForm>();
    ui->setupUi(this);

    setWindowTitle(tr("Specifications"));

    //The seven, in the order of the slots they index.
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

    //If the project carries specifications (a loaded file), the form starts
    //from THEM: starting from seven empty records means the first apply
    //wipes whatever was loaded.
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

    //The frequencies a specification applies at are chosen, not typed.
    m_frequencies = new FrequencyLegend(ui->frequencyHolder);
    //Just the ticks: the label beside them already says what they are, and
    //a filter over six numbers is furniture.
    m_frequencies->setBare(true);
    //Three rows of them and then it scrolls: this is a band, not the main
    //event of the form.
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
    //The seven working records (and their systems) are members, and so is
    //anything published but never taken: nothing to free by hand.
}

void SpecificationsForm::setFrequencies(const std::vector<double> * frequencies)
{
    m_design = frequencies;

    //The ticks are the design frequencies themselves, so a new set of them
    //is a new set of ticks. What the record being edited applies at is put
    //back on the ones that are still there.
    buildFrequencyRows();
    showFrequenciesOf(m_records.at(std::size_t(selectedType())));
}

void SpecificationsForm::buildFrequencyRows()
{
    m_frequencies->clear();

    if (m_design == nullptr) {
        return;
    }

    //The colour of the text: these are not curves in a diagram, they are
    //the frequencies the design works at.
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

        //A specification nobody has entered yet applies everywhere: that is
        //the answer that needs no thinking about, and the user takes out
        //what does not belong.
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

    //The band is from the first tick to the last, and what is unticked
    //between them is the exception list: a band with a hole in it is an
    //ordinary thing to ask for, and there is no other way to say it.
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

    //The specification chosen is shown as it stands: the one already
    //entered, or an empty form for a new one.
    showRecord(selectedType());
}

Formula SpecificationsForm::boundOf(const qftbx::SpecificationRecord & record) const
{
    if (record.constant) {
        //In decibels, which is the unit a bound is read in. One upright
        //literal, units included: a constant bound is a number, not an
        //expression with a unit multiplied into it.
        return formula::number(qftbx::shownText(qftbx::linearToDb(record.height)).toStdString()
                               + " dB");
    }

    if (record.system == nullptr) {
        return Formula();
    }

    return formulaOf(*record.system, shownDigits());
}

//The whole requirement: what the program checks on the left, the bound the
//user gave on the right. Drawn and not photographed, so it says the bound
//it was given and not a W with a subscript.
Formula SpecificationsForm::formulaOfRecord(SpecificationType type,
                                            const qftbx::SpecificationRecord & record) const
{
    return requirementOf(type, boundOf(record));
}

void SpecificationsForm::setVerified(std::optional<qftbx::SpecificationRecord> record)
{
    m_verified = std::move(record);

    if (!m_verified.has_value()) {
        //Not the empty view: what this specification requires of the loop is
        //worth showing before there is a bound to put on the other side of
        //the sign, and it is what the figure of the old screens said.
        ui->boundFormula->setFormula(requirementOf(selectedType()));
        ui->addButton->setText(tr("Verify"));
        return;
    }

    ui->boundFormula->setFormula(formulaOfRecord(selectedType(), *m_verified));

    //A slot that already holds a specification is replaced, not added to:
    //there is one of each.
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
        //Written in the unit it is read in, which is the one the field
        //offers: a bound of 0.5 linear is -6.02 dB.
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

    //One already entered is shown verified: it is what it says it is.
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
        //Which slot the row stands for: the table only lists the used ones,
        //so the row number is not the type.
        name->setData(Qt::UserRole, int(i));
        ui->specificationsTable->setItem(row, 0, name);

        //The band, and how many of its frequencies it was taken out of.
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

        //The one requirement the screen cannot state on its own line: the
        //prefilter multiplies the whole closed loop, so it cannot change
        //the SPREAD of it over the plant family, and the spread against the
        //two bounds is what the loop shaping works with. The prefilter is a
        //later design, which the toolbox does not do yet.
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
        //Parses, but is not a finite number a model can use.
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

    //A free-form bound carries its expressions and no coefficient vectors.
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
    //One button and two steps: what is added has been seen first, drawn as
    //the bound it is.
    if (!m_verified.has_value()) {
        setVerified(build());
        return;
    }

    m_records.at(std::size_t(selectedType())) = m_verified->clone();

    showTable();

    //And the form is left ready for the next one.
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

    //A slot that is not used is a specification that does not exist: the
    //record is emptied, system included.
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

} // namespace qftbx
