#include "src/gui/bench/plan_editor.h"

#include <exception>

#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStringList>
#include <QTableWidget>
#include <QThread>
#include <QVBoxLayout>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/frequencies/omega.h"
#include "src/core/project/settings.h"
#include "src/gui/common/expression_field.h"

namespace qftbx {

namespace {

const int kColumnKind = 0;
const int kColumnMin = 1;
const int kColumnMax = 2;
const int kColumnRun = 3;

double numberOf(const QLineEdit * field, const QString & name)
{
    const std::optional<double> value = evaluateNumber(field->text());
    if (!value || !std::isfinite(*value)) {
        throw InvalidInput(QStringLiteral("%1: '%2' is not a number").arg(name, field->text()).toStdString());
    }
    return *value;
}

QString text(double value)
{
    return QString::number(value, 'g', 15);
}

} // namespace

PlanEditor::PlanEditor(QWidget * parent) : QWidget(parent)
{
    build();
    setPlan(bench::examplePlan());
}

void PlanEditor::build()
{
    auto * outer = new QVBoxLayout(this);
    auto * scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);
    auto * page = new QWidget(scroll);
    scroll->setWidget(page);
    auto * layout = new QVBoxLayout(page);

    //--- the project and the output
    auto * projectBox = new QGroupBox(tr("Project"), page);
    auto * projectForm = new QFormLayout(projectBox);
    auto * projectRow = new QHBoxLayout();
    m_project = new QLineEdit(projectBox);
    m_project->setObjectName("projectFile");
    m_project->setPlaceholderText(tr("a .qft with its templates and boundaries computed"));
    auto * browseProject = new QPushButton(tr("Browse..."), projectBox);
    projectRow->addWidget(m_project);
    projectRow->addWidget(browseProject);
    projectForm->addRow(tr("Project file"), projectRow);
    m_projectSummary = new QLabel(projectBox);
    m_projectSummary->setObjectName("projectSummary");
    m_projectSummary->setWordWrap(true);
    projectForm->addRow(QString(), m_projectSummary);
    m_name = new QLineEdit(projectBox);
    m_name->setObjectName("planName");
    projectForm->addRow(tr("Plan name"), m_name);
    auto * outputRow = new QHBoxLayout();
    m_output = new QLineEdit(projectBox);
    m_output->setObjectName("outputDirectory");
    auto * browseOutput = new QPushButton(tr("Browse..."), projectBox);
    outputRow->addWidget(m_output);
    outputRow->addWidget(browseOutput);
    projectForm->addRow(tr("Output directory"), outputRow);
    layout->addWidget(projectBox);

    //--- the structures
    auto * structuresBox = new QGroupBox(tr("Controller structures"), page);
    auto * structuresLayout = new QVBoxLayout(structuresBox);
    auto * hint = new QLabel(tr("The project's controller structure is the base. Each step adds a zero or a "
                                "pole with its search range; the cases run after every step marked to run."), structuresBox);
    hint->setWordWrap(true);
    structuresLayout->addWidget(hint);
    m_runBase = new QCheckBox(tr("Run the base structure as it is"), structuresBox);
    m_runBase->setObjectName("runBase");
    structuresLayout->addWidget(m_runBase);
    auto * gainRow = new QHBoxLayout();
    m_overrideGain = new QCheckBox(tr("Replace the gain range:"), structuresBox);
    m_overrideGain->setObjectName("overrideGain");
    m_gainMin = new QLineEdit(structuresBox);
    m_gainMin->setObjectName("gainMin");
    m_gainMax = new QLineEdit(structuresBox);
    m_gainMax->setObjectName("gainMax");
    gainRow->addWidget(m_overrideGain);
    gainRow->addWidget(new QLabel(tr("from"), structuresBox));
    gainRow->addWidget(m_gainMin);
    gainRow->addWidget(new QLabel(tr("to"), structuresBox));
    gainRow->addWidget(m_gainMax);
    gainRow->addStretch();
    structuresLayout->addLayout(gainRow);

