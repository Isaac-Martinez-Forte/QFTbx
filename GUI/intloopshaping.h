#ifndef INTLOOPSHAPING_H
#define INTLOOPSHAPING_H

#include <QDialog>

#include "muparserx/mpParser.h"
#include "Modelo/Herramientas/tools.h"
#include "Modelo/controlador.h"
#include "Modelo/Herramientas/tools.h"

namespace Ui {  
class IntLoopShaping;
}

class IntLoopShaping : public QDialog
{
    Q_OBJECT

public:
    explicit IntLoopShaping(QWidget *parent = 0);
    ~IntLoopShaping();

    void setDatos (qreal epsilon);

    bool getTodoCorrecto();

    qreal getEpsilon ();

    tools::alg_loop_shaping getAlg();

    QPointF getRango();

    qreal getNPuntos();

    qreal getDelta ();

    bool getLinLogSpace();

    bool getDepuracion();

    qint32 getInicializacion ();

    bool getHilos();
    
    bool getBisectionAvanced();
    bool getDeteccionAvanced();
    bool getAcelerated();

private slots:
    void on_cancel_clicked();

    void on_ok_clicked();

    void on_linspace_clicked();

    void on_logspace_clicked();

    void on_sachin_clicked();

    void on_nand_clicked();

    void on_ram_clicked();

    void on_isaac_clicked();

private:
    Ui::IntLoopShaping *ui;

    bool todoCorrecto;

    qreal epsilon;

    QPointF rango;

    qreal nPuntos;

    bool hilos;
    
    qreal delta;
    qint32 inicializacion;

    tools::alg_loop_shaping alg;

    bool linLogSpace;
    bool depuracion;
};

#endif // INTLOOPSHAPING_H
