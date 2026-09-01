#ifndef QFTBX_LOOP_BOUNDARIES_VIEWER_H
#define QFTBX_LOOP_BOUNDARIES_VIEWER_H

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


    void setDatos (const BoundaryData * nicholsData, const BoundaryData * nyquistData, QVector<qreal> *omega,
                   LtiSystem * plant, LtiSystem * controller, bool nichols, bool nyquist);

    void showDiagram();

    void drawBox (QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, QColor color);

private slots:
    void applyCheckboxes();

    void on_saveImage_clicked();

private:

    const BoundaryData * nicholsData = nullptr;
    const BoundaryData * nyquistData = nullptr;
    LtiSystem * plant;
    LtiSystem * controller;
    QVector <qreal> * omega;

    bool plotted;

    QVector <QCPCurve * > * curves;
    QGroupBox * frequenciesBox;
    QVector <QCheckBox *> * checkboxes;
    QMap <QString, QColor> * rowColors;
    QVBoxLayout * colorsLayout;

    void addFrequencyRow(QColor color, qint32 pos);
    void clearDiagram();

    bool nichols;
    bool nyquist;

    Ui::LoopBoundariesViewer *ui;

    qint32 finalCurveIndex;

};

#endif // QFTBX_BOUNDARY_UNION_VIEWER_H
