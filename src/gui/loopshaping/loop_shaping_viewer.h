/**
 * @file
 * @brief Shows the designed controller and its loop against the boundaries.
 *
 * Declares the viewer of the loop-shaping result: the boundary union with
 * one legend row per frequency, the open-loop curve, a marker per design
 * frequency, the controller's coefficients and formula at the digits the
 * user chooses, and the verdict of the check against the specifications.
 * The plant and the result are observers on the project; the plot owns
 * the curves and the viewer keeps the containers that tie a frequency's
 * pieces to its legend row.
 */

#ifndef QFTBX_LOOP_SHAPING_VIEWER_H
#define QFTBX_LOOP_SHAPING_VIEWER_H

#include <vector>
#include <memory>

#include <QWidget>

#include "src/core/math/sequence_vectors.h"
#include "qcustomplot.h"
#include "src/gui/common/frequency_legend.h"
#include "src/core/boundaries/boundary_types.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/loop_shaping_result.h"
#include "src/core/specifications/specification.h"

namespace Ui {
class LoopShapingViewer;
}

namespace qftbx {

/**
 * @brief Shows the designed controller and its nominal loop transmission
 * over the Nichols chart, against the boundaries it had to respect.
 *
 * The frequency sweep it plots over is fixed rather than taken from the
 * user; the reasons, and where the answer lies, are recorded at the
 * commented-out block in the implementation.
 */
class LoopShapingViewer : public QWidget
{
    Q_OBJECT

public:
    explicit LoopShapingViewer(QWidget *parent = 0);
    ~LoopShapingViewer();

    void setData (const qftbx::UnionTraces & unionTraces, std::vector<double> *omega, LoopShapingResult * loopShapingData, LtiSystem *plant, bool linSpace);

    /**
     * @brief Forgets what it was drawing and empties the plot.
     *
     * A viewer lives in its phase's dock for as long as the window does,
     * and what it holds are observers on the project: when the project
     * drops a step, the pointers behind them go with it. This is what is
     * called then, instead of destroying the viewer.
     */
    void clear();

    void showDiagram();

signals:
    /// The user chose how many digits the numbers are worth showing at. The
    /// window writes it into the settings; the global itself is set here,
    /// because everything drawn after this reads it.
    void digitsChanged(int digits);

private slots:
    void applyCheckboxes();

    /// The controller, its formula and the verdict, at the digits chosen.
    void showController();

    void on_saveImage_clicked();

private:
    void showCheck();
    void fillDigitsCombo();
    QString specificationTitle(qftbx::SpecificationType type);

    qftbx::UnionTraces unionTraces;
    std::vector<double> * omega = nullptr;
    /// Observers on the project's objects, handed in by setData(): the
    /// viewer never owns what it draws.
    LtiSystem * plant = nullptr;
    LoopShapingResult * loopShapingData = nullptr;

    bool plotted = false;

    /// The curves BELONG TO QCustomPlot, which frees them on
    /// clearPlottables(): only the container is the viewer's.
    QVector <QCPCurve *> curves;

    /// The pieces of each frequency's boundary, by frequency: what the legend
    /// shows or hides. The boundary of a frequency is not always one curve,
    /// so the row of the legend and the curve are no longer one to one.
    /// Observers: the curves above own nothing either, QCustomPlot does.
    QVector <QVector <QCPCurve *>> boundaryCurves;

    void addFrequencyRow(QColor color, qint32 pos);
    FrequencyLegend * legend = nullptr;
    void clearDiagram();

    bool linSpace = false;

    std::unique_ptr<Ui::LoopShapingViewer> ui;
};

}

#endif