    m_steps = new QTableWidget(0, 4, structuresBox);
    m_steps->setObjectName("steps");
    m_steps->setHorizontalHeaderLabels({tr("Adds"), tr("Minimum"), tr("Maximum"), tr("Run after it")});
    m_steps->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_steps->verticalHeader()->setVisible(false);
    m_steps->setSelectionMode(QAbstractItemView::SingleSelection);
    m_steps->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_steps->setMinimumHeight(140);
    structuresLayout->addWidget(m_steps);
    auto * stepButtons = new QHBoxLayout();
    auto * addZero = new QPushButton(tr("Add a zero"), structuresBox);
    addZero->setObjectName("addZero");
    auto * addPole = new QPushButton(tr("Add a pole"), structuresBox);
    addPole->setObjectName("addPole");
    auto * removeStep = new QPushButton(tr("Remove"), structuresBox);
    auto * up = new QPushButton(tr("Move up"), structuresBox);
    auto * down = new QPushButton(tr("Move down"), structuresBox);
    for (QPushButton * b : {addZero, addPole, removeStep, up, down}) {
        stepButtons->addWidget(b);
    }
    stepButtons->addStretch();
    structuresLayout->addLayout(stepButtons);
    m_structures = new QLabel(structuresBox);
    m_structures->setObjectName("structuresSummary");
    m_structures->setWordWrap(true);
    structuresLayout->addWidget(m_structures);
    layout->addWidget(structuresBox);

    //--- what runs
    auto * runsBox = new QGroupBox(tr("Runs"), page);
    auto * runsForm = new QFormLayout(runsBox);
    auto * algorithmsRow = new QHBoxLayout();
    for (const LoopShapingAlgorithm algorithm : {nt, nk, mr, mc1, mc_thesis}) {
        auto * box = new QCheckBox(QString::fromLatin1(bench::algorithmName(algorithm)), runsBox);
        box->setObjectName(QStringLiteral("algorithm_%1").arg(bench::algorithmName(algorithm)));
        algorithmsRow->addWidget(box);
        m_algorithms.emplace_back(algorithm, box);
    }
    algorithmsRow->addStretch();
    runsForm->addRow(tr("Algorithms"), algorithmsRow);
    m_epsilons = new QLineEdit(runsBox);
    m_epsilons->setObjectName("epsilons");
    m_epsilons->setPlaceholderText(tr("one or more, separated by commas: 2, 5"));
    runsForm->addRow(tr("Epsilons (dB)"), m_epsilons);
    auto * repetitionsRow = new QHBoxLayout();
    m_repetitions = new QSpinBox(runsBox);
    m_repetitions->setObjectName("repetitions");
    m_repetitions->setRange(1, 1000);
    m_warmUp = new QCheckBox(tr("plus a warm-up run left out of the statistics"), runsBox);
    m_warmUp->setObjectName("warmUp");
    repetitionsRow->addWidget(m_repetitions);
    repetitionsRow->addWidget(m_warmUp);
    repetitionsRow->addStretch();
    runsForm->addRow(tr("Repetitions"), repetitionsRow);
    layout->addWidget(runsBox);

    //--- what is measured
    auto * measureBox = new QGroupBox(tr("Measure"), page);
    auto * measureLayout = new QVBoxLayout(measureBox);
    m_measureTime = new QCheckBox(tr("Wall-clock time"), measureBox);
    m_measureTime->setObjectName("measureTime");
    m_measureCpu = new QCheckBox(tr("CPU time"), measureBox);
    m_measureCpu->setObjectName("measureCpu");
    m_measureMemory = new QCheckBox(tr("Peak memory (read at the end, costs nothing)"), measureBox);
    m_measureMemory->setObjectName("measureMemory");
    m_measureTrace = new QCheckBox(tr("Memory over time (a sampling thread: leave it off when timing)"), measureBox);
    m_measureTrace->setObjectName("measureTrace");
    m_measureCounters = new QCheckBox(tr("The algorithm's counters: live nodes, boxes, stability verdicts"), measureBox);
    m_measureCounters->setObjectName("measureCounters");
    for (QCheckBox * b : {m_measureTime, m_measureCpu, m_measureMemory, m_measureTrace, m_measureCounters}) {
        measureLayout->addWidget(b);
    }
    layout->addWidget(measureBox);

