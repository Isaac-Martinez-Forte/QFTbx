#ifndef QFTBX_GUI_BENCH_PLAN_EDITOR_H
#define QFTBX_GUI_BENCH_PLAN_EDITOR_H

#include <functional>
#include <vector>

#include <QWidget>

#include "src/bench/plan.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QTableWidget;
class QPushButton;

namespace qftbx {

/**
 * @brief The form of a benchmark plan: every field of bench::Plan, laid
 * out the way a measurement is thought through.
 *
 * Project first, then the structures to grow, then what to run on them,
 * how often, what to measure, and how to execute. The editor reads a Plan
 * and gives one back; it does not run anything. The paths it holds are
 * absolute; the window makes them relative to the plan file when it saves.
 */
class PlanEditor : public QWidget
{
    Q_OBJECT

public:
    explicit PlanEditor(QWidget * parent = nullptr);

    /// The form as a plan. Throws qftbx::InvalidInput naming the field when
    /// something cannot be read.
    bench::Plan plan() const;
    void setPlan(const bench::Plan & plan);

    /// Called after every edit, for the window's case count.
    void setChangeHandler(std::function<void ()> handler) { m_changed = std::move(handler); }

    /// How a file or directory gets asked for (the file dialogs are modal
    /// and cannot be driven by a test): forSaving and a filter, returning
    /// the path or an empty string.
    using FileChooser = std::function<QString (bool forSaving, bool directory, const QString & filter)>;
    void setFileChooser(FileChooser chooser) { m_chooseFile = std::move(chooser); }

    /// What the project at the given path holds, for the summary line;
    /// the message of the failure when it cannot be loaded.
    static QString describeProject(const QString & path);

private:
    void build();
    void changed();
    void refreshStructures();
    void refreshProjectSummary();
    void addStep(bench::StructureStep::Kind kind);
    void removeStep();
    void moveStep(int by);
    void addSetting();
    void removeSetting();
    QString chooseFile(bool forSaving, bool directory, const QString & filter);

    QLineEdit * m_project = nullptr;
    QLabel * m_projectSummary = nullptr;
    QLineEdit * m_name = nullptr;
    QLineEdit * m_output = nullptr;

    QCheckBox * m_runBase = nullptr;
    QCheckBox * m_overrideGain = nullptr;
    QLineEdit * m_gainMin = nullptr;
    QLineEdit * m_gainMax = nullptr;
    QTableWidget * m_steps = nullptr;
    QLabel * m_structures = nullptr;

    std::vector<std::pair<LoopShapingAlgorithm, QCheckBox *>> m_algorithms;
    QLineEdit * m_epsilons = nullptr;
    QSpinBox * m_repetitions = nullptr;
    QCheckBox * m_warmUp = nullptr;

    QCheckBox * m_measureTime = nullptr;
    QCheckBox * m_measureCpu = nullptr;
    QCheckBox * m_measureMemory = nullptr;
    QCheckBox * m_measureTrace = nullptr;
    QCheckBox * m_measureCounters = nullptr;

    QSpinBox * m_jobs = nullptr;
    QLabel * m_jobsHint = nullptr;
    QSpinBox * m_timeout = nullptr;
    QSpinBox * m_memoryLimit = nullptr;

    QTableWidget * m_settings = nullptr;

    std::function<void ()> m_changed;
    FileChooser m_chooseFile;
    bool m_loading = false;
};

} // namespace qftbx

#endif // QFTBX_GUI_BENCH_PLAN_EDITOR_H
