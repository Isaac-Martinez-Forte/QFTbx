#ifndef QFTBX_LOOP_SHAPING_VIEWER_H
#define QFTBX_LOOP_SHAPING_VIEWER_H

#include <QDialog>

#include "Modelo/Herramientas/tools.h"
#include "qcustomplot.h"
#include "src/core/system/lti_system.h"
#include "Modelo/EstructurasDatos/datosloopshaping.h"


namespace Ui {
class LoopShapingViewer;
}

class LoopShapingViewer : public QDialog
{
    Q_OBJECT

public:
    explicit LoopShapingViewer(QWidget *parent = 0);
    ~LoopShapingViewer();


    void setDatos (QVector<QVector<QPointF> *> *boun, QVector<qreal> *omega, DatosLoopShaping * datos, LtiSystem *planta, bool linSpace);

    void mostrar_diagrama();

private slots:
    void revisarCheckBox();


    void on_guardar_clicked();

private:

    QVector <QVector <QPointF> * > * boun;
    QVector <qreal> * omega;
    LtiSystem * planta;
    DatosLoopShaping * datos;

    bool ejecutado;

    QVector <QCPCurve * > * graficos;
    QGroupBox * cajaFrecuencias;
    QVector <QCheckBox *> * checkbox;
    QMap <QString, QColor> * colores;
    QVBoxLayout * layoutColores;

    void pintarCuadro(QColor color, qint32 pos);
    void clearDiagram();

    bool linSpace;

    Ui::LoopShapingViewer *ui;
};

#endif // QFTBX_LOOP_SHAPING_VIEWER_H
