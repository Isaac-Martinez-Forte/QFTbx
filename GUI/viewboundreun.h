#ifndef VIEWBOUNDREUN_H
#define VIEWBOUNDREUN_H

#include <QDialog>

//#include "cinterval.hpp"

#include "Modelo/Herramientas/tools.h"
#include "Librerias/qcustomplot/qcustomplot.h"


namespace Ui {
class ViewBoundReun;
}

class ViewBoundReun : public QDialog
{
    Q_OBJECT

public:
    explicit ViewBoundReun(QWidget *parent = nullptr);
    ~ViewBoundReun();


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


    Ui::ViewBoundReun *ui;

    qint32 finalk;

    QVector <QColor> * col;

    qint32 unBoundarie = -1;
};

#endif // VIEWBOUNDREUN_H
