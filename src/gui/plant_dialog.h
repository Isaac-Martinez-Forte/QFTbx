#ifndef QFTBX_PLANT_DIALOG_H
#define QFTBX_PLANT_DIALOG_H

#include <memory>

#include <QDialog>
#include <QString>

#include "src/core/system/lti_system.h"
#include "src/gui/coefficient_tables.h"
#include "src/gui/step_dialog.h"
#include "src/gui/system_description_reader.h"
#include "src/gui/uncertainty_dialog.h"

namespace Ui {
class PlantDialog;
}

namespace qftbx {

/**
 * @brief Step 1 of the design: the plant, in one of the four families, with
 * its uncertain parameters described through the uncertainty dialog.
 *
 * The dialog does not know the project: it builds a plant and hands it over
 * through takePlant(), and the main window publishes it. Reading the fields
 * is SystemDescriptionReader's job, shared with the controller dialog.
 */
class PlantDialog : public StepDialog
{
    Q_OBJECT

public:
    explicit PlantDialog(QWidget *parent = nullptr);
    ~PlantDialog();

    /// The plant the user described, or nullptr when the dialog was
    /// cancelled or its data rejected. Ownership passes to the caller.
    std::unique_ptr<LtiSystem> takePlant();

private slots:
    void on_zerosPolesRadio_toggled(bool checked);
    void on_transferFunctionRadio_toggled(bool checked);
    void on_zpkRadio_toggled(bool checked);
    void on_tcgRadio_toggled(bool checked);
    void on_polynomialRadio_toggled(bool checked);
    void on_okButton_clicked();
    void on_uncertaintyButton_clicked();
    void on_freeFormRadio_clicked();

private:
    /// The family the radios select.
    LtiSystem::SystemType selectedType() const;

    /// The coefficients of the described plant, or nothing when the dialog
    /// could not read them (the reader has already said why).
    std::optional<CoefficientTable> readTables(CoefficientTable & expressionTable,
                                               UncertainTable & uncertainTable);

    /// The name field, marked red and reported when empty.
    bool nameIsPresent();

    std::unique_ptr<Ui::PlantDialog> ui;

    UncertaintyDialog * uncertaintyDialog = nullptr;

    std::unique_ptr<LtiSystem> plant;

    bool uncertaintyEntered = false;

    SystemDescriptionReader m_reader;
};

} // namespace qftbx

#endif // QFTBX_PLANT_DIALOG_H
