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

class LoopShapingViewer : public QDialog
{
    Q_OBJECT

public:
    explicit LoopShapingViewer(QWidget *parent = 0);
    ~LoopShapingViewer();


    void setDatos (const qftbx::UnionTraces & unionTraces, QVector<qreal> *omega, LoopShapingResult * loopShapingData, LtiSystem *plant, bool linSpace);

    void showDiagram();

private slots:
    void applyCheckboxes();


    void on_saveImage_clicked();

private:

    qftbx::UnionTraces unionTraces;
    QVector <qreal> * omega = nullptr;
    //Observers on the project's objects, handed in by setDatos(): the
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
    QMap <QString, QColor> * rowColors;
    QVBoxLayout * colorsLayout = nullptr;

    void addFrequencyRow(QColor color, qint32 pos);
    void clearDiagram();

    bool linSpace = false;

    std::unique_ptr<Ui::LoopShapingViewer> ui;
};

#endif // QFTBX_LOOP_SHAPING_VIEWER_H
