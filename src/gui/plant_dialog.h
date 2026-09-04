#ifndef QFTBX_PLANT_DIALOG_H
#define QFTBX_PLANT_DIALOG_H

#include "src/gui/step_dialog.h"
#include "src/gui/coefficient_tables.h"

#include <memory>

#include <optional>
#include <vector>

#include <QDialog>
#include <qvalidator.h>
#include <QRadioButton>
#include <QList>
#include <QFileDialog>
#include <QRegularExpression>

#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/free_form.h"
#include "src/gui/uncertainty_dialog.h"
#include "src/core/math/sequence_vectors.h"
#include "mpParser.h"

namespace Ui {
class PlantDialog;
}

class PlantDialog : public StepDialog
{
    Q_OBJECT

public:

    explicit PlantDialog(QWidget *parent = 0);
    ~PlantDialog();


    /**
     * @brief The plant the user described, or nullptr when the dialog was
     * cancelled or its data rejected. Ownership passes to the caller.
     *
     * The dialog does not know the project: it builds a plant and hands it
     * over, and the main window is what publishes it. (It used to hold the
     * facade and write into it, which gave every dialog access to the whole
     * application.)
     */
    std::unique_ptr<LtiSystem> takePlant();
    
    
    
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
    std::unique_ptr<Ui::PlantDialog> ui;

    UncertaintyDialog * uncertaintyDialog = nullptr;

    std::unique_ptr<LtiSystem> plant;

    bool uncertaintyEntered = false;

    /// The coefficients of the described system, or nothing when the dialog
    /// could not read them (the caller reports it).
    std::optional<CoefficientTable> readTables(CoefficientTable & expressionTable,
                                               UncertainTable & uncertainTable);
    /// The coefficients of one polynomial, or nothing when any of them is
    /// not a valid expression. The invalid one used to become 0 in silence,
    /// which quietly designed for a different plant. Same contract as
    /// SpecificationsDialog::buildParameters.
    std::optional<std::vector<Parameter>> buildParameters(const CoefficientRow & numbers);
    bool parseCoefficients(CoefficientTable & table, QLineEdit * field,
                           CoefficientTable & expressionTable, UncertainTable & uncertainTable);
    bool parseScalar(CoefficientTable & table, QLineEdit *field,
                     CoefficientTable & expressionTable, UncertainTable & uncertainTable);
    bool parseFreeForm(QLineEdit * field, CoefficientTable & table,
                       CoefficientTable & expressionTable, UncertainTable & uncertainTable);


    mup::ParserX p;

};

#endif // QFTBX_PLANT_DIALOG_H
