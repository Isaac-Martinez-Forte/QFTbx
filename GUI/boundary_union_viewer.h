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


    void setDatos (QVector< QVector<QPointF> * > * boun, QVector<qreal> *omega);
    void setDatos (QVector< QVector<QPointF> * > * boun, QVector<qreal> *omega, qint32 unBoundarie);

    void setDatos (QVector< QVector< QVector<QPointF> * > * > * boun, QVector<qreal> *omega);

    void setDatos (QVector< QVector< QVector<QPointF> * > * > * boun, QVector<qreal> *omega, QVector<QPointF> * b);

    void mostrar_diagrama();

    void dibujar_cuadro (QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, qint32 contador);
    void dibujar_cuadro2 (QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, qint32 contador);

private slots:
    void revisarCheckBox();

    void on_guardar_clicked();

private:

    QVector< QVector<QPointF> * > * boun;
    QVector< QVector< QVector<QPointF> * > * > * bounHash;
    QVector <qreal> * omega;
    QVector<QPointF> * b = nullptr;

    bool hash;

    bool ejecutado;

    QVector <QCPCurve * > * graficos;
    QVector <QCPCurve * > * graficos2;
    QVector <QCPCurve * > * graficos3;
    QGroupBox * cajaFrecuencias;
    QVector <QCheckBox *> * checkbox;
    QMap <QString, QColor> * colores;
    QVBoxLayout * layoutColores;

    void pintarCuadro(QColor color, qint32 pos);
    void clearDiagram();


    Ui::BoundaryUnionViewer *ui;

    qint32 finalk;

    QVector <QColor> * col;

    qint32 unBoundarie = -1;
};

#endif // QFTBX_BOUNDARY_UNION_VIEWER_H
