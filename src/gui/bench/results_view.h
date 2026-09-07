#ifndef QFTBX_GUI_BENCH_RESULTS_VIEW_H
#define QFTBX_GUI_BENCH_RESULTS_VIEW_H

#include <functional>
#include <vector>

#include <QWidget>

#include "src/bench/summary.h"

class QComboBox;
class QCheckBox;
class QCustomPlot;
class QLabel;
class QTableWidget;

namespace qftbx {

/**
 * @brief The summary of a run: the statistics per case as a table, and a
 * figure of one measure against the controller structure, one curve per
 * algorithm, the way the thesis draws its comparisons.
 */
class ResultsView : public QWidget
{
    Q_OBJECT

public:
    explicit ResultsView(QWidget * parent = nullptr);

    void show(const std::vector<bench::Aggregate> & aggregates);
    void clear();

    /// How a file name gets asked for when exporting; see PlanEditor.
    using FileChooser = std::function<QString (bool forSaving, const QString & filter)>;
    void setFileChooser(FileChooser chooser) { m_chooseFile = std::move(chooser); }

    int rowCount() const;

private:
    void fillTable();
    void draw();
    void exportCsv();
    void exportMarkdown();
    QString chooseFile(const QString & filter);

    std::vector<bench::Aggregate> m_aggregates;
    QTableWidget * m_table = nullptr;
    QComboBox * m_measure = nullptr;
    QCheckBox * m_logarithmic = nullptr;
    QCustomPlot * m_plot = nullptr;
    QLabel * m_note = nullptr;
    FileChooser m_chooseFile;
};

} // namespace qftbx

#endif // QFTBX_GUI_BENCH_RESULTS_VIEW_H
