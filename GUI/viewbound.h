#ifndef VIEWBOUND_H
#define VIEWBOUND_H

#include <QDialog>
#include <QVector>
#include <QFileDialog>


#include "Modelo/Herramientas/tools.h"
#include "Modelo/EstructurasDatos/datosbound.h"
#include "Librerias/qcustomplot/qcustomplot.h"



  /**
    * @class ViewBound
    * @brief Clase que representa gráficamente los boundaries calculados.
    * 
    * @author Isaac Martínez Forte
    */


namespace Ui {
class ViewBound;
}

class ViewBound : public QDialog
{
    Q_OBJECT
    
public:
  
   /**
    * @fn ViewBound
    * @brief Constructor de la clase que solo tiene como parámetro el padre de la misma.
    * 
    * @param parent padre de la clase que se para como parámetro al constructor de la superclase, puede ser vacío.
    */
  
    explicit ViewBound(QWidget *parent = 0);
    ~ViewBound();

    
   /**
    * @fn setDatos
    * @brief Función que introduce los datos necesarios para representar los boundaries.
    * 
    * @param datos boundaries calculados.
    * @param sabana sábana completa del cálculo intermedio a los boundaries.
    */
    
    void setDatos (DatosBound *datos, QVector<qreal> *omega);
    
    
   /**
    * @fn mostrarDiagrama
    * @brief Función que crea la gráfica con los datos introducidos anteriormente.
    */
    
    void mostrarDiagrama();

private slots:
    void on_exportar_clicked();

    void on_guardar_clicked();

    void revisarCheckBox ();

private:

    void crearCuadro(QColor color, qint32 pos);

    DatosBound * boundaries;
    QVector <qreal> * omega;

    bool ejecutado;

    QVector <QVector <QCPCurve * > * > * graficos;

    QGroupBox * cajaFrecuencias;
    QVector <QCheckBox *> * checkbox;
    QMap <QString, QColor> * colores;
    QVBoxLayout * layoutColores;

    Ui::ViewBound *ui;
};

#endif // VIEWBOUND_H