    //--- how it executes
    auto * executionBox = new QGroupBox(tr("Execution"), page);
    auto * executionForm = new QFormLayout(executionBox);
    auto * jobsRow = new QHBoxLayout();
    m_jobs = new QSpinBox(executionBox);
    m_jobs->setObjectName("jobs");
    m_jobs->setRange(0, 1024);
    m_jobsHint = new QLabel(executionBox);
    jobsRow->addWidget(m_jobs);
    jobsRow->addWidget(m_jobsHint);
    jobsRow->addStretch();
    executionForm->addRow(tr("Cases at once"), jobsRow);
    m_timeout = new QSpinBox(executionBox);
    m_timeout->setObjectName("timeoutSeconds");
    m_timeout->setRange(0, 1000000);
    m_timeout->setSpecialValueText(tr("none"));
    m_timeout->setSuffix(tr(" s"));
    executionForm->addRow(tr("Time limit per case"), m_timeout);
    m_memoryLimit = new QSpinBox(executionBox);
    m_memoryLimit->setObjectName("memoryLimitMegabytes");
    m_memoryLimit->setRange(0, 1000000);
    m_memoryLimit->setSpecialValueText(tr("none"));
    m_memoryLimit->setSuffix(tr(" MB"));
    executionForm->addRow(tr("Memory limit per case"), m_memoryLimit);
    layout->addWidget(executionBox);

    //--- settings overrides
    auto * settingsBox = new QGroupBox(tr("Settings for the runs"), page);
    auto * settingsLayout = new QVBoxLayout(settingsBox);
    auto * settingsHint = new QLabel(tr("Values of qftbx.conf that the runs use instead of the defaults, by key."), settingsBox);
    settingsHint->setWordWrap(true);
    settingsLayout->addWidget(settingsHint);
    m_settings = new QTableWidget(0, 2, settingsBox);
    m_settings->setObjectName("settings");
    m_settings->setHorizontalHeaderLabels({tr("Key"), tr("Value")});
    m_settings->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_settings->verticalHeader()->setVisible(false);
    m_settings->setMinimumHeight(90);
    settingsLayout->addWidget(m_settings);
    auto * settingsButtons = new QHBoxLayout();
    auto * addSetting = new QPushButton(tr("Add"), settingsBox);
    addSetting->setObjectName("addSetting");
    auto * removeSetting = new QPushButton(tr("Remove"), settingsBox);
    settingsButtons->addWidget(addSetting);
    settingsButtons->addWidget(removeSetting);
    settingsButtons->addStretch();
    settingsLayout->addLayout(settingsButtons);
    layout->addWidget(settingsBox);
    layout->addStretch();

    //--- wiring
    connect(browseProject, &QPushButton::clicked, this, [this]() {
        const QString path = chooseFile(false, false, tr("QFT projects (*.qft)"));
        if (!path.isEmpty()) {
            m_project->setText(path);
        }
    });
    connect(browseOutput, &QPushButton::clicked, this, [this]() {
        const QString path = chooseFile(true, true, QString());
        if (!path.isEmpty()) {
            m_output->setText(path);
        }
    });
    connect(m_project, &QLineEdit::editingFinished, this, [this]() { refreshProjectSummary(); });
    connect(m_project, &QLineEdit::textChanged, this, [this]() { changed(); });
    for (QLineEdit * field : {m_name, m_output, m_gainMin, m_gainMax, m_epsilons}) {
        connect(field, &QLineEdit::textChanged, this, [this]() { changed(); });
    }
    for (QCheckBox * box : {m_runBase, m_overrideGain, m_warmUp, m_measureTime, m_measureCpu, m_measureMemory,
                            m_measureTrace, m_measureCounters}) {
        connect(box, &QCheckBox::toggled, this, [this]() { changed(); });
    }
    for (auto & [algorithm, box] : m_algorithms) {
        connect(box, &QCheckBox::toggled, this, [this]() { changed(); });
    }
    for (QSpinBox * spin : {m_repetitions, m_jobs, m_timeout, m_memoryLimit}) {
        connect(spin, &QSpinBox::valueChanged, this, [this]() { changed(); });
    }
    connect(m_steps, &QTableWidget::cellChanged, this, [this]() { changed(); });
    connect(m_settings, &QTableWidget::cellChanged, this, [this]() { changed(); });
    connect(addZero, &QPushButton::clicked, this, [this]() { addStep(bench::StructureStep::Kind::Zero); });
    connect(addPole, &QPushButton::clicked, this, [this]() { addStep(bench::StructureStep::Kind::Pole); });
    connect(removeStep, &QPushButton::clicked, this, [this]() { this->removeStep(); });
    connect(up, &QPushButton::clicked, this, [this]() { moveStep(-1); });
    connect(down, &QPushButton::clicked, this, [this]() { moveStep(1); });
    connect(addSetting, &QPushButton::clicked, this, [this]() { this->addSetting(); });
    connect(removeSetting, &QPushButton::clicked, this, [this]() { this->removeSetting(); });
}

