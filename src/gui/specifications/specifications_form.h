/**
 * @file
 * @brief The step that enters the specifications and lists those entered.
 *
 * Declares the panel where one specification is described at a time, its
 * band chosen by ticking design frequencies, verified before it is added,
 * and listed beside the form with its bound drawn as a formula; from the
 * list it is edited again or removed. Underneath are the seven positional
 * records every consumer indexes by type, and the list is the used ones.
 * The form is pointed at the project's current frequencies and refuses to
 * publish while there are none; it hands over what the user applied.
 *
 * The form knows nothing of the project. Its constructor needs frequencies,
 * neither null nor empty, whose ends are the default band, and starts from
 * the records already in the project when given them. The form outlives
 * the frequency set it was built on, which entering new frequencies
 * destroys, so setFrequencies() must point it at the new one; that accepts
 * a null or empty set. A band runs from the first ticked frequency to the
 * last, the unticked ones between are skipped, and a record not used yet
 * applies at all of them. The form edits its own records and publishes
 * deep clones, which takeSpecifications() hands to the caller, or nothing
 * when none were applied.
 */

#ifndef QFTBX_SPECIFICATIONS_FORM_H
#define QFTBX_SPECIFICATIONS_FORM_H

#include <memory>
#include <optional>
#include <vector>

#include <QLineEdit>
#include <QString>
#include <QWidget>

#include "src/core/math/formula.h"
#include "src/core/specifications/specification_record.h"
#include "src/gui/application/step_panel.h"
#include "src/gui/common/frequency_legend.h"
#include "src/gui/common/system_description_reader.h"

namespace Ui {
class SpecificationsForm;
}

namespace qftbx {

class SpecificationsForm : public StepPanel
{
    Q_OBJECT

public:
    explicit SpecificationsForm(const std::vector<double> * frequencies,
                                const qftbx::SpecificationRecords * loaded = nullptr,
                                QWidget * parent = nullptr);
    ~SpecificationsForm();

    void setFrequencies(const std::vector<double> * frequencies);

    std::optional<qftbx::SpecificationRecords> takeSpecifications();

private slots:
    void on_addButton_clicked();
    void on_clearButton_clicked();
    void on_editButton_clicked();
    void on_removeButton_clicked();
    void on_okButton_clicked();

    void showBound();

    void fieldEdited();

    void typeChosen();

private:
    qftbx::SpecificationType selectedType() const;

    std::optional<qftbx::SpecificationRecord> build();

    void setVerified(std::optional<qftbx::SpecificationRecord> record);

    void showRecord(qftbx::SpecificationType type);

    void buildFrequencyRows();

    void showFrequenciesOf(const qftbx::SpecificationRecord & record);

    bool readFrequencies(qftbx::SpecificationRecord & record);

    void showTable();

    Formula boundOf(const qftbx::SpecificationRecord & record) const;

    Formula formulaOfRecord(qftbx::SpecificationType type,
                            const qftbx::SpecificationRecord & record) const;

    void say(const QString & complaint);

    std::optional<std::vector<Parameter>> parametersFrom(const QString & text);

    std::optional<Parameter> scalarFrom(const QString & text, double fallback);

    std::unique_ptr<Ui::SpecificationsForm> ui;

    qftbx::SpecificationRecords m_records;

    std::optional<qftbx::SpecificationRecord> m_verified;

    std::optional<qftbx::SpecificationRecords> m_published;

    bool m_filling = false;

    FrequencyLegend * m_frequencies = nullptr;

    SystemDescriptionReader m_reader;

    const std::vector<double> * m_design = nullptr;
};

}

#endif
