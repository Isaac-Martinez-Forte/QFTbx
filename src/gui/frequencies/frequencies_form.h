/**
 * @file
 * @brief The step that asks for the design frequencies of the whole design.
 *
 * Declares the panel where the set is entered linearly, logarithmically,
 * by hand or from a file. The panel knows nothing of the project: it
 * builds the set and hands it over. A ceiling on the number of
 * frequencies comes from the settings. Reopening a design shows the rule
 * it was generated with, and the values themselves on the manual page.
 */

#ifndef QFTBX_FREQUENCIES_FORM_H
#define QFTBX_FREQUENCIES_FORM_H

#include "src/core/project/settings.h"
#include "src/gui/application/step_panel.h"
#include <memory>

#include <QWidget>
#include <QString>
#include <QFileDialog>
#include <QDoubleValidator>
#include <QVector>
#include <QMessageBox>
#include <QTextStream>

#include "src/core/math/sequence_vectors.h"
#include "src/core/frequencies/omega.h"

namespace Ui {
class FrequenciesForm;
}

namespace qftbx {

/**
 * @brief Step 2 of the design: the set of design frequencies (Omega) the
 * whole pipeline is computed at, entered linearly, logarithmically, by
 * hand or from a file.
 *
 * @author Isaac Martínez Forte
 */
class FrequenciesForm : public StepPanel
{
    Q_OBJECT

public:
    /// Ceiling on the number of design frequencies, from the settings.
    void applyFrequencyCountLimit(std::int32_t count);

  /// The dialog knows nothing of the project: it builds a frequency set
  /// and takeOmega() hands it over.
    explicit FrequenciesForm(QWidget *parent = 0);

    /// The design frequencies the user described, or nullptr when cancelled
    /// or rejected. Ownership passes to the caller.
    std::unique_ptr<Omega> takeOmega();

    /**
     * @brief Fills the fields with the frequency set the project holds, so
     * that opening a design shows what it was designed with.
     *
     * A null set leaves the dialog as it is: there is nothing to show yet.
     * The mode comes from how the set was generated, and the manual page
     * lists the values themselves, which is what a set read from a file or
     * typed by hand has instead of a rule.
     */
    void setFromProject(const Omega * omega);
    ~FrequenciesForm();

private slots:

    void on_fileButton_clicked();

    void on_okButton_clicked();

private:
    std::unique_ptr<Omega> m_omega;
    QString filePath;

    std::unique_ptr<Ui::FrequenciesForm> ui;

    std::int32_t m_maxFrequencyCount = qftbx::Settings().limits.maxFrequencyCount;

};

}

#endif
