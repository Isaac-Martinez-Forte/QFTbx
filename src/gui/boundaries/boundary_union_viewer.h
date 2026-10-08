/**
 * @file
 * @brief Plots the union of the QFT boundaries, one curve per design
 * frequency.
 *
 * Declares the viewer of the single set of bounds the loop shaping has to
 * respect. It draws what it is handed and observes the design frequencies;
 * when the project drops the step it is emptied rather than destroyed.
 * The curves belong to the plot, which frees them; the viewer keeps only
 * the containers that tie each frequency's pieces to its legend row, which
 * shows or hides all of them at once.
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

class BoundaryUnionViewer : public QWidget
{
    Q_OBJECT

public:
    explicit BoundaryUnionViewer(QWidget *parent = nullptr);
    ~BoundaryUnionViewer();

    void setData (const qftbx::UnionTraces & unionTraces, std::vector<double> *omega);

    void clear();

    void showDiagram();

private slots:
    void applyCheckboxes();

    void on_saveImage_clicked();

private:
    qftbx::UnionTraces unionTraces;
    std::vector<double> * omega = nullptr;

    bool plotted = false;

    QVector <QVector <QCPCurve *>> curves;

    void addFrequencyRow(QColor color, qint32 pos);
    FrequencyLegend * legend = nullptr;
    void clearDiagram();

    std::unique_ptr<Ui::BoundaryUnionViewer> ui;
};

}

#endif
