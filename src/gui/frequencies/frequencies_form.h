/**
 * @file
 * @brief The step that asks for the design frequencies of the whole design.
 *
 * Declares the panel of step 2, where the set the whole pipeline is
 * computed at is entered linearly, logarithmically, by hand or from a
 * file. The panel knows nothing of the project: it builds the set and
 * takeOmega() hands it over, passing ownership to the caller, or gives
 * null when the user cancelled or the set was rejected. A ceiling on the
 * number of frequencies comes from the settings. Reopening a design shows
 * the rule it was generated with, and the values themselves on the manual
 * page; a null set leaves the fields as they are.
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

class FrequenciesForm : public StepPanel
{
    Q_OBJECT

public:
    void applyFrequencyCountLimit(std::int32_t count);

    explicit FrequenciesForm(QWidget *parent = 0);

    std::unique_ptr<Omega> takeOmega();

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
