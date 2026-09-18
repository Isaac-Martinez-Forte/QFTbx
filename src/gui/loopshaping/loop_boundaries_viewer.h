#ifndef QFTBX_LOOP_BOUNDARIES_VIEWER_H
#define QFTBX_LOOP_BOUNDARIES_VIEWER_H

#include <vector>
#include <memory>

#include <QDialog>

#include "src/core/math/sequence_vectors.h"
#include "qcustomplot.h"
#include "src/gui/common/frequency_legend.h"

#include "src/core/boundaries/boundary_data.h"

#include "src/core/system/lti_system.h"


namespace Ui {
class LoopBoundariesViewer;
}

namespace qftbx {


class LoopBoundariesViewer : public QDialog
{
    Q_OBJECT

public:
    explicit LoopBoundariesViewer(QWidget *parent = 0);
    ~LoopBoundariesViewer();


    /**
     * @brief Publishes what the two diagrams draw.
     *
     * @param nicholsData the boundaries, on the chart they were computed on.
     * @param nyquistTraces the same union read on the complex plane. The
     * curves themselves, not a BoundaryData: this view is only drawn, never
     * classified, and its points are complex, not Nichols points.
     * @param omega the design frequencies the curves belong to.
     * @param plant, controller what the loop is drawn from.
     * @param nichols, nyquist which of the two diagrams to draw.
     */
    void setData (const BoundaryData * nicholsData, const qftbx::NyquistTraces & nyquistTraces,
                   std::vector<double> *omega,
                   LtiSystem * plant, LtiSystem * controller, bool nichols, bool nyquist);

    void showDiagram();

private slots:
    void applyCheckboxes();

    void on_saveImage_clicked();

private:
    /// The pieces of one boundary as one curve each, all of them under one
    /// row of the legend. The cuts are where the Nichols trace jumps, so
    /// the two diagrams are cut in the same places.
    QVector<QCPCurve *> piecesOf(const std::vector<double> & x, const std::vector<double> & y,
                                 const std::vector<std::size_t> & cuts, const QColor & color);


    const BoundaryData * nicholsData = nullptr;
    //By value: the curves are computed for this view and belong to it.
    qftbx::NyquistTraces nyquistTraces;
    //Observers on the project's objects, handed in by setData(): the
    //viewer never owns what it draws.
    LtiSystem * plant = nullptr;
    LtiSystem * controller = nullptr;
    std::vector<double> * omega = nullptr;

    bool plotted = false;

    //One entry per ROW of the legend - a frequency in one of the two
    //diagrams - and inside it one curve per piece of that boundary, which
    //is not always one.
    //The curves BELONG TO QCustomPlot, which frees them on
    //clearPlottables(): only the container is the viewer's.
    QVector <QVector <QCPCurve *>> curves;

    /// One row per curve, labelled with its frequency AND its diagram: in
    /// the both-diagrams mode a frequency gets two rows.
    void addFrequencyRow(QColor color, qint32 pos, QString diagram);
    FrequencyLegend * legend = nullptr;
    void clearDiagram();

    bool nichols = false;
    bool nyquist = false;

    std::unique_ptr<Ui::LoopBoundariesViewer> ui;

};

} // namespace qftbx

#endif // QFTBX_LOOP_BOUNDARIES_VIEWER_H
