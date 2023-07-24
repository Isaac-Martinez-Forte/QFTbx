#ifndef VIEWLOOPSHAPING_H
#define VIEWLOOPSHAPING_H

#include <QDialog>

#include "Modelo/Herramientas/tools.h"
#include "Librerias/qcustomplot/qcustomplot.h"
#include "Modelo/EstructuraSistema/sistema.h"
#include "Modelo/EstructurasDatos/datosloopshaping.h"


namespace Ui {
class ViewLoopShaping;
}

class ViewLoopShaping : public QDialog
{
    Q_OBJECT

public:
    explicit ViewLoopShaping(QWidget *parent = 0);
    ~ViewLoopShaping();


    void setDatos (QVector<QVector<QPointF> *> *boun, QVector<qreal> *omega, std::shared_ptr<DatosLoopShaping> datos, std::shared_ptr<Sistema> planta, bool linSpace);

    void mostrar_diagrama();

private slots:
    void revisarCheckBox();


    void on_guardar_clicked();

private:

    QVector <QVector <QPointF> * > * boun;
    QVector <qreal> * omega;
    std::shared_ptr<Sistema> planta;
    std::shared_ptr<DatosLoopShaping> datos;

    bool ejecutado;

    QVector <QCPCurve * > * graficos;
    QGroupBox * cajaFrecuencias;
    QVector <QCheckBox *> * checkbox;
    QMap <QString, QColor> * colores;
    QVBoxLayout * layoutColores;

    void pintarCuadro(QColor color, qint32 pos);

    bool linSpace;

    Ui::ViewLoopShaping *ui;
};

#endif // VIEWLOOPSHAPING_H
