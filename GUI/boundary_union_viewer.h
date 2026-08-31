#ifndef QFTBX_BOUNDARY_UNION_VIEWER_H
#define QFTBX_BOUNDARY_UNION_VIEWER_H

#include <QDialog>

//#include "cinterval.hpp"

#include "Modelo/Herramientas/tools.h"
#include "qcustomplot.h"


namespace Ui {
class BoundaryUnionViewer;
}

class BoundaryUnionViewer : public QDialog
{
    Q_OBJECT

public:
    explicit BoundaryUnionViewer(QWidget *parent = nullptr);
    ~BoundaryUnionViewer();


    void setDatos (QVector< QVector<QPointF> * > * unionTraces, QVector<qreal> *omega);
    void setDatos (QVector< QVector<QPointF> * > * unionTraces, QVector<qreal> *omega, qint32 singleBoundary);

    void setDatos (QVector< QVector< QVector<QPointF> * > * > * unionTraces, QVector<qreal> *omega);

    void setDatos (QVector< QVector< QVector<QPointF> * > * > * unionTraces, QVector<qreal> *omega, QVector<QPointF> * b);

    void showDiagram();

    void drawBox (QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, qint32 contador);
    void drawBox2 (QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, qint32 contador);

private slots:
    void applyCheckboxes();

    void on_saveImage_clicked();

private:

    QVector< QVector<QPointF> * > * unionTraces;
    QVector< QVector< QVector<QPointF> * > * > * unionBuckets;
    QVector <qreal> * omega;
    QVector<QPointF> * b = nullptr;

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
