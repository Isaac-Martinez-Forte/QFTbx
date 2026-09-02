#ifndef QFTBX_SPECIFICATIONS_DIALOG_H
#define QFTBX_SPECIFICATIONS_DIALOG_H

#include <memory>

#include <optional>
#include <vector>

#include <QDialog>
#include <QPixmap>

#include "src/core/math/sequence_vectors.h"
#include "src/core/specifications/specification_record.h"
#include "mpParser.h"
#include "src/core/frequencies/omega.h"

using namespace tools;

namespace Ui {
class SpecificationsDialog;
}

class SpecificationsDialog : public QDialog
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
     */
    explicit SpecificationsDialog(const QVector<qreal> * frequencies,
                                  const qftbx::SpecificationRecords * loaded = nullptr,
                                  QWidget *parent = 0);
    ~SpecificationsDialog();


    bool getTodoCorrecto ();

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

    qint32 activeTab;

    bool getDatos(qftbx::SpecificationRecord & record_in, QString name_in);
    bool getDatos(qftbx::SpecificationRecord & record_in, qftbx::SpecificationRecord & upperRecord,
                  QString name_in);
    void setDatos (qftbx::SpecificationRecord & record_in);
    void setDatos (qftbx::SpecificationRecord & record_in, qftbx::SpecificationRecord & upperRecord);
    void saveActiveTab();
    void discardPublished();

    std::optional<std::vector<Parameter>> buildParameters(QString linea);
    std::optional<Parameter> buildScalar(QString linea, bool isK);

    static QString coefficientsText(std::vector<Parameter> & parametros);
    static QString numeratorText(LtiSystem * sistema);
    static QString denominatorText(LtiSystem * sistema);

    //images
    QPixmap trackingImagePixmap;
    QPixmap controlEffortPixmap;
    QPixmap outputDisturbancePixmap;
    QPixmap inputDisturbancePixmap;
    QPixmap sensorNoisePixmap;
    QPixmap stabilityPixmap;

    const QVector <qreal> * frequencies;

    bool todoCorrecto;
};


#endif // QFTBX_SPECIFICATIONS_DIALOG_H
