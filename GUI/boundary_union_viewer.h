#ifndef QFTBX_BOUNDARY_UNION_VIEWER_H
#define QFTBX_BOUNDARY_UNION_VIEWER_H

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
    QVector <qreal> * omega;
    //An extra curve painted on top, empty when there is none. It was a
    //pointer whose nullness was the flag.
    qftbx::Trace b;

    bool bucketMode;

    bool plotted;

    QVector <QCPCurve * > * curves;
    QVector <QCPCurve * > * boxCurves;
    QVector <QCPCurve * > * boxCurves2;
    QGroupBox * frequenciesBox;
    QVector <QCheckBox *> * checkboxes;
    QMap <QString, QColor> * colores;
    QVBoxLayout * colorsLayout;

    void addFrequencyRow(QColor color, qint32 pos);
    void clearDiagram();


    Ui::BoundaryUnionViewer *ui;

    qint32 finalCurveIndex;

    QVector <QColor> * colors;

    qint32 singleBoundary = -1;
};

#endif // QFTBX_BOUNDARY_UNION_VIEWER_H
