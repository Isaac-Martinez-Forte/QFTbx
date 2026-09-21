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

/**
 * @brief Step 3 of the design: the specifications, one form and the list of
 * the ones already entered.
 *
 * One form describes ONE specification, the button verifies it before it
 * can be added, and what is
 * added goes to the list beside it, with the bound drawn as the formula it
 * is. From the list a specification is edited again or removed.
 *
 * The seven positional slots are the model underneath - every
 * consumer indexes them by SpecificationType - and the list is the used
 * ones.
 *
 * The form knows nothing of the project: it is given what it needs and
 * takeSpecifications() hands over what the user applied.
 */
class SpecificationsForm : public StepPanel
{
    Q_OBJECT

public:
    /**
     * @param frequencies the design frequencies, whose ends are the default
     * band of every specification. Must not be null or empty.
     * @param loaded the 7 records already in the project, if any, so that
     * reopening the form starts from them instead of from blanks.
     * @param parent the Qt parent.
     */
    explicit SpecificationsForm(const std::vector<double> * frequencies,
                                const qftbx::SpecificationRecords * loaded = nullptr,
                                QWidget * parent = nullptr);
    ~SpecificationsForm();

    /**
     * @brief Points the form at the project's CURRENT design frequencies.
     *
     * The form outlives the frequency set it was built with. Entering new
     * frequencies destroys the Omega that owns the values, and this form is
     * not one of the things rebuilt when that happens, so the pointer taken
     * in the constructor was left dangling: the next accept read freed
     * memory through it, because an empty band field defaults to the first
     * and last design frequency.
     *
     * A null or empty set is accepted here, unlike in the constructor: the
     * panel stays open while the project changes, and the frequencies it
     * was built on can be taken away under it. It refuses to publish until
     * there are frequencies again, rather than reading a vector that has
     * been freed.
     */
    void setFrequencies(const std::vector<double> * frequencies);

    /**
     * @brief The 7 specification records the user applied, or nothing when
     * none were. Ownership of the records passes to the caller.
     */
    std::optional<qftbx::SpecificationRecords> takeSpecifications();

private slots:
    void on_addButton_clicked();
    void on_clearButton_clicked();
    void on_editButton_clicked();
    void on_removeButton_clicked();
    void on_okButton_clicked();

    /// The kind of bound, the family of its system: what the form shows.
    void showBound();

    /// A field changed: what was verified no longer describes it.
    void fieldEdited();

    /// Another specification chosen in the combo: its figure, and the
    /// record it already has, if any.
    void typeChosen();

private:
    /// The type the combo has, as the slot it indexes.
    qftbx::SpecificationType selectedType() const;

    /// The record the form describes, or nothing when it does not describe
    /// one; the reason is already on screen.
    std::optional<qftbx::SpecificationRecord> build();

    /// The verified record, its formula and the button that adds it - or
    /// none of the three.
    void setVerified(std::optional<qftbx::SpecificationRecord> record);

    /// The form over one record: the type's own, or an empty one.
    void showRecord(qftbx::SpecificationType type);

    /// One tick box per design frequency, rebuilt when the project's
    /// frequencies change.
    void buildFrequencyRows();

    /// Ticks the frequencies a record applies at; a record that is not used
    /// yet applies at all of them.
    void showFrequenciesOf(const qftbx::SpecificationRecord & record);

    /// The band and the exceptions the ticks describe: the band is from the
    /// first ticked frequency to the last, and what is unticked between
    /// them is what the specification skips. False when nothing is ticked.
    bool readFrequencies(qftbx::SpecificationRecord & record);

    /// The list of the used specifications, rebuilt.
    void showTable();

    /// The bound of a record as a formula: the transfer function it is, or
    /// the constant it is.
    Formula boundOf(const qftbx::SpecificationRecord & record) const;

    /// And the whole requirement: the magnitude the program checks, the
    /// sign, and that bound. The type is given, not read off the form: the
    /// list draws every slot, not the one being edited.
    Formula formulaOfRecord(qftbx::SpecificationType type,
                            const qftbx::SpecificationRecord & record) const;

    void say(const QString & complaint);

    /// One coefficient line as constants, or nothing when a token is not a
    /// valid finite expression.
    std::optional<std::vector<Parameter>> parametersFrom(const QString & text);

    /// A gain or delay field: 'fallback' when empty, nothing when it does
    /// not evaluate to a finite number.
    std::optional<Parameter> scalarFrom(const QString & text, double fallback);

    std::unique_ptr<Ui::SpecificationsForm> ui;

    /// The seven working slots, by value: the form edits them and publishes
    /// deep clones.
    qftbx::SpecificationRecords m_records;

    /// What the button would add, once it has been verified.
    std::optional<qftbx::SpecificationRecord> m_verified;

    std::optional<qftbx::SpecificationRecords> m_published;

    /// True while the form is writing its own fields.
    bool m_filling = false;

    /// One tick per design frequency, which is what a band is asked through:
    /// the numbers are not typed, they are chosen from the ones the design
    /// actually uses.
    FrequencyLegend * m_frequencies = nullptr;

    SystemDescriptionReader m_reader;

    const std::vector<double> * m_design = nullptr;
};

}

#endif
