/**
 * @file
 * @brief Plots the union of the QFT boundaries, one curve per design frequency.
 *
 * Declares the viewer of the single set of bounds the loop shaping has to
 * respect. It draws what it is handed and observes the design frequencies;
 * when the project drops the step it is emptied rather than destroyed.
 * The curves belong to the plot, which frees them; the viewer keeps only
 * the containers that tie each frequency's pieces to its legend row.
 */

#ifndef QFTBX_BOUNDARY_UNION_VIEWER_H
#define QFTBX_BOUNDARY_UNION_VIEWER_H

#include <vector>
#include <memory>

#include <QWidget>

#include "qcustomplot.h"
#include "src/gui/common/frequency_legend.h"
#include "src/core/boundaries/boundary_types.h"

namespace Ui {
class BoundaryUnionViewer;
}

namespace qftbx {

/**
 * @brief Plots the union of the QFT boundaries of every specification, one
 * curve per design frequency: the single set of bounds the loop shaping
 * actually has to respect.
 *
 */
class BoundaryUnionViewer : public QWidget
{
    Q_OBJECT

public:
    explicit BoundaryUnionViewer(QWidget *parent = nullptr);
    ~BoundaryUnionViewer();

    /// Publishes what the plot draws: the union curves and the design
    /// frequencies they belong to (an observer on the latter).
    void setData (const qftbx::UnionTraces & unionTraces, std::vector<double> *omega);

    /**
     * @brief Forgets what it was drawing and empties the plot.
     *
     * A viewer lives in its phase's dock for as long as the window does,
     * and what it holds are observers on the project: when the project
     * drops a step, the pointers behind them go with it. This is what is
     * called then, instead of destroying the viewer.
     */
    void clear();

    void showDiagram();

private slots:
    void applyCheckboxes();

    void on_saveImage_clicked();

private:
    qftbx::UnionTraces unionTraces;
    std::vector<double> * omega = nullptr;

    bool plotted = false;

    /// One entry per frequency, and inside it one curve per piece of its
    /// boundary: what the legend shows or hides is the frequency, which is
    /// all of them.
    /// The curves BELONG TO QCustomPlot, which frees them on
    /// clearPlottables(): only the container is the viewer's.
    QVector <QVector <QCPCurve *>> curves;

    void addFrequencyRow(QColor color, qint32 pos);
    FrequencyLegend * legend = nullptr;
    void clearDiagram();

    std::unique_ptr<Ui::BoundaryUnionViewer> ui;
};

}

#endif