QString PlanEditor::chooseFile(bool forSaving, bool directory, const QString & filter)
{
    if (m_chooseFile) {
        return m_chooseFile(forSaving, directory, filter);
    }
    if (directory) {
        return QFileDialog::getExistingDirectory(this, tr("Output directory"));
    }
    return forSaving ? QFileDialog::getSaveFileName(this, tr("Save"), QString(), filter)
                     : QFileDialog::getOpenFileName(this, tr("Open"), QString(), filter);
}

void PlanEditor::changed()
{
    if (m_loading) {
        return;
    }
    refreshStructures();
    if (m_changed) {
        m_changed();
    }
}

void PlanEditor::refreshStructures()
{
    QStringList labels;
    if (m_runBase->isChecked()) {
        labels << tr("base");
    }
    QString label = tr("base");
    for (int row = 0; row < m_steps->rowCount(); ++row) {
        auto * kind = qobject_cast<QComboBox *>(m_steps->cellWidget(row, kColumnKind));
        label += kind != nullptr && kind->currentIndex() == 1 ? "+p" : "+z";
        QTableWidgetItem * run = m_steps->item(row, kColumnRun);
        if (run != nullptr && run->checkState() == Qt::Checked) {
            labels << label;
        }
    }
    m_structures->setText(labels.isEmpty() ? tr("No structure runs: mark the base or a step.")
                                           : tr("Structures to run (%1): %2").arg(labels.size()).arg(labels.join(", ")));
    const int cores = QThread::idealThreadCount();
    m_jobsHint->setText(m_jobs->value() == 0 ? tr("0 means one less than the cores: %1").arg(cores > 1 ? cores - 1 : 1)
                                             : tr("of %1 cores").arg(cores));
}

QString PlanEditor::describeProject(const QString & path)
{
    if (path.trimmed().isEmpty()) {
        return QString();
    }
    try {
        ProjectController project;
        project.load(path.toStdString());
        QStringList parts;
        parts << (project.plant() != nullptr ? QObject::tr("plant '%1'").arg(QString::fromStdString(project.plant()->name()))
                                             : QObject::tr("NO plant"));
        parts << (project.omega() != nullptr ? QObject::tr("%1 design frequencies").arg(project.omega()->values()->size())
                                             : QObject::tr("NO design frequencies"));
        parts << (!project.templates().empty() ? QObject::tr("templates") : QObject::tr("NO templates"));
        parts << (project.boundaries() != nullptr ? QObject::tr("boundaries") : QObject::tr("NO boundaries"));
        LtiSystem * controller = project.controllerStructure();
        if (controller == nullptr) {
            parts << QObject::tr("NO controller structure");
        } else {
            parts << QObject::tr("controller with %1 zeros and %2 poles, gain %3 to %4")
                         .arg(controller->numerator().size()).arg(controller->denominator().size())
                         .arg(controller->gain().range().min).arg(controller->gain().range().max);
        }
        return parts.join(", ");
    } catch (const std::exception & failure) {
        return QObject::tr("Cannot load it: %1").arg(QString::fromUtf8(failure.what()));
    }
}

void PlanEditor::refreshProjectSummary()
{
    m_projectSummary->setText(describeProject(m_project->text()));
}

void PlanEditor::addStep(bench::StructureStep::Kind kind)
{
    const int row = m_steps->rowCount();
    m_steps->insertRow(row);
    auto * kinds = new QComboBox(m_steps);
    kinds->addItems({tr("zero"), tr("pole")});
    kinds->setCurrentIndex(kind == bench::StructureStep::Kind::Zero ? 0 : 1);
    connect(kinds, &QComboBox::currentIndexChanged, this, [this]() { changed(); });
    m_steps->setCellWidget(row, kColumnKind, kinds);
    m_steps->setItem(row, kColumnMin, new QTableWidgetItem("0.01"));
    m_steps->setItem(row, kColumnMax, new QTableWidgetItem("1000"));
    auto * run = new QTableWidgetItem();
    run->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    run->setCheckState(Qt::Checked);
    m_steps->setItem(row, kColumnRun, run);
    m_steps->selectRow(row);
    changed();
}

