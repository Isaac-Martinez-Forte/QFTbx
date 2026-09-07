#ifndef QFTBX_FREQUENCIES_DIALOG_H
#define QFTBX_FREQUENCIES_DIALOG_H

#include "src/core/project/settings.h"
#include "src/gui/application/step_dialog.h"
#include <memory>

#include <QDialog>
#include <QString>
#include <QFileDialog>
#include <QDoubleValidator>
#include <QVector>
#include <QMessageBox>
#include <QTextStream>

#include "src/core/math/sequence_vectors.h"
#include "src/core/frequencies/omega.h"

namespace Ui {
class FrequenciesDialog;
}

namespace qftbx {


/**
 * @brief Step 2 of the design: the set of design frequencies (Omega) the
 * whole pipeline is computed at, entered linearly, logarithmically, by
 * hand or from a file.
 *
 * @author Isaac Martínez Forte
 */
class FrequenciesDialog : public StepDialog
{
    Q_OBJECT
    
public:
    /// Ceiling on the number of design frequencies, from the settings.
    void applyFrequencyCountLimit(std::int32_t count);

  
  /// The dialog knows nothing of the project: it builds a frequency set
  /// and takeOmega() hands it over.
    explicit FrequenciesDialog(QWidget *parent = 0);

    /// The design frequencies the user described, or nullptr when cancelled
    /// or rejected. Ownership passes to the caller.
    std::unique_ptr<Omega> takeOmega();
    ~FrequenciesDialog();


    
private slots:

    void on_fileButton_clicked();

    void on_okButton_clicked();

signals:
    void close_ok ();

private:
    std::unique_ptr<Omega> m_omega;
    QString filePath;

    std::unique_ptr<Ui::FrequenciesDialog> ui;

    std::int32_t m_maxFrequencyCount = qftbx::Settings().limits.maxFrequencyCount;

};

} // namespace qftbx

#endif // QFTBX_FREQUENCIES_DIALOG_H
