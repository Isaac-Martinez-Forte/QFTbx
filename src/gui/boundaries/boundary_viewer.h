#ifndef QFTBX_BOUNDARY_VIEWER_H
#define QFTBX_BOUNDARY_VIEWER_H

#include <vector>
#include <memory>

#include <QWidget>
#include <QVector>
#include <QFileDialog>


#include "src/core/math/sequence_vectors.h"
#include "src/core/boundaries/boundary_data.h"
#include "qcustomplot.h"
#include "src/gui/common/frequency_legend.h"


namespace Ui {
class BoundaryViewer;
}

namespace qftbx {


/**
 * @brief Plots the computed QFT boundaries on the Nichols chart, one curve
 * per design frequency and specification.
 *
 * @author Isaac Martínez Forte
 */
class BoundaryViewer : public QWidget
{
    Q_OBJECT
    
public:

    explicit BoundaryViewer(QWidget *parent = 0);
    ~BoundaryViewer();

    
   /**
    * @brief Publishes what the plot needs. Observers on both: the viewer
    * outlives neither.
    *
    * @param data the computed boundaries.
    * @param omega the design frequencies they were computed at.
    */
    void setData (const BoundaryData *data, std::vector<double> *omega);
    
    
    /**
     * @brief Forgets what it was drawing and empties the plot.
     *
     * A viewer lives in its phase's dock for as long as the window does,
     * and what it holds are observers on the project: when the project
     * drops a step, the pointers behind them go with it. This is what is
     * called then, instead of destroying the viewer.
     */
    void clear();

   /// Builds the plot from the data published by setData().
    void showDiagram();

private slots:

    void on_saveImage_clicked();

    void applyCheckboxes ();

private:

    void addFrequencyRow(QColor color, qint32 pos);
    FrequencyLegend * legend = nullptr;
    void clearDiagram();

    const BoundaryData * boundaryData = nullptr;
    std::vector<double> * omega = nullptr;

    bool plotted = false;

    //The curves BELONG TO QCustomPlot, which frees them on
    //clearPlottables(): only these containers are the viewer's.
    QVector <QVector <QCPCurve *> > curves;


    std::unique_ptr<Ui::BoundaryViewer> ui;
};

} // namespace qftbx

#endif // QFTBX_BOUNDARY_VIEWER_H
