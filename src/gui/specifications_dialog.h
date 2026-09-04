#ifndef QFTBX_SPECIFICATIONS_DIALOG_H
#define QFTBX_SPECIFICATIONS_DIALOG_H

#include "src/gui/step_dialog.h"
#include <memory>

#include <optional>
#include <vector>

#include <QDialog>
#include <QPixmap>

#include "src/core/math/sequence_vectors.h"
#include "src/core/specifications/specification_record.h"
#include "mpParser.h"
#include "src/core/frequencies/omega.h"

namespace Ui {
class SpecificationsDialog;
}

class SpecificationsDialog : public StepDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor. The dialog knows nothing of the project: it is
     * given what it needs to read and takeSpecifications() hands over what
     * the user described.
     *
     * @param frequencies the design frequencies, whose ends are the default
     * band of every specification. Must not be null or empty.
     * @param loaded the 7 records already in the project, if any, so that
     * reopening the dialog starts from them instead of from blanks.
     * @param parent the Qt parent.
     */
    explicit SpecificationsDialog(const std::vector<double> * frequencies,
                                  const qftbx::SpecificationRecords * loaded = nullptr,
                                  QWidget *parent = 0);
    ~SpecificationsDialog();



    /**
     * @brief Points the dialog at the project's CURRENT design frequencies.
     *
     * The dialog outlives the frequency set it was built with. Entering new
     * frequencies destroys the Omega that owns the values, and this dialog is
     * not one of the things rebuilt when that happens, so the pointer taken
     * in the constructor was left dangling: the next accept read freed memory
     * through it, because an empty band field defaults to the first and last
     * design frequency. Every other dialog and viewer is handed its data
     * again right before it is shown, and this one now is too.
     *
     * Throws qftbx::InvalidInput when there are no frequencies, like the
     * constructor.
     */
    void setFrequencies(const std::vector<double> * frequencies);

    /**
     * @brief The 7 specification records the user described, or nullptr when
     * the dialog was cancelled or its data rejected. Ownership of the vector
     * and of every record in it passes to the caller.
     */
    std::optional<qftbx::SpecificationRecords> takeSpecifications();

private slots:
    void on_polynomialRadio_clicked();

    void on_tcgRadio_clicked();

    void on_zpkRadio_clicked();

    void on_trackingRadio_clicked();

    void on_stabilityRadio_clicked();

    void on_noiseRadio_clicked();

    void on_outputDisturbanceRadio_clicked();

    void on_inputDisturbanceRadio_clicked();

    void on_controlEffortRadio_clicked();

    void on_constantRadio_clicked();

    void on_systemRadio_clicked();

    void on_cancelButton_clicked();

    void on_okButton_clicked();

    void on_freeFormRadio_clicked();

    void on_lowerPolynomialRadio_clicked();

    void on_lowerFreeFormRadio_clicked();

    void on_lowerZpkRadio_clicked();

    void on_lowerTcgRadio_clicked();

    void on_upperPolynomialRadio_clicked();

    void on_upperZpkRadio_clicked();

    void on_upperTcgRadio_clicked();

    void on_upperFreeFormRadio_clicked();

private:
    std::unique_ptr<Ui::SpecificationsDialog> ui;

    //The seven working records, by value: the dialog edits them and
    //publishes deep clones.
    qftbx::SpecificationRecord tracking;
    qftbx::SpecificationRecord trackingUpper;
    qftbx::SpecificationRecord stability;
    qftbx::SpecificationRecord sensorNoise;
    qftbx::SpecificationRecord outputDisturbance;
    qftbx::SpecificationRecord inputDisturbance;
    qftbx::SpecificationRecord controlEffort;

    std::optional<qftbx::SpecificationRecords> published;

    qint32 activeTab = 0;

    bool data(qftbx::SpecificationRecord & record_in, QString name_in);
    bool data(qftbx::SpecificationRecord & record_in, qftbx::SpecificationRecord & upperRecord,
                  QString name_in);
    void setData (qftbx::SpecificationRecord & record_in);
    void setData (qftbx::SpecificationRecord & record_in, qftbx::SpecificationRecord & upperRecord);
    /**
     * @brief Reads the tab being left into its record.
     *
     * False when the tab could not be read: data() has emptied the
     * record it could not fill and marked the offending field, so the
     * caller must NOT switch away.
     */
    bool saveActiveTab();

    /// saveActiveTab() plus the refusal: puts the selection back on the tab
    /// that failed and says why. False means stay where you are.
    bool leaveActiveTab();

    /// Re-checks the radio of the tab in activeTab. setChecked() does not
    /// emit clicked(), so this does not re-enter the handlers.
    void restoreActiveTabRadio();
    void discardPublished();

    std::optional<std::vector<Parameter>> buildParameters(QString linea);
    std::optional<Parameter> buildScalar(QString linea, bool isK);

    static QString coefficientsText(std::vector<Parameter> & parameters);
    static QString numeratorText(LtiSystem * system);
    static QString denominatorText(LtiSystem * system);

    //images
    QPixmap trackingImagePixmap;
    QPixmap controlEffortPixmap;
    QPixmap outputDisturbancePixmap;
    QPixmap inputDisturbancePixmap;
    QPixmap sensorNoisePixmap;
    QPixmap stabilityPixmap;

    const std::vector<double> * frequencies;

};


#endif // QFTBX_SPECIFICATIONS_DIALOG_H
