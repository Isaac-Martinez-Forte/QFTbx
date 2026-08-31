#ifndef QFTBX_LOOP_BOUNDARIES_VIEWER_H
#define QFTBX_LOOP_BOUNDARIES_VIEWER_H

#include <QDialog>

//#include "cinterval.hpp"

#include "Modelo/Herramientas/tools.h"
#include "qcustomplot.h"

#include "src/core/boundaries/boundary_data.h"

#include "src/core/system/lti_system.h"
#include "Modelo/LoopShaping/NaturalIntervalExtension/natural_interval_extension.h"


namespace Ui {
class LoopBoundariesViewer;
}

class LoopBoundariesViewer : public QDialog
{
    Q_OBJECT

public:
    explicit LoopBoundariesViewer(QWidget *parent = 0);
    ~LoopBoundariesViewer();


    void setDatos (BoundaryData * boundariesNichols, BoundaryData * boundariesNyquist, QVector<qreal> *omega,
                   LtiSystem * planta, LtiSystem * controlador, bool nichols, bool nyquist);

    void mostrar_diagrama();

    void dibujar_cuadro (QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, QColor color);

private slots:
    void revisarCheckBox();

    void on_guardar_clicked();

private:

    BoundaryData * boundariesNichols;
    BoundaryData * boundariesNyquist;
    LtiSystem * planta;
    LtiSystem * controlador;
    QVector <qreal> * omega;

    bool ejecutado;

    QVector <QCPCurve * > * graficos;
    QGroupBox * cajaFrecuencias;
    QVector <QCheckBox *> * checkbox;
    QMap <QString, QColor> * colores;
    QVBoxLayout * layoutColores;

    void pintarCuadro(QColor color, qint32 pos);
    void clearDiagram();

    bool nichols;
    bool nyquist;

    Ui::LoopBoundariesViewer *ui;

    qint32 finalk;

};

#endif // QFTBX_BOUNDARY_UNION_VIEWER_H
