#ifndef QFTBX_LOOP_SHAPING_VIEWER_H
#define QFTBX_LOOP_SHAPING_VIEWER_H

#include <memory>

#include <QDialog>

#include "src/core/math/sequence_vectors.h"
#include "qcustomplot.h"
#include "src/core/boundaries/boundary_types.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/loop_shaping_result.h"


namespace Ui {
class LoopShapingViewer;
}

/**
 * @brief Shows the designed controller and its nominal loop transmission
 * over the Nichols chart, against the boundaries it had to respect.
 *
 * The frequency sweep it plots over is fixed rather than taken from the
 * user; the reasons, and where the answer lies, are recorded at the
 * commented-out block in the implementation.
 */
class LoopShapingViewer : public QDialog
{
    Q_OBJECT

public:
    explicit LoopShapingViewer(QWidget *parent = 0);
    ~LoopShapingViewer();


    void setData (const qftbx::UnionTraces & unionTraces, QVector<qreal> *omega, LoopShapingResult * loopShapingData, LtiSystem *plant, bool linSpace);

    void showDiagram();

private slots:
    void applyCheckboxes();


    void on_saveImage_clicked();

private:

    qftbx::UnionTraces unionTraces;
    QVector <qreal> * omega = nullptr;
    //Observers on the project's objects, handed in by setData(): the
    //viewer never owns what it draws.
    LtiSystem * plant = nullptr;
    LoopShapingResult * loopShapingData = nullptr;

    bool plotted = false;

    //The curves BELONG TO QCustomPlot, which frees them on
    //clearPlottables(): only the container is the viewer's.
    QVector <QCPCurve *> curves;
    QGroupBox * frequenciesBox = nullptr;
    //The checkboxes belong to their row widget: the viewer deletes the
    //rows, not these.
    QVector <QCheckBox *> checkboxes;
    QVBoxLayout * colorsLayout = nullptr;

    void addFrequencyRow(QColor color, qint32 pos);
    void clearDiagram();

    bool linSpace = false;

    std::unique_ptr<Ui::LoopShapingViewer> ui;
};

#endif // QFTBX_LOOP_SHAPING_VIEWER_H
