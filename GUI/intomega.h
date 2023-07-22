#ifndef INTOMEGA_H
#define INTOMEGA_H

#include <QDialog>
#include <QString>
#include <QFileDialog>
#include <QDoubleValidator>
#include <QVector>
#include <QMessageBox>
#include <QTextStream>

#include "Modelo/controlador.h"
#include "Modelo/Herramientas/tools.h"
#include "Modelo/Objetos/omega.h"

namespace Ui {
class IntOmega;
}

/**
    * @class IntOmega
    * @brief Clase gráfica a través de la cual se puede introducir las frecuencias de diseño (Omega) para guardarlas en el sistema.
    * 
    * @author Isaac Martínez Forte
   */

class IntOmega : public QDialog
{
    Q_OBJECT
    
public:
  
  /**
    * @fn IntOmega
    * @brief Constructor de la clase, que solo tiene por parámetro el padre de dicha clase.
    * 
    * @param parent padre de la clase a crear, puede se vacío.
    */
  
    explicit IntOmega(QWidget *parent = 0);
    
  /**
    * @fn IntOmega
    * @brief Constructor de la clase, que además tiene por parámetros el controlador del sistema, necesario para poder interaccionar con la lógica del sistema.
    * 
    * @param parent padre de la clase a crear, puede ser vacío.
    * @param controlador del sistema.
    */
  
    explicit IntOmega(Controlador * controlador, QWidget *parent = 0);
    ~IntOmega();


    bool getTodoCorrecto();
    
private slots:

    void on_buttonFic_clicked();

    void on_ok_clicked();

signals:
    void close_ok ();

private:
    Controlador * controlador;
    bool fallo;
    QString file;

    QVector <qreal> * fichero(QString ruta);
    Ui::IntOmega *ui;

    bool todoCorrecto;
};

#endif // INTOMEGA_H