void PlanEditor::removeStep()
{
    const int row = m_steps->currentRow();
    if (row >= 0) {
        m_steps->removeRow(row);
        changed();
    }
}

void PlanEditor::moveStep(int by)
{
    const int row = m_steps->currentRow();
    const int target = row + by;
    if (row < 0 || target < 0 || target >= m_steps->rowCount()) {
        return;
    }
    //The rows as plan steps, swapped, and written back.
    bench::Plan current = plan();
    std::swap(current.steps[static_cast<std::size_t>(row)], current.steps[static_cast<std::size_t>(target)]);
    setPlan(current);
    m_steps->selectRow(target);
    changed();
}

void PlanEditor::addSetting()
{
    const int row = m_settings->rowCount();
    m_settings->insertRow(row);
    auto * key = new QLineEdit(m_settings);
    QStringList keys;
    for (const std::string & name : settingKeys()) {
        keys << QString::fromStdString(name);
    }
    auto * completer = new QCompleter(keys, key);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    key->setCompleter(completer);
    connect(key, &QLineEdit::textChanged, this, [this]() { changed(); });
    m_settings->setCellWidget(row, 0, key);
    m_settings->setItem(row, 1, new QTableWidgetItem(QString()));
    m_settings->selectRow(row);
    key->setFocus();
    changed();
}

void PlanEditor::removeSetting()
{
    const int row = m_settings->currentRow();
    if (row >= 0) {
        m_settings->removeRow(row);
        changed();
    }
}

bench::Plan PlanEditor::plan() const
{
    bench::Plan plan;
    plan.name = m_name->text().trimmed().toStdString();
    if (plan.name.empty()) {
        throw InvalidInput("The plan needs a name: it names its result files.");
    }
    plan.projectFile = m_project->text().trimmed().toStdString();
    if (plan.projectFile.empty()) {
        throw InvalidInput("The plan needs a project file.");
    }
    plan.outputDirectory = m_output->text().trimmed().toStdString();
    if (plan.outputDirectory.empty()) {
        plan.outputDirectory = ".";
    }

    plan.runBase = m_runBase->isChecked();
    if (m_overrideGain->isChecked()) {
        plan.gainRange = Range(numberOf(m_gainMin, tr("Gain minimum")), numberOf(m_gainMax, tr("Gain maximum")));
    }
    for (int row = 0; row < m_steps->rowCount(); ++row) {
        bench::StructureStep step;
        auto * kind = qobject_cast<QComboBox *>(m_steps->cellWidget(row, kColumnKind));
        step.kind = kind != nullptr && kind->currentIndex() == 1 ? bench::StructureStep::Kind::Pole
                                                                  : bench::StructureStep::Kind::Zero;
        const auto cell = [&](int column, const QString & name) {
            QTableWidgetItem * item = m_steps->item(row, column);
            const std::optional<double> value = item != nullptr ? evaluateNumber(item->text()) : std::nullopt;
            if (!value) {
                throw InvalidInput(tr("Step %1: %2 is not a number").arg(row + 1).arg(name).toStdString());
            }
            return *value;
        };
        step.min = cell(kColumnMin, tr("the minimum"));
        step.max = cell(kColumnMax, tr("the maximum"));
        if (step.min > step.max) {
            std::swap(step.min, step.max);
        }
        QTableWidgetItem * run = m_steps->item(row, kColumnRun);
        step.run = run != nullptr && run->checkState() == Qt::Checked;
        plan.steps.push_back(step);
    }
    if (bench::measuredStructures(plan).empty()) {
        throw InvalidInput("No structure runs: mark the base or at least one step.");
    }

    for (const auto & [algorithm, box] : m_algorithms) {
        if (box->isChecked()) {
            plan.algorithms.push_back(algorithm);
        }
    }
    if (plan.algorithms.empty()) {
        throw InvalidInput("Choose at least one algorithm.");
    }
    for (const QString & piece : m_epsilons->text().split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts)) {
        const std::optional<double> value = evaluateNumber(piece);
        if (!value || *value <= 0.0) {
            throw InvalidInput(tr("Epsilons: '%1' is not a positive number").arg(piece).toStdString());
        }
        plan.epsilons.push_back(*value);
    }
    if (plan.epsilons.empty()) {
        throw InvalidInput("Give at least one epsilon.");
    }
    plan.repetitions = m_repetitions->value();
    plan.warmUp = m_warmUp->isChecked();

    plan.measures.time = m_measureTime->isChecked();
    plan.measures.cpu = m_measureCpu->isChecked();
    plan.measures.memory = m_measureMemory->isChecked();
    plan.measures.memoryTrace = m_measureTrace->isChecked();
    plan.measures.counters = m_measureCounters->isChecked();

    plan.jobs = m_jobs->value();
    plan.timeoutSeconds = m_timeout->value();
    plan.memoryLimitMegabytes = m_memoryLimit->value();

    for (int row = 0; row < m_settings->rowCount(); ++row) {
        auto * key = qobject_cast<QLineEdit *>(m_settings->cellWidget(row, 0));
        QTableWidgetItem * value = m_settings->item(row, 1);
        const QString keyText = key != nullptr ? key->text().trimmed() : QString();
        const QString valueText = value != nullptr ? value->text().trimmed() : QString();
        if (keyText.isEmpty() && valueText.isEmpty()) {
            continue;
        }
        if (keyText.isEmpty() || valueText.isEmpty()) {
            throw InvalidInput(tr("Setting %1 needs both a key and a value").arg(row + 1).toStdString());
        }
        plan.settings.emplace_back(keyText.toStdString(), valueText.toStdString());
    }
    return plan;
}

