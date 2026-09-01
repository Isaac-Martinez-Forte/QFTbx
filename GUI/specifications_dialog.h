#ifndef QFTBX_SPECIFICATIONS_DIALOG_H
#define QFTBX_SPECIFICATIONS_DIALOG_H

#include <QDialog>
#include <QPixmap>

#include "Modelo/Herramientas/tools.h"
#include "Modelo/controlador.h"
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
    explicit SpecificationsDialog(Controlador * controller, QWidget *parent = 0);
    ~SpecificationsDialog();


    bool getTodoCorrecto ();

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
    Ui::SpecificationsDialog *ui;

    qftbx::SpecificationRecord *tracking;
    qftbx::SpecificationRecord *trackingUpper;
    qftbx::SpecificationRecord *stability;
    qftbx::SpecificationRecord *sensorNoise;
    qftbx::SpecificationRecord *outputDisturbance;
    qftbx::SpecificationRecord *inputDisturbance;
    qftbx::SpecificationRecord *controlEffort;

    QVector <qftbx::SpecificationRecord *> * published;

    qint32 activeTab;

    bool getDatos(qftbx::SpecificationRecord * record_in, QString name_in);
    bool getDatos(qftbx::SpecificationRecord *record_in, qftbx::SpecificationRecord * upperRecord, QString name_in);
    void setDatos (qftbx::SpecificationRecord * record_in);
    void setDatos (qftbx::SpecificationRecord * record_in, qftbx::SpecificationRecord * upperRecord);
    void saveActiveTab();

    QVector <Parameter * > * buildParameters(QString linea);
    Parameter * buildScalar(QString linea, bool isK);

    static QString coefficientsText(QVector <Parameter *> * parametros);
    static QString numeratorText(LtiSystem * sistema);
    static QString denominatorText(LtiSystem * sistema);

    Controlador * controller;

    //images
    QPixmap trackingImagePixmap;
    QPixmap controlEffortPixmap;
    QPixmap outputDisturbancePixmap;
    QPixmap inputDisturbancePixmap;
    QPixmap sensorNoisePixmap;
    QPixmap stabilityPixmap;

    QVector <qreal> * frequencies;

    bool todoCorrecto;
};


#endif // QFTBX_SPECIFICATIONS_DIALOG_H
