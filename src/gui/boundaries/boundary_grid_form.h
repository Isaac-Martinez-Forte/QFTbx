/**
 * @file
 * @brief The form that asks for the Nichols grid the boundaries are
 * computed on.
 *
 * Declares the step panel for the phase axis, in degrees, and the
 * magnitude axis, in dB, their point counts, the finite stand-in for
 * infinity used on export (negative for none; it takes no part in the
 * computation), whether the engine is fed the template contours, far fewer
 * points at the epsilon hull's accuracy, or the full value sets, and the
 * CPU/GPU choice. Defaults come from the settings and only prefill the
 * fields; setFromProject() shows instead the grid the project's boundaries
 * were computed on, and leaves the defaults when there are none. A ceiling
 * on grid cells, also from the settings, only ever refuses input, so
 * moving it changes no result.
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

class BoundaryGridForm : public StepPanel
{
    Q_OBJECT

public:
    void setMaxGridCells(std::int64_t cells) { m_maxGridCells = cells; }

    void applyDefaults(const qftbx::Settings::Defaults & defaults);

    void setFromProject(const qftbx::BoundaryData * boundaries);

    explicit BoundaryGridForm(QWidget *parent = 0);

    ~BoundaryGridForm();

    qftbx::Range phaseRangeValue();

    qint32 phaseCountValue();

    qftbx::Range magnitudeRangeValue();

    qint32 magnitudeCountValue();

    qreal infinityValue();

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

    std::int64_t m_maxGridCells = qftbx::Settings().limits.maxGridCells;

};

}

#endif
