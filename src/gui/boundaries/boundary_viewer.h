/**
 * @file
 * @brief Plots the computed QFT boundaries on the Nichols chart.
 *
 * Declares the viewer that draws every boundary per design frequency and
 * specification, coloured by frequency, with a legend that hides a
 * frequency at a time. setData publishes the boundary data and the
 * frequencies as observers, which must stay alive until clear(), and
 * showDiagram builds the plot from them. The viewer lives as long as the
 * window: when the project drops the step it is emptied with clear(), not
 * destroyed. The plot owns the curves and frees them when it clears its
 * plottables; the viewer keeps only the containers.
 */

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

class BoundaryViewer : public QWidget
{
    Q_OBJECT

public:

    explicit BoundaryViewer(QWidget *parent = 0);
    ~BoundaryViewer();

    void setData (const BoundaryData *data, std::vector<double> *omega);

    void clear();

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

    QVector <QVector <QCPCurve *> > curves;

    std::unique_ptr<Ui::BoundaryViewer> ui;
};

}

#endif
