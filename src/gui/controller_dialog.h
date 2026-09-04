#ifndef QFTBX_CONTROLLER_DIALOG_H
#define QFTBX_CONTROLLER_DIALOG_H

#include "src/gui/step_dialog.h"
#include "src/gui/coefficient_tables.h"

#include <memory>

#include <optional>
#include <vector>

#include <QDialog>
#include <QRegularExpression>

#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/polynomial_form.h"
#include "src/gui/uncertainty_dialog.h"
#include "src/core/math/sequence_vectors.h"
#include "mpParser.h"


namespace Ui {
class ControllerDialog;
}

class ControllerDialog : public StepDialog
{
    Q_OBJECT
    
public:
    explicit ControllerDialog(QWidget *parent = 0);
    ~ControllerDialog();


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
    std::unique_ptr<Ui::ControllerDialog> ui;




    QString file;

    UncertaintyDialog * uncertaintyDialog= NULL;

    std::unique_ptr<LtiSystem> controllerSystem;

    bool uncertaintyEntered;

    /// The coefficients of the described controller, or nothing when the
    /// dialog could not read them (the caller reports it).
    std::optional<CoefficientTable> readTables(CoefficientTable & expressionTable,
                                               UncertainTable & uncertainTable);
    /// The coefficients of one polynomial, or nothing when any of them is
    /// not a valid expression. The invalid one used to become 0 in silence,
    /// which quietly designed a different controller.
    std::optional<std::vector<Parameter>> buildParameters(const CoefficientRow & numbers);
    bool parse(QString cadena);
    bool parseCoefficients(CoefficientTable & tabla, QLineEdit *linea,
                           CoefficientTable & expressionTable, UncertainTable & uncertainTable);
    bool parseGainRange(CoefficientTable & tabla, QLineEdit *linea1, QLineEdit *linea2,
                        CoefficientTable & expressionTable, UncertainTable & uncertainTable);

    bool parseFreeForm(QLineEdit * linea, CoefficientTable & tabla,
                       CoefficientTable & expressionTable, UncertainTable & uncertainTable);

    mup::ParserX p;

};

#endif // QFTBX_CONTROLLER_DIALOG_H
