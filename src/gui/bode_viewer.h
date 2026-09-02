#ifndef QFTBX_BODE_VIEWER_H
#define QFTBX_BODE_VIEWER_H

#include <memory>

#include <QDialog>
#include <QVector>
#include <complex>
#include <qmath.h>
#include <QFileDialog>
#include <QMessageBox>

#include "qcustomplot.h"
#include "src/core/system/lti_system.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/frequencies/omega.h"

namespace Ui {
class BodeViewer;
}

/**
 * @brief Plots the Bode diagram of a plant over a set of design
 * frequencies.
 *
 * @author Isaac Martínez Forte
 */
class BodeViewer : public QDialog
{
    Q_OBJECT

public:
    explicit BodeViewer(QWidget *parent = 0);
    ~BodeViewer();


    /**
     * @brief Draws the Bode diagram.
     *
     * @param plant the plant to evaluate.
     * @param omega the frequencies to evaluate it at.
     */
    void drawBode(LtiSystem * plant, Omega * omega);

private slots:
    void on_actionExport_triggered();

private:
    std::unique_ptr<Ui::BodeViewer> ui;
    void drawAxis(QString yAxisName, const QVector<qreal> & yAxis_values,
                  const QVector<qreal> & frequencies, QCustomPlot * magnitudePlot);
};

#endif // QFTBX_BODE_VIEWER_H
