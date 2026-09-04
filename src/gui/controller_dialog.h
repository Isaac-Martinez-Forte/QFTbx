#ifndef QFTBX_CONTROLLER_DIALOG_H
#define QFTBX_CONTROLLER_DIALOG_H

#include <memory>
#include <optional>

#include <QDialog>

#include "src/core/system/lti_system.h"
#include "src/gui/coefficient_tables.h"
#include "src/gui/step_dialog.h"
#include "src/gui/system_description_reader.h"
#include "src/gui/uncertainty_dialog.h"

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
class ControllerDialog : public StepDialog
{
    Q_OBJECT

public:
    explicit ControllerDialog(QWidget *parent = nullptr);
    ~ControllerDialog();

    /// The controller structure the user described (with the search box of
    /// its parameters), or nullptr when cancelled or rejected. Ownership
    /// passes to the caller.
    std::unique_ptr<LtiSystem> takeControllerStructure();

private slots:
    void on_polynomialRadio_clicked();
    void on_zpkRadio_clicked();
    void on_tcgRadio_clicked();
    void on_uncertaintyButton_clicked();
    void on_cancelButton_clicked();
    void on_okButton_clicked();

private:
    /// The family the radios select.
    LtiSystem::SystemType selectedType() const;

    /// The coefficients of the described controller, or nothing when the
    /// dialog could not read them (the reader has already said why).
    std::optional<CoefficientTable> readTables(CoefficientTable & expressionTable,
                                               UncertainTable & uncertainTable);

    std::unique_ptr<Ui::ControllerDialog> ui;

    UncertaintyDialog * uncertaintyDialog = nullptr;

    std::unique_ptr<LtiSystem> controllerSystem;

    bool uncertaintyEntered = false;

    SystemDescriptionReader m_reader;
};

} // namespace qftbx

#endif // QFTBX_CONTROLLER_DIALOG_H
