#ifndef QFTBX_BOUNDARY_GRID_DIALOG_H
#define QFTBX_BOUNDARY_GRID_DIALOG_H

#include "src/core/settings.h"
#include "src/gui/step_dialog.h"
#include <memory>

#include <QDialog>

#include "src/core/range.h"

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
class BoundaryGridDialog : public StepDialog
{
    Q_OBJECT
    
public:
    /**
     * @brief The ceiling on grid cells, from the settings.
     *
     * Handed in rather than compiled in, so it can be changed without a
     * rebuild. It only ever REFUSES input, so moving it changes no result -
     * which is why it is the safest kind of setting there is.
     */
    void setMaxGridCells(std::int64_t cells) { m_maxGridCells = cells; }

  
    explicit BoundaryGridDialog(QWidget *parent = 0);

    ~BoundaryGridDialog();
    
    
    /// Start and end of the phase axis, in degrees.
    qftbx::Range phaseRangeValue();
    
    
    /// How many points the phase axis is sampled at.
    qint32 phaseCountValue();
    
    
    /// Start and end of the magnitude axis, in dB.
    qftbx::Range magnitudeRangeValue();
    
    
    /// How many points the magnitude axis is sampled at.
    qint32 magnitudeCountValue();
    
    
    /// Finite stand-in for infinity when the boundaries are exported; a
    /// negative value means none, and it takes no part in the computation.
    qreal infinityValue();
    
    
    /// Whether to feed the engine the template contours instead of the
    /// full value sets: far fewer points, at the epsilon-hull's accuracy.
    bool contourSelected();

    bool cudaSelected();


    
private slots:
    void on_buttonBox_accepted();

protected:
    void showEvent(QShowEvent * event) override;

private:
    std::unique_ptr<Ui::BoundaryGridDialog> ui;

    qftbx::Range phaseRange;
    qftbx::Range magnitudeRange;
    qint32 phaseCount = 0;
    qint32 magnitudeCount = 0;
    qreal infinityEdit = 0.0;
    bool accepted_once = false;
    bool cudaCheck = false;

    /// Default from qftbx::Settings::Limits.
    std::int64_t m_maxGridCells = qftbx::Settings().limits.maxGridCells;

};

#endif // QFTBX_BOUNDARY_GRID_DIALOG_H
