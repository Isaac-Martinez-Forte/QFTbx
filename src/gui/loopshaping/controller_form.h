/**
 * @file
 * @brief The step that describes the controller structure and its search box.
 *
 * Declares the panel where the structure is written in one of the families,
 * with a name in place of any coefficient the search may move and the two
 * ends of the gain's box. Fields are read as they are typed and marked
 * where wrong; the freedom of the parameters is a page of the form, and the
 * one button verifies before it applies, showing the structure drawn as
 * the formula it is. The fields are read by the description reader shared
 * with the plant form.
 *
 * takeControllerStructure() hands the caller the structure applied, with
 * the search box of its parameters, or null when cancelled or rejected.
 * After setFromProject() the structure's own parameters and gain answer for
 * the search box as long as the fields still describe them: the two gain
 * fields are only the ends of its box, so the gain's name and its place in
 * the box survive only by being kept. Coefficients and gain are compared
 * apart, since editing a coefficient says nothing about the gain, and the
 * freedom page answers only for the coefficients it was last applied over.
 */

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

class ControllerForm : public StepPanel
{
    Q_OBJECT

public:
    explicit ControllerForm(QWidget *parent = nullptr);
    ~ControllerForm();

    std::unique_ptr<LtiSystem> takeControllerStructure();

    void setFromProject(LtiSystem * structure);

private slots:
    void on_uncertaintyButton_clicked();
    void on_okButton_clicked();

    void familyChosen();

    void fieldEdited();

private:
    std::optional<LtiSystem::SystemType> selectedType() const;

    void showFamily();

    std::unique_ptr<LtiSystem> build();

    bool freedomIsCurrent() const;

    void setVerified(std::unique_ptr<LtiSystem> structure);

    void say(const QString & complaint);

    std::optional<CoefficientTable> readTables(CoefficientTable & expressionTable,
                                               UncertainTable & uncertainTable);

    QString currentCoefficients() const;
    QString currentGain() const;

    std::unique_ptr<Ui::ControllerForm> ui;

    UncertaintyPanel * m_freedom = nullptr;

    std::unique_ptr<LtiSystem> m_verified;
    std::unique_ptr<LtiSystem> m_applied;

    QString m_freedomCoefficients;

    bool m_filling = false;

    QString m_describedCoefficients;
    QString m_describedGain;

    std::optional<Parameter> m_projectGain;

    SystemDescriptionReader m_reader;
};

}

#endif
