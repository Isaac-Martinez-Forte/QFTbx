/**
 * @file
 * @brief The step that describes the plant and its uncertain parameters.
 *
 * Declares the panel where the plant is written in one of the four
 * families, with a name in place of any uncertain coefficient. What is
 * typed is read as it is typed and the field that cannot be read is
 * marked with the reason; the uncertainty is a page of the form; and the
 * one button verifies before it applies, drawing the formula where the
 * figure of the family was.
 *
 * The form knows nothing of the project: takePlant hands over the plant
 * applied, or null, and the caller owns it. setFromProject writes the form
 * back from a plant, and while the fields stay as it left them the plant's
 * own parameters answer for the uncertainty; the first edit makes the
 * uncertainty be read from the fields again, the coefficients and the gain
 * and delay each on their own, so that editing one keeps the other. The
 * gain and delay parameters of that plant are kept whole, since the form
 * has no field for their names and builds "k" and "delay". The fields are
 * read by the description reader shared with the controller form.
 */

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

class PlantForm : public StepPanel
{
    Q_OBJECT

public:
    explicit PlantForm(QWidget * parent = nullptr);
    ~PlantForm();

    std::unique_ptr<LtiSystem> takePlant();

    void setFromProject(LtiSystem * plant);

private slots:
    void on_okButton_clicked();
    void on_uncertaintyButton_clicked();

    void familyChosen();

    void fieldEdited();

private:
    std::optional<LtiSystem::SystemType> selectedType() const;

    void showFamily();

    std::optional<CoefficientTable> readTables(CoefficientTable & expressionTable,
                                               UncertainTable & uncertainTable);

    std::unique_ptr<LtiSystem> build();

    bool uncertaintyIsCurrent() const;

    void setVerified(std::unique_ptr<LtiSystem> plant);

    void say(const QString & complaint);

    QString currentCoefficients() const;
    QString currentScalars() const;

    std::unique_ptr<Ui::PlantForm> ui;

    UncertaintyPanel * m_uncertainty = nullptr;

    std::unique_ptr<LtiSystem> m_verified;
    std::unique_ptr<LtiSystem> m_applied;

    QString m_uncertaintyCoefficients;

    QString m_describedCoefficients;
    QString m_describedScalars;

    std::optional<Parameter> m_projectGain;
    std::optional<Parameter> m_projectDelay;

    bool m_filling = false;

    SystemDescriptionReader m_reader;
};

}

#endif
