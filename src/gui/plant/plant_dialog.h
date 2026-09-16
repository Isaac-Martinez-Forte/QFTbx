#ifndef QFTBX_PLANT_DIALOG_H
#define QFTBX_PLANT_DIALOG_H

#include <memory>
#include <optional>

#include <QWidget>
#include <QString>

#include "src/core/system/lti_system.h"
#include "src/gui/common/coefficient_tables.h"
#include "src/gui/application/step_panel.h"
#include "src/gui/common/system_description_reader.h"
#include "src/gui/plant/uncertainty_dialog.h"

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
class PlantDialog : public StepPanel
{
    Q_OBJECT

public:
    explicit PlantDialog(QWidget *parent = nullptr);
    ~PlantDialog();

    /// The plant the user described, or nullptr when the dialog was
    /// cancelled or its data rejected. Ownership passes to the caller.
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

    /// The two coefficient fields of the chosen family, as one text, and
    /// the gain and delay fields as another: what tells a form still
    /// showing the project's plant from one the user has edited. They are
    /// separate because they answer for different things - the
    /// coefficients for the polynomials, these two for the gain and the
    /// delay - and editing one must not throw the other away.
    QString currentCoefficients() const;
    QString currentScalars() const;

    std::unique_ptr<Ui::PlantDialog> ui;

    UncertaintyDialog * uncertaintyDialog = nullptr;

    std::unique_ptr<LtiSystem> plant;

    bool uncertaintyEntered = false;

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

    SystemDescriptionReader m_reader;
};

} // namespace qftbx

#endif // QFTBX_PLANT_DIALOG_H
