#ifndef QFTBX_LOOP_SHAPING_VIEWER_H
#define QFTBX_LOOP_SHAPING_VIEWER_H

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
    QVector <qreal> * omega;
    LtiSystem * plant;
    LoopShapingResult * loopShapingData;

    bool plotted;

    QVector <QCPCurve * > * curves;
    QGroupBox * frequenciesBox;
    QVector <QCheckBox *> * checkboxes;
    QMap <QString, QColor> * rowColors;
    QVBoxLayout * colorsLayout;

    void addFrequencyRow(QColor color, qint32 pos);
    void clearDiagram();

    bool linSpace;

    Ui::LoopShapingViewer *ui;
};

#endif // QFTBX_LOOP_SHAPING_VIEWER_H
