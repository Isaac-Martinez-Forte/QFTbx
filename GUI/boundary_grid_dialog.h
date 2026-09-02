#ifndef QFTBX_BOUNDARY_GRID_DIALOG_H
#define QFTBX_BOUNDARY_GRID_DIALOG_H

#include <memory>

#include <QDialog>

#include "QVector"
#include "QDoubleValidator"
#include "QIntValidator"
#include "src/core/math/sequence_vectors.h"

    /**
      * @class BoundaryGridDialog
      * @brief Clase gráfica que interactúa con el usuario para recoger datos necesarios para el cálculo de boundaries.
      *
      * @author Isaac Martínez Forte
     */

namespace Ui {
class BoundaryGridDialog;
}

class BoundaryGridDialog : public QDialog
{
    Q_OBJECT
    
public:
  

    /**
      * @fn BoundaryGridDialog
      * @brief Constructor de la clase.
      *
      * @param parent parámetro que indica el padre de la clase en una jerarquía gráfica, puede ser vacío.
      */

    explicit BoundaryGridDialog(QWidget *parent = 0);
    
    
    
    /**
      * @fn ~BoundaryGridDialog
      * @brief Destructor de la clase.
      */
    
    ~BoundaryGridDialog();
    
    
    /**
      * @fn phaseRangeValue
      * @brief Función que retorna el inicio y el fin del eje de coordenadas de las fases.
      * 
      * @return ParVal con el inicio y el fin del eje de coordenadas de las fases.
      */

    QPointF phaseRangeValue();
    
    
    /**
      * @fn phaseCountValue
      * @brief Función que retorna el número de puntos que tiene el eje de coordenadas de las fases.
      * 
      * @return entero con el número de puntos del eje de coordenadas de las fases.
      */
    
    qint32 phaseCountValue();
    
    
    /**
      * @fn magnitudeRangeValue
      * @brief Función que retorna el inicio y el fin del eje de coordenadas de las magnitudes.
      * 
      * @return ParVal con el inicio y el fin del eje de coordenadas de las magnitudes.
      */
    
    QPointF magnitudeRangeValue();
    
    
    /**
      * @fn magnitudeCountValue
      * @brief Función que retorna el número de puntos que tiene el eje de coordenadas de las magnitudes.
      * 
      * @return entero con el número de puntos del eje de coordenadas de las magnitudes.
      */
    
    qint32 magnitudeCountValue();
    
    
    /**
      * @fn infinityValue
      * @brief Función que retorna el valor que se ha establecido para infinityEdit.
      * 
      * @return real con el valor establecido para el infinityEdit.
      */
    
    qreal infinityValue();
    
    
    /**
      * @fn contourSelected
      * @brief Función que retorna un booleando indicando si el usuario a seleccionado utilizar el contorno de los templates para el cálculo de boundaries.
      * 
      * @return booleano indicando si se ha seleccionado utilizar el contorno de los templates para el cálculo de boundaries.
      */
    
    bool contourSelected();

    bool cudaSelected();

    bool getTodoCorrecto();

    
private slots:
    void on_buttonBox_accepted();

protected:
    void showEvent(QShowEvent * event) override;

private:
    std::unique_ptr<Ui::BoundaryGridDialog> ui;

    QPointF phaseRange;
    QPointF magnitudeRange;
    qint32 phaseCount = 0;
    qint32 magnitudeCount = 0;
    qreal infinityEdit = 0.0;
    bool accepted_once = false;
    bool cudaCheck = false;

    bool todoCorrecto;
};

#endif // QFTBX_BOUNDARY_GRID_DIALOG_H
