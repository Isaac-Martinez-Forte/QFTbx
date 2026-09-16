#ifndef QFTBX_CONTROLLER_DIALOG_H
#define QFTBX_CONTROLLER_DIALOG_H

#include <memory>
#include <optional>

#include <QWidget>

#include "src/core/system/lti_system.h"
#include "src/gui/common/coefficient_tables.h"
#include "src/gui/application/step_panel.h"
#include "src/gui/common/system_description_reader.h"
#include "src/gui/plant/uncertainty_dialog.h"

namespace Ui {
class ControllerDialog;
}

namespace qftbx {

/**
 * @brief Step 6 of the design: the controller structure, with the search box
 * of every parameter and of the gain.
 *
 * Like the plant dialog, it builds a system and hands it over; the fields
 * are read by SystemDescriptionReader, shared between the two.
 */
class ControllerDialog : public StepPanel
{
    Q_OBJECT

public:
    explicit ControllerDialog(QWidget *parent = nullptr);
    ~ControllerDialog();

    /// The controller structure the user described (with the search box of
    /// its parameters), or nullptr when cancelled or rejected. Ownership
    /// passes to the caller.
    std::unique_ptr<LtiSystem> takeControllerStructure();

    /**
     * @brief Shows the structure the project holds: the family, the
     * coefficients and the search box of the gain.
     *
     * As in the plant dialog, the structure's own parameters answer for the
     * search box while the fields still describe them.
     */
    void setFromProject(LtiSystem * structure);

private slots:
    void on_polynomialRadio_clicked();
    void on_zpkRadio_clicked();
    void on_tcgRadio_clicked();
    void on_uncertaintyButton_clicked();
    void on_okButton_clicked();

private:
    /// The family the radios select.
    LtiSystem::SystemType selectedType() const;

    /// The coefficients of the described controller, or nothing when the
    /// dialog could not read them (the reader has already said why).
    std::optional<CoefficientTable> readTables(CoefficientTable & expressionTable,
                                               UncertainTable & uncertainTable);

    /// The coefficient fields as one text, and the two ends of the gain box
    /// as another: what tells a form still showing the project's structure
    /// from one the user has edited. Separate, because editing a
    /// coefficient says nothing about the gain.
    QString currentCoefficients() const;
    QString currentGain() const;

    std::unique_ptr<Ui::ControllerDialog> ui;

    UncertaintyDialog * uncertaintyDialog = nullptr;

    std::unique_ptr<LtiSystem> controllerSystem;

    bool uncertaintyEntered = false;

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

#endif // QFTBX_CONTROLLER_DIALOG_H
