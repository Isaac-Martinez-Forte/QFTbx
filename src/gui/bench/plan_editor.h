/**
 * @file
 * @brief The form of a benchmark plan.
 *
 * Declares the widget that shows every field of a plan and reads one back,
 * laid out the way a measurement is thought through: the project and the
 * output, the controller structures to grow, the algorithms, epsilons and
 * repetitions to run, what to measure, how to execute, and the settings
 * the runs override. Reading the form back throws InvalidInput naming the
 * field that cannot be read. It runs nothing and holds absolute paths; the
 * window makes them relative to the plan file when it saves. A handler
 * called after every edit lets the window count cases, and a file chooser
 * seam, answering a path or an empty string, lets tests avoid the modal
 * file dialogs. describeProject sums up a project file for the summary
 * line, or gives the message of the failure to load it.
 */

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

class PlanEditor : public QWidget
{
    Q_OBJECT

public:
    explicit PlanEditor(QWidget * parent = nullptr);

    bench::Plan plan() const;
    void setPlan(const bench::Plan & plan);

    void setChangeHandler(std::function<void ()> handler) { m_changed = std::move(handler); }

    using FileChooser = std::function<QString (bool forSaving, bool directory, const QString & filter)>;
    void setFileChooser(FileChooser chooser) { m_chooseFile = std::move(chooser); }

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

}

#endif
