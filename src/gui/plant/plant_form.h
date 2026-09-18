#ifndef QFTBX_PLANT_FORM_H
#define QFTBX_PLANT_FORM_H

#include <memory>
#include <optional>

#include <QString>
#include <QWidget>

#include "src/core/system/lti_system.h"
#include "src/gui/application/step_panel.h"
#include "src/gui/common/coefficient_tables.h"
#include "src/gui/common/system_description_reader.h"
#include "src/gui/plant/uncertainty_panel.h"

namespace Ui {
class PlantForm;
}

namespace qftbx {

/**
 * @brief Step 1 of the design: the plant, in one of the four families, with
 * its uncertain parameters.
 *
 * Two things happen here before a plant leaves the form. What the user
 * types is READ as it is typed, and the field that cannot be read is marked
 * with the reason; and nothing is applied that has not been verified first,
 * which is why there is one button and it says Verify until it has anything
 * to apply. What verifying produces is the formula, drawn where the figure
 * of the family was: the answer to the only question a line of coefficients
 * raises, which is whether it means what it was meant to mean.
 *
 * The form does not know the project: it builds a plant and hands it over
 * through takePlant(), and the main window publishes it. Reading the fields
 * is SystemDescriptionReader's job, shared with the controller form.
 */
class PlantForm : public StepPanel
{
    Q_OBJECT

public:
    explicit PlantForm(QWidget * parent = nullptr);
    ~PlantForm();

    /// The plant the user applied, or nullptr when none was. Ownership
    /// passes to the caller.
    std::unique_ptr<LtiSystem> takePlant();

    /**
     * @brief Shows the plant the project holds: the family, the fields and
     * the uncertainty it was built with.
     *
     * A project carries a system, not the text it was typed as, so the form
     * is written back from it. While the fields stay as this left them, the
     * plant's own parameters answer for the uncertainty; the first edit
     * makes them stop describing what is on screen, and the uncertainty is
     * read from the fields again.
     */
    void setFromProject(LtiSystem * plant);

private slots:
    void on_okButton_clicked();
    void on_uncertaintyButton_clicked();

    /// A radio of any of the three levels: the path decides which levels
    /// are worth showing and which family is being described.
    void familyChosen();

    /// Any field: what was verified no longer describes what is on screen.
    void fieldEdited();

private:
    /// The family the path of radios selects, or nothing while the path is
    /// unfinished.
    std::optional<LtiSystem::SystemType> selectedType() const;

    /// The labels, the hint and the figure of the family in use.
    void showFamily();

    /// Reads the fields into the three tables, marking every field that
    /// cannot be read. Nothing when one of them could not.
    std::optional<CoefficientTable> readTables(CoefficientTable & expressionTable,
                                               UncertainTable & uncertainTable);

    /// The plant the fields describe, or nullptr when they do not describe
    /// one; the reason is already on screen.
    std::unique_ptr<LtiSystem> build();

    /// Whether the panel answers for the coefficients now on screen.
    bool uncertaintyIsCurrent() const;

    /// The verified plant, its formula and the button that applies it - or
    /// none of the three.
    void setVerified(std::unique_ptr<LtiSystem> plant);

    void say(const QString & complaint);

    /// The two coefficient fields as one text, and the gain and delay
    /// fields as another: what tells a form still showing the project's
    /// plant from one the user has edited. They are separate because they
    /// answer for different things - the coefficients for the polynomials,
    /// these two for the gain and the delay - and editing one must not
    /// throw the other away.
    QString currentCoefficients() const;
    QString currentScalars() const;

    std::unique_ptr<Ui::PlantForm> ui;

    UncertaintyPanel * m_uncertainty = nullptr;

    /// What Verify built and Apply hands over.
    std::unique_ptr<LtiSystem> m_verified;
    std::unique_ptr<LtiSystem> m_applied;

    /// The coefficients the uncertainty panel was last applied over: an
    /// edit since makes its ranges answer for something else.
    QString m_uncertaintyCoefficients;

    /// The two of them as setFromProject left them, empty when the form was
    /// not filled from a project.
    QString m_describedCoefficients;
    QString m_describedScalars;

    /// The gain and the delay of the plant the form was filled from. The
    /// form has no field for their NAMES - it builds "k" and "delay" - so a
    /// plant that calls its gain something else only survives a trip
    /// through the form by being kept.
    std::optional<Parameter> m_projectGain;
    std::optional<Parameter> m_projectDelay;

    //True while setFromProject is writing the fields: what it writes is not
    //an edit, and reading it back as one would throw away the parameters of
    //the plant it is showing.
    bool m_filling = false;

    SystemDescriptionReader m_reader;
};

} // namespace qftbx

#endif // QFTBX_PLANT_FORM_H
