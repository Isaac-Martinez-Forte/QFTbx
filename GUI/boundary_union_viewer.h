#ifndef QFTBX_BOUNDARY_UNION_VIEWER_H
#define QFTBX_BOUNDARY_UNION_VIEWER_H

#include <memory>

#include <QDialog>

//#include "cinterval.hpp"

#include "src/core/math/sequence_vectors.h"
#include "qcustomplot.h"
#include "src/core/boundaries/boundary_types.h"


namespace Ui {
class BoundaryUnionViewer;
}

class BoundaryUnionViewer : public QDialog
{
    Q_OBJECT

public:
    explicit BoundaryUnionViewer(QWidget *parent = nullptr);
    ~BoundaryUnionViewer();


    void setDatos (const qftbx::UnionTraces & unionTraces, QVector<qreal> *omega);
    void setDatos (const qftbx::UnionTraces & unionTraces, QVector<qreal> *omega, qint32 singleBoundary);

    void setDatos (const qftbx::UnionBuckets & unionTraces, QVector<qreal> *omega);

    void setDatos (const qftbx::UnionBuckets & unionTraces, QVector<qreal> *omega, const qftbx::Trace & b);

    void showDiagram();

    void drawBox (QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, qint32 contador);
    void drawBox2 (QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, qint32 contador);

private slots:
    void applyCheckboxes();

    void on_saveImage_clicked();

private:

    qftbx::UnionTraces unionTraces;
    qftbx::UnionBuckets unionBuckets;
    QVector <qreal> * omega = nullptr;
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
    QMap <QString, QColor> * colores;
    QVBoxLayout * colorsLayout = nullptr;

    void addFrequencyRow(QColor color, qint32 pos);
    void clearDiagram();


    std::unique_ptr<Ui::BoundaryUnionViewer> ui;

    qint32 finalCurveIndex = 0;

    QVector <QColor> colors;

    qint32 singleBoundary = -1;
};

#endif // QFTBX_BOUNDARY_UNION_VIEWER_H
