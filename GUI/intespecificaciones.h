#ifndef INTESPECIFICACIONES_H
#define INTESPECIFICACIONES_H

#include <QDialog>
#include <QPixmap>

#include "Modelo/Herramientas/tools.h"
#include "Modelo/controlador.h"
#include "muparserx/mpParser.h"
#include "Modelo/Objetos/omega.h"

using namespace tools;

namespace Ui {
class IntEspecificaciones;
}

class IntEspecificaciones : public QDialog
{
    Q_OBJECT

public:
    explicit IntEspecificaciones(Controlador * controlador, QWidget *parent = 0);
    ~IntEspecificaciones();


    bool getTodoCorrecto ();

private slots:
    void on_poli_clicked();

    void on_hfgain_clicked();

    void on_lfgain_clicked();

    void on_radioButton_clicked();

    void on_radioButton_2_clicked();

    void on_radioButton_3_clicked();

    void on_radioButton_4_clicked();

    void on_radioButton_5_clicked();

    void on_radioButton_6_clicked();

    void on_cons_clicked();

    void on_fun_clicked();

    void on_Cancel_clicked();

    void on_OK_clicked();

    void on_libre_clicked();

    void on_CPoliSe1_clicked();

    void on_FLSe1_clicked();

    void on_KGSe1_clicked();

    void on_KNGSe1_clicked();

    void on_CPoliSe1_2_clicked();

    void on_KGSe1_2_clicked();

    void on_KNGSe1_2_clicked();

    void on_FLSe1_2_clicked();

private:
    Ui::IntEspecificaciones *ui;

    dBND *seguimiento;
    dBND *seguimiento_2;
    dBND *estabilidad;
    dBND *ruido;
    dBND *RPS;
    dBND *RPE;
    dBND *EC;

    QVector <dBND *> * retorno;

    qint32 activado;

    bool getDatos(dBND * datos, QString nombre);
    bool getDatos(dBND *datos, dBND * datos1, QString nombre);
    void setDatos (dBND * datos);
    void setDatos (dBND * datos, dBND * datos1);
    void seleccionar();

    QVector <Var * > * crearNumeradorDenominador(QString linea);
    Var * crearKRet(QString linea, bool isK);

    Controlador * controlador;

    //imágenes
    QPixmap seguimiento_img;
    QPixmap EC_img;
    QPixmap RPS_img;
    QPixmap RPE_img;
    QPixmap ruidosensor_img;
    QPixmap estabilidad_img;

    QVector <qreal> * omega;

    bool todoCorrecto;
};


#endif // INTESPECIFICACIONES_H
