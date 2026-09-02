#ifndef QFTBX_LOOP_BOUNDARIES_VIEWER_H
#define QFTBX_LOOP_BOUNDARIES_VIEWER_H

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
    //Observers on the project's objects, handed in by setDatos(): the
    //viewer never owns what it draws.
    LtiSystem * plant = nullptr;
    LtiSystem * controller = nullptr;
    QVector <qreal> * omega = nullptr;

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
