#ifndef INTDATOSBOUNDARIES_H
#define INTDATOSBOUNDARIES_H

#include <QDialog>

#include "QVector"
#include "QDoubleValidator"
#include "QIntValidator"
#include "Modelo/Herramientas/tools.h"

    /**
      * @class IntDatosBoundaries
      * @brief Clase gráfica que interactúa con el usuario para recoger datos necesarios para el cálculo de boundaries.
      *
      * @author Isaac Martínez Forte
     */

namespace Ui {
class IntDatosBoundaries;
}

class IntDatosBoundaries : public QDialog
{
    Q_OBJECT
    
public:
  

    /**
      * @fn IntDatosBoundaries
      * @brief Constructor de la clase.
      *
      * @param parent parámetro que indica el padre de la clase en una jerarquía gráfica, puede ser vacío.
      */

    explicit IntDatosBoundaries(QWidget *parent = 0);
    
    
    
    /**
      * @fn ~IntDatosBoundaries
      * @brief Destructor de la clase.
      */
    
    ~IntDatosBoundaries();
    
    
    /**
      * @fn getDatosFas
      * @brief Función que retorna el inicio y el fin del eje de coordenadas de las fases.
      * 
      * @return ParVal con el inicio y el fin del eje de coordenadas de las fases.
      */

    QPointF getDatosFas();
    
    
    /**
      * @fn getPuntosFas
      * @brief Función que retorna el número de puntos que tiene el eje de coordenadas de las fases.
      * 
      * @return entero con el número de puntos del eje de coordenadas de las fases.
      */
    
    qint32 getPuntosFas();
    
    
    /**
      * @fn getDatosMag
      * @brief Función que retorna el inicio y el fin del eje de coordenadas de las magnitudes.
      * 
      * @return ParVal con el inicio y el fin del eje de coordenadas de las magnitudes.
      */
    
    QPointF getDatosMag();
    
    
    /**
      * @fn getPuntosMag
      * @brief Función que retorna el número de puntos que tiene el eje de coordenadas de las magnitudes.
      * 
      * @return entero con el número de puntos del eje de coordenadas de las magnitudes.
      */
    
    qint32 getPuntosMag();
    
    
    /**
      * @fn getInfinito
      * @brief Función que retorna el valor que se ha establecido para infinito.
      * 
      * @return real con el valor establecido para el infinito.
      */
    
    qreal getInfinito();
    
    
    /**
      * @fn isContornoSelect
      * @brief Función que retorna un booleando indicando si el usuario a seleccionado utilizar el contorno de los templates para el cálculo de boundaries.
      * 
      * @return booleano indicando si se ha seleccionado utilizar el contorno de los templates para el cálculo de boundaries.
      */
    
    bool isContornoSelect();

    bool getCUDA();

    bool getTodoCorrecto();

    
private slots:
    void on_buttonBox_accepted();

private:
    Ui::IntDatosBoundaries *ui;

    QPointF datosFase;
    QPointF datosMag;
    qint32 nPuntosFas;
    qint32 nPuntosMag;
    qreal infinito;
    bool realizado;
    bool cuda;

    bool todoCorrecto;
};

#endif // INTDATOSBOUNDARIES_H
