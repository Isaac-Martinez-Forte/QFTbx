/**
 * @file
 * @brief Shows the designed controller and its loop against the boundaries.
 *
 * Declares the viewer of the loop-shaping result: the boundary union with
 * one legend row per frequency, the open-loop curve, a marker per design
 * frequency, the controller's coefficients and formula at the digits the
 * user chooses, and one line of the verdict on the specifications and the
 * closed-loop stability, whose whole is in the report beside it
 * (loop_shaping_report.h). The plant and the result are observers on the
 * project; the plot owns the curves and the viewer keeps the containers
 * that tie a frequency's pieces to its legend row. The frequency sweep it
 * plots over is fixed rather than taken from the user. clear() forgets
 * what it was drawing when the project drops the step, since the observers
 * go with it.
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
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/specifications/specification.h"

namespace Ui {
class LoopShapingViewer;
}

namespace qftbx {

class LoopShapingViewer : public QWidget
{
    Q_OBJECT

public:
    explicit LoopShapingViewer(QWidget *parent = 0);
    ~LoopShapingViewer();

    void setData (const qftbx::UnionTraces & unionTraces, std::vector<double> *omega, LoopShapingResult * loopShapingData, LtiSystem *plant, bool linSpace);

    void clear();

    void showDiagram();

    void setSummaryShown(bool shown);

signals:
    void digitsChanged(int digits);

    void detailsAsked();

private slots:
    void applyCheckboxes();

    void showController();

    void on_saveImage_clicked();

private:
    bool eventFilter(QObject * watched, QEvent * event) override;

    void showCheck();
    void elideSummary();
    void fillDigitsCombo();

    QString summary;

    qftbx::UnionTraces unionTraces;
    std::vector<double> * omega = nullptr;
    LtiSystem * plant = nullptr;
    LoopShapingResult * loopShapingData = nullptr;

    bool plotted = false;

    QVector <QCPCurve *> curves;

    QVector <QVector <QCPCurve *>> boundaryCurves;

    void addFrequencyRow(QColor color, qint32 pos);
    FrequencyLegend * legend = nullptr;
    void clearDiagram();

    bool linSpace = false;

    std::unique_ptr<Ui::LoopShapingViewer> ui;
};

}

#endif
