#ifndef QFTBX_FREQUENCIES_DIALOG_H
#define QFTBX_FREQUENCIES_DIALOG_H

#include <QDialog>
#include <QString>
#include <QFileDialog>
#include <QDoubleValidator>
#include <QVector>
#include <QMessageBox>
#include <QTextStream>

#include "Modelo/controlador.h"
#include "Modelo/Herramientas/tools.h"
#include "src/core/frequencies/omega.h"

namespace Ui {
class FrequenciesDialog;
}

/**
    * @class FrequenciesDialog
    * @brief Clase gráfica a través de la cual se puede introducir las frequencies de diseño (Omega) para guardarlas en el sistema.
    * 
    * @author Isaac Martínez Forte
   */

class FrequenciesDialog : public QDialog
{
    Q_OBJECT
    
public:
  
  /**
    * @fn FrequenciesDialog
    * @brief Constructor de la clase, que solo tiene por parámetro el padre de dicha clase.
    * 
    * @param parent padre de la clase a crear, puede se vacío.
    */
  
    explicit FrequenciesDialog(QWidget *parent = 0);
    
  /**
    * @fn FrequenciesDialog
    * @brief Constructor de la clase, que además tiene por parámetros el controlador del sistema, necesario para poder interaccionar con la lógica del sistema.
    * 
    * @param parent padre de la clase a crear, puede ser vacío.
    * @param controlador del sistema.
    */
  
    explicit FrequenciesDialog(Controlador * controlador, QWidget *parent = 0);
    ~FrequenciesDialog();


    bool getTodoCorrecto();
    
private slots:

    void on_fileButton_clicked();

    void on_okButton_clicked();

signals:
    void close_ok ();

private:
    Controlador * controlador;
    QString filePath;

    Ui::FrequenciesDialog *ui;

    bool todoCorrecto;
};

#endif // QFTBX_FREQUENCIES_DIALOG_H