void PlanEditor::setPlan(const bench::Plan & plan)
{
    m_loading = true;
    m_name->setText(QString::fromStdString(plan.name));
    m_project->setText(QString::fromStdString(plan.projectFile));
    m_output->setText(QString::fromStdString(plan.outputDirectory));

    m_runBase->setChecked(plan.runBase);
    m_overrideGain->setChecked(plan.gainRange.has_value());
    m_gainMin->setText(plan.gainRange ? text(plan.gainRange->min) : QString());
    m_gainMax->setText(plan.gainRange ? text(plan.gainRange->max) : QString());
    m_steps->setRowCount(0);
    for (const bench::StructureStep & step : plan.steps) {
        addStep(step.kind);
        const int row = m_steps->rowCount() - 1;
        m_steps->item(row, kColumnMin)->setText(text(step.min));
        m_steps->item(row, kColumnMax)->setText(text(step.max));
        m_steps->item(row, kColumnRun)->setCheckState(step.run ? Qt::Checked : Qt::Unchecked);
    }

    for (auto & [algorithm, box] : m_algorithms) {
        box->setChecked(std::find(plan.algorithms.begin(), plan.algorithms.end(), algorithm) != plan.algorithms.end());
    }
    QStringList epsilons;
    for (const double epsilon : plan.epsilons) {
        epsilons << text(epsilon);
    }
    m_epsilons->setText(epsilons.join(", "));
    m_repetitions->setValue(plan.repetitions);
    m_warmUp->setChecked(plan.warmUp);

    m_measureTime->setChecked(plan.measures.time);
    m_measureCpu->setChecked(plan.measures.cpu);
    m_measureMemory->setChecked(plan.measures.memory);
    m_measureTrace->setChecked(plan.measures.memoryTrace);
    m_measureCounters->setChecked(plan.measures.counters);

    m_jobs->setValue(plan.jobs);
    m_timeout->setValue(static_cast<int>(plan.timeoutSeconds));
    m_memoryLimit->setValue(plan.memoryLimitMegabytes);

    m_settings->setRowCount(0);
    for (const auto & [key, value] : plan.settings) {
        addSetting();
        const int row = m_settings->rowCount() - 1;
        qobject_cast<QLineEdit *>(m_settings->cellWidget(row, 0))->setText(QString::fromStdString(key));
        m_settings->item(row, 1)->setText(QString::fromStdString(value));
    }
    m_loading = false;
    refreshProjectSummary();
    changed();
}

} // namespace qftbx
