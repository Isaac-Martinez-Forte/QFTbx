/**
 * @file
 * @brief The form that asks for the Nichols grid the boundaries are computed on.
 *
 * Declares the step panel for the phase and magnitude axes, their point
 * counts, the finite stand-in for infinity used on export, whether the
 * engine is fed the template contours or the full value sets, and the
 * CPU/GPU choice. Defaults come from the settings and a ceiling on grid
 * cells, also from the settings, only ever refuses input.
 */

#ifndef QFTBX_BOUNDARY_GRID_FORM_H
#define QFTBX_BOUNDARY_GRID_FORM_H

#include "src/core/boundaries/boundary_data.h"
#include "src/core/project/settings.h"
#include "src/gui/application/step_panel.h"
#include <memory>

#include <QWidget>

#include "src/core/math/range.h"

#include <QDoubleValidator>
#include <QIntValidator>
#include "src/core/math/sequence_vectors.h"

namespace Ui {
class BoundaryGridForm;
}

namespace qftbx {

/**
 * @brief Asks the user for the Nichols grid the boundaries are computed
 * over: the phase and magnitude axes, their point counts, the stand-in for
 * infinity, and whether to feed the engine the template contours.
 *
 * @author Isaac Martínez Forte
 */
class BoundaryGridForm : public StepPanel
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

    /**
     * @brief Prefills the grid fields from the settings.
     *
     * Prefilling only: the user sees the values and types over them if they
     * want, so nothing here can change a computed result. It is the setting
     * that shows most in daily use - whoever always works with the same
     * Nichols grid should not have to type it again every time.
     */
    void applyDefaults(const qftbx::Settings::Defaults & defaults);

    /**
     * @brief Fills the grid with the one the project's boundaries were
     * computed on, so that reopening the step shows what produced them
     * instead of the defaults.
     *
     * Null boundaries leave the defaults in place: there is nothing to show.
     */
    void setFromProject(const qftbx::BoundaryData * boundaries);

    explicit BoundaryGridForm(QWidget *parent = 0);

    ~BoundaryGridForm();

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
    void on_okButton_clicked();

private:
    std::unique_ptr<Ui::BoundaryGridForm> ui;

    qftbx::Range phaseRange;
    qftbx::Range magnitudeRange;
    qint32 phaseCount = 0;
    qint32 magnitudeCount = 0;
    qreal infinityEdit = 0.0;
    bool cudaCheck = false;

    /// Default from qftbx::Settings::Limits.
    std::int64_t m_maxGridCells = qftbx::Settings().limits.maxGridCells;

};

}

#endif
