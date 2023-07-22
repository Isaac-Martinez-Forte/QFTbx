#ifndef DIAGRAMABODE_H
#define DIAGRAMABODE_H

#include <QDialog>
#include <QVector>
#include <complex>
#include <qmath.h>
#include <QFileDialog>
#include <QMessageBox>

#include "Librerias/qcustomplot/qcustomplot.h"
#include "Modelo/EstructuraSistema/sistema.h"
#include "Modelo/Herramientas/tools.h"
#include "Modelo/Objetos/omega.h"

 /**
    * @class DiagramaBode
    * @brief Clase gráfica que representa un Diagrama de Bode.
    * Dicha clase se encarga de representar gráficamente un Diagrama de Bode.
    *
    * @author Isaac Martínez Forte
   */

namespace Ui {
class DiagramaBode;
}

class DiagramaBode : public QDialog
{
    Q_OBJECT

public:
  /**
    * @fn DiagramaBode
    * @brief Constructor de la clase.
    *
    * @param parent padre del objeto en la jerarquía gráfica, puede ser vacío.
    */
    explicit DiagramaBode(QWidget *parent = 0);
    ~DiagramaBode();


  /**
    * @fn dibujarBode
    * @brief Función que dibuja gráficamente el Diagrama de Bode a partir de los parámetros pasados.
    *
    * @param planta de la cual queremos ver su Diagrama de Bode.
    * @param frecuencias necesarias para resolver la Planta.
   */

    void dibujarBode(Sistema * planta, Omega * omega);

private slots:
    void on_actionExportar_triggered();

private:
    Ui::DiagramaBode *ui;
    void dibujarDiagrama(QString nombreEjeY, QVector<qreal> *ejeY, QVector<qreal> *frecuencias, QCustomPlot * customPlot);
};

#endif // DIAGRAMABODE_H
