#ifndef QFTBX_PLANT_DIALOG_H
#define QFTBX_PLANT_DIALOG_H

#include <vector>

#include <QDialog>
#include <qvalidator.h>
#include <QRadioButton>
#include <QList>
#include <QFileDialog>
#include <QRegularExpression>
#include <QRegularExpression>

#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/free_form.h"
#include "GUI/uncertainty_dialog.h"
#include "Modelo/controlador.h"
#include "src/core/math/sequence_vectors.h"
#include "mpParser.h"

namespace Ui {
class PlantDialog;
}

class PlantDialog : public QDialog
{
    Q_OBJECT

public:

    explicit PlantDialog(Controlador *controller, QWidget *parent = 0);
    ~PlantDialog();

    bool getTodoCorrecto();
    
    
    
private slots:

    void on_zerosPolesRadio_toggled(bool checked);

    void on_transferFunctionRadio_toggled(bool checked);

    void on_zpkRadio_toggled(bool checked);

    void on_tcgRadio_toggled(bool checked);

    void on_polynomialRadio_toggled(bool checked);

    void on_okButton_clicked();

    void on_uncertaintyButton_clicked();

    void on_freeFormRadio_clicked();
    

private:
    Ui::PlantDialog *ui;
    
    Controlador * controller = NULL;

    QRadioButton * gFT= NULL;
    QString file;

    UncertaintyDialog * uncertaintyDialog= NULL;

    LtiSystem * plant= NULL;

    bool uncertaintyEntered;

    void openFile();
    bool guardar ();
    QVector<QVector<QString> *> *readTables(QVector<QVector<QString> *> *expressionTable, QVector<QVector<bool> *> *uncertainTable);
    std::vector<Parameter> buildParameters(QVector<QString> *numeros);
    bool parse(QString cadena);
    bool parseCoefficients(QVector<QVector<QString> *> *tabla, QLineEdit * linea, QVector<QVector<QString> *> *expressionTable,
                        QVector<QVector<bool> *> *uncertainTable);
    bool parseScalar(QVector<QVector <QString> * > * tabla, QLineEdit *linea, QVector<QVector<QString> *> *expressionTable,
                            QVector <QVector <bool> * > * uncertainTable);
    bool parseFreeForm(QLineEdit * linea, QVector<QVector <QString> * > * tabla, QVector<QVector<QString> *> *expressionTable,
                           QVector <QVector <bool> * > * uncertainTable);


    qreal resultado;
    mup::ParserX p;

    bool todoCorrecto;
};

#endif // QFTBX_PLANT_DIALOG_H
