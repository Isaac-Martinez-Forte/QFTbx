#ifndef QFTBX_BOUNDARY_GRID_DIALOG_H
#define QFTBX_BOUNDARY_GRID_DIALOG_H

#include <memory>

#include <QDialog>

#include "QVector"
#include "QDoubleValidator"
#include "QIntValidator"
#include "src/core/math/sequence_vectors.h"

namespace Ui {
class BoundaryGridDialog;
}

/**
 * @brief Asks the user for the Nichols grid the boundaries are computed
 * over: the phase and magnitude axes, their point counts, the stand-in for
 * infinity, and whether to feed the engine the template contours.
 *
 * @author Isaac Martínez Forte
 */
class BoundaryGridDialog : public QDialog
{
    Q_OBJECT
    
public:
  
    explicit BoundaryGridDialog(QWidget *parent = 0);

    ~BoundaryGridDialog();
    
    
    /// Start and end of the phase axis, in degrees.
    QPointF phaseRangeValue();
    
    
    /// How many points the phase axis is sampled at.
    qint32 phaseCountValue();
    
    
    /// Start and end of the magnitude axis, in dB.
    QPointF magnitudeRangeValue();
    
    
    /// How many points the magnitude axis is sampled at.
    qint32 magnitudeCountValue();
    
    
    /// Finite stand-in for infinity when the boundaries are exported; a
    /// negative value means none, and it takes no part in the computation.
    qreal infinityValue();
    
    
    /// Whether to feed the engine the template contours instead of the
    /// full value sets: far fewer points, at the epsilon-hull's accuracy.
    bool contourSelected();

    bool cudaSelected();

    bool wasAccepted();

    
private slots:
    void on_buttonBox_accepted();

protected:
    void showEvent(QShowEvent * event) override;

private:
    std::unique_ptr<Ui::BoundaryGridDialog> ui;

    QPointF phaseRange;
    QPointF magnitudeRange;
    qint32 phaseCount = 0;
    qint32 magnitudeCount = 0;
    qreal infinityEdit = 0.0;
    bool accepted_once = false;
    bool cudaCheck = false;

    bool accepted;
};

#endif // QFTBX_BOUNDARY_GRID_DIALOG_H
