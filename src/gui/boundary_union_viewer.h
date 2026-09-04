#ifndef QFTBX_BOUNDARY_UNION_VIEWER_H
#define QFTBX_BOUNDARY_UNION_VIEWER_H

#include <vector>
#include <memory>

#include <QDialog>

#include "qcustomplot.h"
#include "src/core/boundaries/boundary_types.h"

namespace Ui {
class BoundaryUnionViewer;
}

/**
 * @brief Plots the union of the QFT boundaries of every specification, one
 * curve per design frequency: the single set of bounds the loop shaping
 * actually has to respect.
 *
 * It used to carry three more modes (a single frequency, the bucketed union,
 * the bucketed union plus one extra trace) and two box-drawing routines,
 * none of which anything called.
 */
class BoundaryUnionViewer : public QDialog
{
    Q_OBJECT

public:
    explicit BoundaryUnionViewer(QWidget *parent = nullptr);
    ~BoundaryUnionViewer();

    /// Publishes what the plot draws: the union curves and the design
    /// frequencies they belong to (an observer on the latter).
    void setData (const qftbx::UnionTraces & unionTraces, std::vector<double> *omega);

    void showDiagram();

private slots:
    void applyCheckboxes();

    void on_saveImage_clicked();

private:
    qftbx::UnionTraces unionTraces;
    std::vector<double> * omega = nullptr;

    bool plotted = false;

    //The curves BELONG TO QCustomPlot, which frees them on
    //clearPlottables(): only the container is the viewer's.
    QVector <QCPCurve *> curves;
    QGroupBox * frequenciesBox = nullptr;
    //The checkboxes belong to their row widget: the viewer deletes the
    //rows, not these.
    QVector <QCheckBox *> checkboxes;
    QVBoxLayout * colorsLayout = nullptr;

    void addFrequencyRow(QColor color, qint32 pos);
    void clearDiagram();

    std::unique_ptr<Ui::BoundaryUnionViewer> ui;
};

#endif // QFTBX_BOUNDARY_UNION_VIEWER_H
