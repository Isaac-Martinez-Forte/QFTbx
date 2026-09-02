#ifndef QFTBX_LOOP_SHAPING_DIALOG_H
#define QFTBX_LOOP_SHAPING_DIALOG_H

#include <memory>

#include <QDialog>

#include "mpParser.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequence_vectors.h"

namespace Ui {  
class LoopShapingDialog;
}

/**
 * @brief Step 7 of the design: picks one of the five loop-shaping
 * algorithms and the accuracy to run it to.
 *
 * What the accuracy measures depends on the algorithm chosen, because each
 * follows the criterion of its own paper - see
 * ProjectController::computeLoopShaping. The single field does not say so
 * yet.
 */
class LoopShapingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoopShapingDialog(QWidget *parent = 0);
    ~LoopShapingDialog();

    void setEpsilonValue (qreal epsilonEdit);

    bool wasAccepted();

    qreal epsilonValue ();

    tools::LoopShapingAlgorithm algorithmValue();

    QPointF range();

    qreal pointCountValue();

    bool isLinSpace();

    qint32 initialisationValue ();

private slots:
    void on_cancelButton_clicked();

    void on_okButton_clicked();

    void on_linspaceRadio_clicked();

    void on_logspaceRadio_clicked();

    void on_ntRadio_clicked();

    void on_nkRadio_clicked();

    void on_mrRadio_clicked();
    void on_mc1Radio_clicked();
    void on_mcThesisRadio_clicked();

protected:
    void showEvent(QShowEvent * event) override;

private:
    std::unique_ptr<Ui::LoopShapingDialog> ui;

    bool accepted;

    qreal epsilonEdit = 0.0;

    QPointF plotRange;

    qreal pointCountEdit = 0.0;

    qint32 initialisation = 0;

    tools::LoopShapingAlgorithm alg = tools::nt;

    bool linLogSpace;
};

#endif // QFTBX_LOOP_SHAPING_DIALOG_H
