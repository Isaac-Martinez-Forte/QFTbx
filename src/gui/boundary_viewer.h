#ifndef QFTBX_BOUNDARY_VIEWER_H
#define QFTBX_BOUNDARY_VIEWER_H

#include <memory>

#include <QDialog>
#include <QVector>
#include <QFileDialog>


#include "src/core/math/sequence_vectors.h"
#include "src/core/boundaries/boundary_data.h"
#include "qcustomplot.h"



namespace Ui {
class BoundaryViewer;
}

/**
 * @brief Plots the computed QFT boundaries on the Nichols chart, one curve
 * per design frequency and specification.
 *
 * @author Isaac Martínez Forte
 */
class BoundaryViewer : public QDialog
{
    Q_OBJECT
    
public:

    explicit BoundaryViewer(QWidget *parent = 0);
    ~BoundaryViewer();

    
   /**
    * @brief Publishes what the plot needs. Observers on both: the viewer
    * outlives neither.
    *
    * @param datos the computed boundaries.
    * @param omega the design frequencies they were computed at.
    */
    void setDatos (const BoundaryData *datos, QVector<qreal> *omega);
    
    
   /// Builds the plot from the data published by setDatos().
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
