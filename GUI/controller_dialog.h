#ifndef QFTBX_CONTROLLER_DIALOG_H
#define QFTBX_CONTROLLER_DIALOG_H

#include <memory>

#include <optional>
#include <vector>

#include <QDialog>
#include <QRegularExpression>

#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/polynomial_form.h"
#include "GUI/uncertainty_dialog.h"
#include "src/core/math/sequence_vectors.h"
#include "mpParser.h"


namespace Ui {
class ControllerDialog;
}

class ControllerDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ControllerDialog(QWidget *parent = 0);
    ~ControllerDialog();

    bool getTodoCorrecto();

    /// The controller structure the user described (with the search box of
    /// its parameters), or nullptr when cancelled or rejected. Ownership
    /// passes to the caller.
    std::unique_ptr<LtiSystem> takeControllerStructure();
    
private slots:
    void on_polynomialRadio_clicked();

    void on_zpkRadio_clicked();

    void on_tcgRadio_clicked();

    void on_uncertaintyButton_clicked();

    void on_cancelButton_clicked();

    void on_okButton_clicked();

private:
    Ui::ControllerDialog *ui;




    QString file;

    UncertaintyDialog * uncertaintyDialog= NULL;

    std::unique_ptr<LtiSystem> controllerSystem;

    bool uncertaintyEntered;

    QVector<QVector<QString> *> * readTables(QVector <QVector <QString> * > * expressionTable,
                                            QVector <QVector <bool> * > * uncertainTable);
    /// The coefficients of one polynomial, or nothing when any of them is
    /// not a valid expression. The invalid one used to become 0 in silence,
    /// which quietly designed a different controller.
    std::optional<std::vector<Parameter>> buildParameters(QVector<QString> *numeros);
    bool parse(QString cadena);
    bool parseCoefficients(QVector<QVector <QString> * > * tabla, QLineEdit *linea,
                        QVector<QVector <QString> * > * expressionTable, QVector <QVector <bool> * > * uncertainTable);
    bool parseGainRange(QVector<QVector <QString> * > * tabla, QLineEdit *linea1, QLineEdit *linea2,
                            QVector <QVector <QString> * > * expressionTable, QVector <QVector <bool> * > * uncertainTable);

    bool parseFreeForm(QLineEdit * linea, QVector<QVector <QString> * > * tabla,
                                             QVector<QVector<QString> *> *expressionTable,
                                             QVector <QVector <bool> * > * uncertainTable);

    qreal resultado;
    mup::ParserX p;

    bool todoCorrecto;
};

#endif // QFTBX_CONTROLLER_DIALOG_H
