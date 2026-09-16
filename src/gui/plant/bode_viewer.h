#ifndef QFTBX_BODE_VIEWER_H
#define QFTBX_BODE_VIEWER_H

#include <vector>
#include <memory>

#include <QWidget>
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

namespace qftbx {


/**
 * @brief Plots the Bode diagram of a plant over a set of design
 * frequencies.
 *
 * @author Isaac Martínez Forte
 */
class BodeViewer : public QWidget
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

    /**
     * @brief Empties both plots.
     *
     * The viewer lives in the plant's dock for as long as the window does,
     * so when the project loses its plant this is what is called, instead
     * of destroying it.
     */
    void clear();

private slots:
    void on_saveImage_clicked();

private:
    std::unique_ptr<Ui::BodeViewer> ui;
    void drawAxis(QString yAxisName, const std::vector<double> & yAxis_values,
                  const std::vector<double> & frequencies, QCustomPlot * magnitudePlot);
};

} // namespace qftbx

#endif // QFTBX_BODE_VIEWER_H
