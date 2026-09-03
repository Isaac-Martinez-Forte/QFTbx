#ifndef QFTBX_BOUNDARY_UNION_VIEWER_H
#define QFTBX_BOUNDARY_UNION_VIEWER_H

#include <vector>
#include <memory>

#include <QDialog>

//#include "cinterval.hpp"

#include "src/core/math/sequence_vectors.h"
#include "qcustomplot.h"
#include "src/core/boundaries/boundary_types.h"


namespace Ui {
class BoundaryUnionViewer;
}

/**
 * @brief Plots the union of the QFT boundaries of every specification, one
 * curve per design frequency: the single set of bounds the loop shaping
 * actually has to respect.
 */
class BoundaryUnionViewer : public QDialog
{
    Q_OBJECT

public:
    explicit BoundaryUnionViewer(QWidget *parent = nullptr);
    ~BoundaryUnionViewer();


    void setData (const qftbx::UnionTraces & unionTraces, std::vector<double> *omega);
    void setData (const qftbx::UnionTraces & unionTraces, std::vector<double> *omega, qint32 singleBoundary);

    void setData (const qftbx::UnionBuckets & unionTraces, std::vector<double> *omega);

    void setData (const qftbx::UnionBuckets & unionTraces, std::vector<double> *omega, const qftbx::Trace & b);

    void showDiagram();

    void drawBox (QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, qint32 frequencyIndex);
    void drawBox2 (QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, qint32 frequencyIndex);

private slots:
    void applyCheckboxes();

    void on_saveImage_clicked();

private:

    qftbx::UnionTraces unionTraces;
    qftbx::UnionBuckets unionBuckets;
    std::vector<double> * omega = nullptr;
    //An extra curve painted on top, empty when there is none. It was a
    //pointer whose nullness was the flag.
    qftbx::Trace b;

    bool bucketMode = false;

    bool plotted = false;

    //The curves BELONG TO QCustomPlot, which frees them on
    //clearPlottables(): only these containers are the viewer's, and
    //keeping them across a replot would leave dangling observers.
    QVector <QCPCurve *> curves;
    QVector <QCPCurve *> boxCurves;
    QVector <QCPCurve *> boxCurves2;
    QGroupBox * frequenciesBox = nullptr;
    //The checkboxes belong to their row widget: the viewer deletes the
    //rows, not these.
    QVector <QCheckBox *> checkboxes;
    QVBoxLayout * colorsLayout = nullptr;

    void addFrequencyRow(QColor color, qint32 pos);
    void clearDiagram();


    std::unique_ptr<Ui::BoundaryUnionViewer> ui;

    qint32 finalCurveIndex = 0;

    QVector <QColor> colors;

    qint32 singleBoundary = -1;
};

#endif // QFTBX_BOUNDARY_UNION_VIEWER_H
