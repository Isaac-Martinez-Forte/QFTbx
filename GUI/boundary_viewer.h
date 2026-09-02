#ifndef QFTBX_BOUNDARY_VIEWER_H
#define QFTBX_BOUNDARY_VIEWER_H

#include <memory>

#include <QDialog>
#include <QVector>
#include <QFileDialog>


#include "src/core/math/sequence_vectors.h"
#include "src/core/boundaries/boundary_data.h"
#include "qcustomplot.h"



  /**
    * @class BoundaryViewer
    * @brief Clase que representa gráficamente los boundaryData calculados.
    * 
    * @author Isaac Martínez Forte
    */


namespace Ui {
class BoundaryViewer;
}

class BoundaryViewer : public QDialog
{
    Q_OBJECT
    
public:
  
   /**
    * @fn BoundaryViewer
    * @brief Constructor de la clase que solo tiene como parámetro el padre de la misma.
    * 
    * @param parent padre de la clase que se para como parámetro al constructor de la superclase, puede ser vacío.
    */
  
    explicit BoundaryViewer(QWidget *parent = 0);
    ~BoundaryViewer();

    
   /**
    * @fn setDatos
    * @brief Función que introduce los datos necesarios para representar los boundaryData.
    * 
    * @param datos boundaryData calculados.
    * @param sabana sábana completa del cálculo intermedio a los boundaryData.
    */
    
    void setDatos (const BoundaryData *datos, QVector<qreal> *omega);
    
    
   /**
    * @fn showDiagram
    * @brief Función que crea la gráfica con los datos introducidos anteriormente.
    */
    
    void showDiagram();

private slots:

    void on_saveImage_clicked();

    void applyCheckboxes ();

private:

    void addFrequencyRow(QColor color, qint32 pos);
    void clearDiagram();

    const BoundaryData * boundaryData = nullptr;
    QVector <qreal> * omega = nullptr;

    bool plotted = false;

    //The curves BELONG TO QCustomPlot, which frees them on
    //clearPlottables(): only these containers are the viewer's, and they
    //used to be a vector of pointers behind a pointer.
    QVector <QVector <QCPCurve *> > curves;

    QGroupBox * frequenciesBox = nullptr;
    //The checkboxes belong to their row widget, which belongs to the
    //layout: the viewer deletes the rows, not these.
    QVector <QCheckBox *> checkboxes;
    QVBoxLayout * colorsLayout = nullptr;

    std::unique_ptr<Ui::BoundaryViewer> ui;
};

#endif // QFTBX_BOUNDARY_VIEWER_H
