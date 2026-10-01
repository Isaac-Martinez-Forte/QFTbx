/**
 * @file
 * @brief The step that picks the loop-shaping algorithm and its accuracy.
 *
 * Step 7 of the design: the panel for the algorithm, its epsilon, the
 * starting point of the local search where the algorithm has one, and the
 * range and point count of the plot. What the epsilon measures depends on the
 * algorithm, since each follows the criterion of its own paper (see
 * ProjectController::computeLoopShaping), and the label of the field says
 * which. The range fields are prefilled from the settings, the same range on
 * opening and on picking either spacing, and the ceilings, also from the
 * settings, only keep a typo from reaching a conversion or an allocation:
 * moving them changes no computed result. Reopening a design shows the
 * algorithm, epsilon, range and point count that produced it.
 */

#ifndef QFTBX_LOOP_SHAPING_FORM_H
#define QFTBX_LOOP_SHAPING_FORM_H

#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/loopshaping/loop_shaping_result.h"
#include "src/core/project/settings.h"
#include "src/gui/application/step_panel.h"
#include <memory>

#include <QWidget>

#include "src/core/math/range.h"

#include "src/core/math/sequence_vectors.h"

namespace Ui {
class LoopShapingForm;
}

namespace qftbx {

class LoopShapingForm : public StepPanel
{
    Q_OBJECT

public:
    void setLimits(double maxMagnitude, double maxPointCount)
    { m_maxMagnitude = maxMagnitude; m_maxPointCount = maxPointCount; }

    void applyDefaults(const qftbx::Settings::Defaults & defaults);

    explicit LoopShapingForm(QWidget *parent = 0);
    ~LoopShapingForm();

    qreal epsilonValue ();

    qftbx::LoopShapingAlgorithm algorithmValue();

    qftbx::Range range();

    qreal pointCountValue();

    bool isLinSpace();

    void setFromProject(const qftbx::LoopShapingResult * result);

    qint32 initialisationValue ();

private slots:
    void updateEpsilonLabel();

    void on_okButton_clicked();

    void on_linspaceRadio_clicked();

    void on_logspaceRadio_clicked();

    void on_ntRadio_clicked();

    void on_nkRadio_clicked();

    void on_mrRadio_clicked();
    void on_mc1Radio_clicked();
    void on_mc2Radio_clicked();

private:
    std::unique_ptr<Ui::LoopShapingForm> ui;

    qreal epsilonEdit = 0.0;

    qftbx::Range plotRange;

    qreal pointCountEdit = 0.0;

    qint32 initialisation = 0;

    qftbx::LoopShapingAlgorithm alg = qftbx::nt;

    bool linLogSpace = false;
    qftbx::Settings::Defaults m_defaults;

    double m_maxMagnitude = qftbx::Settings().limits.maxMagnitude;
    double m_maxPointCount = qftbx::Settings().limits.maxTemplatePoints;

};

}

#endif
