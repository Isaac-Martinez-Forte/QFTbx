#ifndef VERBOUNDARIES_H
#define VERBOUNDARIES_H

#include <QDialog>

//#include "cinterval.hpp"

#include "Modelo/Herramientas/tools.h"
#include "Librerias/qcustomplot/qcustomplot.h"

#include "Modelo/EstructurasDatos/datosbound.h"

#include "Modelo/EstructuraSistema/sistema.h"
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


    void setDatos (std::shared_ptr<DatosBound> boundariesNichols, std::shared_ptr<DatosBound> boundariesNyquist, QVector<qreal> *omega,
                   std::shared_ptr<Sistema> planta, std::shared_ptr<Sistema> controlador, bool nichols, bool nyquist);

    void mostrar_diagrama();

    void dibujar_cuadro (QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, QColor color);

private slots:
    void revisarCheckBox();

    void on_guardar_clicked();

private:

    std::shared_ptr<DatosBound> boundariesNichols;
    std::shared_ptr<DatosBound> boundariesNyquist;
    std::shared_ptr<Sistema> planta;
    std::shared_ptr<Sistema> controlador;
    QVector <qreal> * omega;

    bool ejecutado;

    QVector <QCPCurve * > * graficos;
    QGroupBox * cajaFrecuencias;
    QVector <QCheckBox *> * checkbox;
    QMap <QString, QColor> * colores;
    QVBoxLayout * layoutColores;

    void pintarCuadro(QColor color, qint32 pos);

    bool nichols;
    bool nyquist;

    Ui::verBoundaries *ui;

    qint32 finalk;

};

#endif // VIEWBOUNDREUN_H
