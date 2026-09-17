#ifndef QFTBX_CONTROLLER_FORM_H
#define QFTBX_CONTROLLER_FORM_H

#include <memory>
#include <optional>

#include <QWidget>

#include "src/core/system/lti_system.h"
#include "src/gui/common/coefficient_tables.h"
#include "src/gui/application/step_panel.h"
#include "src/gui/common/system_description_reader.h"
#include "src/gui/plant/uncertainty_panel.h"

namespace Ui {
class ControllerForm;
}

namespace qftbx {

/**
 * @brief Step 6 of the design: the controller structure, with the search box
 * of every parameter and of the gain.
 *
 * Like the plant form in everything: what is typed is read as it is typed
 * and marked where it is wrong, the freedom of the structure is a page of
 * this form and not a window over it, and the one button verifies before it
 * applies - what verifying shows is the structure drawn as the formula it
 * is. The fields are read by SystemDescriptionReader, shared between the
 * two forms.
 */
class ControllerForm : public StepPanel
{
    Q_OBJECT

public:
    explicit ControllerForm(QWidget *parent = nullptr);
    ~ControllerForm();

    /// The controller structure the user described (with the search box of
    /// its parameters), or nullptr when cancelled or rejected. Ownership
    /// passes to the caller.
    std::unique_ptr<LtiSystem> takeControllerStructure();

    /**
     * @brief Shows the structure the project holds: the family, the
     * coefficients and the search box of the gain.
     *
     * As in the plant form, the structure's own parameters answer for the
     * search box while the fields still describe them.
     */
    void setFromProject(LtiSystem * structure);

private slots:
    void on_uncertaintyButton_clicked();
    void on_okButton_clicked();

    /// A radio: the family being described has changed.
    void familyChosen();

    /// Any field: what was verified no longer describes what is on screen.
    void fieldEdited();

private:
    /// The family the path of radios selects, or nothing while the path is
    /// unfinished.
    std::optional<LtiSystem::SystemType> selectedType() const;

    /// The labels, the hint and the figure of the family in use.
    void showFamily();

    /// The structure the fields describe, or nullptr when they do not
    /// describe one; the reason is already on screen.
    std::unique_ptr<LtiSystem> build();

    /// Whether the freedom panel answers for the coefficients now on screen.
    bool freedomIsCurrent() const;

    /// The verified structure, its formula and the button that applies it -
    /// or none of the three.
    void setVerified(std::unique_ptr<LtiSystem> structure);

    void say(const QString & complaint);

    /// The coefficients of the described controller, or nothing when the
    /// form could not read them (the reader has already said why).
    std::optional<CoefficientTable> readTables(CoefficientTable & expressionTable,
                                               UncertainTable & uncertainTable);

    /// The coefficient fields as one text, and the two ends of the gain box
    /// as another: what tells a form still showing the project's structure
    /// from one the user has edited. Separate, because editing a
    /// coefficient says nothing about the gain.
    QString currentCoefficients() const;
    QString currentGain() const;

    std::unique_ptr<Ui::ControllerForm> ui;

    UncertaintyPanel * m_freedom = nullptr;

    /// What Verify built and Apply hands over.
    std::unique_ptr<LtiSystem> m_verified;
    std::unique_ptr<LtiSystem> m_applied;

    /// The coefficients the freedom panel was last applied over: an edit
    /// since makes its ranges answer for something else.
    QString m_freedomCoefficients;

    //True while setFromProject is writing the fields: what it writes is not
    //an edit.
    bool m_filling = false;

    /// The two of them as setFromProject left them, empty when the form was
    /// not filled from a project.
    QString m_describedCoefficients;
    QString m_describedGain;

    /// The gain of the structure the form was filled from. The two fields
    /// are the ends of its search box and nothing else - not its name, not
    /// where inside the box it sits - so it only survives a trip through
    /// the form by being kept.
    std::optional<Parameter> m_projectGain;

    SystemDescriptionReader m_reader;
};

} // namespace qftbx

#endif // QFTBX_CONTROLLER_FORM_H
