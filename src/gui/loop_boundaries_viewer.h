#ifndef QFTBX_LOOP_BOUNDARIES_VIEWER_H
#define QFTBX_LOOP_BOUNDARIES_VIEWER_H

#include <vector>
#include <memory>

#include <QDialog>

//#include "cinterval.hpp"

#include "src/core/math/sequence_vectors.h"
#include "qcustomplot.h"

#include "src/core/boundaries/boundary_data.h"

#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/natural_interval_extension.h"


namespace Ui {
class LoopBoundariesViewer;
}

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
     * curves themselves, not a BoundaryData: the viewer used to be handed
     * one fabricated for the occasion, which had to carry empty bucket rows
     * because this view is only drawn and never classified, and whose
     * Nichols-typed points were holding real and imaginary parts.
     * @param omega the design frequencies the curves belong to.
     * @param plant, controller what the loop is drawn from.
     * @param nichols, nyquist which of the two diagrams to draw.
     */
    void setData (const BoundaryData * nicholsData, const qftbx::NyquistTraces & nyquistTraces,
                   std::vector<double> *omega,
                   LtiSystem * plant, LtiSystem * controller, bool nichols, bool nyquist);

    void showDiagram();

    void drawBox (QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, QColor color);

private slots:
    void applyCheckboxes();

    void on_saveImage_clicked();

private:

    const BoundaryData * nicholsData = nullptr;
    //By value: the curves are computed for this view and belong to it.
    qftbx::NyquistTraces nyquistTraces;
    //Observers on the project's objects, handed in by setData(): the
    //viewer never owns what it draws.
    LtiSystem * plant = nullptr;
    LtiSystem * controller = nullptr;
    std::vector<double> * omega = nullptr;

    bool plotted = false;

    //The curves BELONG TO QCustomPlot, which frees them on
    //clearPlottables(): only the container is the viewer's.
    QVector <QCPCurve *> curves;
    QGroupBox * frequenciesBox = nullptr;
    //The checkboxes belong to their row widget: the viewer deletes the
    //rows, not these.
    QVector <QCheckBox *> checkboxes;
    QVBoxLayout * colorsLayout = nullptr;

    /// One row per curve, labelled with its frequency and its diagram: in
    /// the both-diagrams mode a frequency gets two rows, and they used to
    /// carry the same text.
    void addFrequencyRow(QColor color, qint32 pos, QString diagram);
    void clearDiagram();

    bool nichols = false;
    bool nyquist = false;

    std::unique_ptr<Ui::LoopBoundariesViewer> ui;

    qint32 finalCurveIndex = 0;

};

#endif // QFTBX_BOUNDARY_UNION_VIEWER_H
