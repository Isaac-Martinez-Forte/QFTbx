#ifndef QFTBX_LOOP_SHAPING_DIALOG_H
#define QFTBX_LOOP_SHAPING_DIALOG_H

#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/project/settings.h"
#include "src/gui/application/step_dialog.h"
#include <memory>

#include <QDialog>

#include "src/core/math/range.h"

#include "mpParser.h"
#include "src/core/math/sequence_vectors.h"

namespace Ui {
class LoopShapingDialog;
}

namespace qftbx {


/**
 * @brief Step 7 of the design: picks one of the five loop-shaping
 * algorithms and the accuracy to run it to.
 *
 * What the accuracy measures depends on the algorithm chosen, because each
 * follows the criterion of its own paper - see
 * ProjectController::computeLoopShaping. The single field does not say so
 * yet.
 */
class LoopShapingDialog : public StepDialog
{
    Q_OBJECT

public:
    /**
     * @brief The ceilings on what its fields accept, from the settings.
     *
     * They exist to keep a typo from reaching a conversion or an allocation,
     * not to express any control-design limit, so moving them changes no
     * computed result.
     */
    void setLimits(double maxMagnitude, double maxPointCount)
    { m_maxMagnitude = maxMagnitude; m_maxPointCount = maxPointCount; }

    /**
     * @brief Prefills the range fields from the settings.
     *
     * ONE range, used on opening and on picking either mode. There were three
     * hardcoded sets before - one per place - and the two mode ones differed
     * from each other and from the opening one with no reason given anywhere.
     * Worse, they overwrote whatever the fields held, so a configured default
     * would have been thrown away the moment a mode was picked.
     */
    void applyDefaults(const qftbx::Settings::Defaults & defaults);

    explicit LoopShapingDialog(QWidget *parent = 0);
    ~LoopShapingDialog();

    qreal epsilonValue ();

    qftbx::LoopShapingAlgorithm algorithmValue();

    qftbx::Range range();

    qreal pointCountValue();

    bool isLinSpace();

    qint32 initialisationValue ();

private slots:
    /// Says which epsilon the field is asking for, because it is not the
    /// same quantity for every algorithm.
    void updateEpsilonLabel();

    void on_cancelButton_clicked();

    void on_okButton_clicked();

    void on_linspaceRadio_clicked();

    void on_logspaceRadio_clicked();

    void on_ntRadio_clicked();

    void on_nkRadio_clicked();

    void on_mrRadio_clicked();
    void on_mc1Radio_clicked();
    void on_mcThesisRadio_clicked();

private:
    std::unique_ptr<Ui::LoopShapingDialog> ui;


    qreal epsilonEdit = 0.0;

    qftbx::Range plotRange;

    qreal pointCountEdit = 0.0;

    qint32 initialisation = 0;

    qftbx::LoopShapingAlgorithm alg = qftbx::nt;

    bool linLogSpace = false;
    /// Kept because the mode radios prefill from it too.
    qftbx::Settings::Defaults m_defaults;

    double m_maxMagnitude = qftbx::Settings().limits.maxMagnitude;
    double m_maxPointCount = qftbx::Settings().limits.maxTemplatePoints;

};

} // namespace qftbx

#endif // QFTBX_LOOP_SHAPING_DIALOG_H
