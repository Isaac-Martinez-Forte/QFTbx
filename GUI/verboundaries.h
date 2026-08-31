#ifndef VERBOUNDARIES_H
#define VERBOUNDARIES_H

#include <QDialog>

//#include "cinterval.hpp"

#include "Modelo/Herramientas/tools.h"
#include "qcustomplot.h"

#include "src/core/boundaries/boundary_data.h"

#include "src/core/system/lti_system.h"
#include "Modelo/LoopShaping/NaturalIntervalExtension/natural_interval_extension.h"


namespace Ui {
class verBoundaries;
}

class verBoundaries : public QDialog
{
    Q_OBJECT

public:
    explicit verBoundaries(QWidget *parent = 0);
    ~verBoundaries();


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

    Ui::verBoundaries *ui;

    qint32 finalk;

};

#endif // VIEWBOUNDREUN_H
