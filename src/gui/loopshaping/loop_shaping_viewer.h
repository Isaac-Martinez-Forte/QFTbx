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

private slots:
    void applyCheckboxes();


    void on_saveImage_clicked();

private:
    void showCheck();
    QString specificationTitle(qftbx::SpecificationType type);

    qftbx::UnionTraces unionTraces;
    std::vector<double> * omega = nullptr;
    //Observers on the project's objects, handed in by setData(): the
    //viewer never owns what it draws.
    LtiSystem * plant = nullptr;
    LoopShapingResult * loopShapingData = nullptr;

    bool plotted = false;

    //The curves BELONG TO QCustomPlot, which frees them on
    //clearPlottables(): only the container is the viewer's.
    QVector <QCPCurve *> curves;

    void addFrequencyRow(QColor color, qint32 pos);
    FrequencyLegend * legend = nullptr;
    void clearDiagram();

    bool linSpace = false;

    std::unique_ptr<Ui::LoopShapingViewer> ui;
};

} // namespace qftbx

#endif // QFTBX_LOOP_SHAPING_VIEWER_H
