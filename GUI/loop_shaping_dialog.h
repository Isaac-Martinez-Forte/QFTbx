#ifndef QFTBX_LOOP_SHAPING_DIALOG_H
#define QFTBX_LOOP_SHAPING_DIALOG_H

#include <QDialog>

#include "mpParser.h"
#include "Modelo/Herramientas/tools.h"
#include "Modelo/controlador.h"
#include "Modelo/Herramientas/tools.h"

namespace Ui {  
class LoopShapingDialog;
}

class LoopShapingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoopShapingDialog(QWidget *parent = 0);
    ~LoopShapingDialog();

    void setEpsilonValue (qreal epsilonEdit);

    bool getTodoCorrecto();

    qreal epsilonValue ();

    tools::alg_loop_shaping algorithmValue();

    QPointF range();

    qreal pointCountValue();

    qreal deltaValue ();

    bool isLinSpace();

    bool debugValue();

    qint32 initialisationValue ();

    bool threadsValue();
    
    bool bisectionValue();
    bool detectionValue();
    bool acceleratedValue();

private slots:
    void on_cancelButton_clicked();

    void on_okButton_clicked();

    void on_linspaceRadio_clicked();

    void on_logspaceRadio_clicked();

    void on_ntRadio_clicked();

    void on_nkRadio_clicked();

    void on_mrRadio_clicked();

protected:
    void showEvent(QShowEvent * event) override;

private:
    Ui::LoopShapingDialog *ui;

    bool todoCorrecto;

    qreal epsilonEdit;

    QPointF plotRange;

    qreal pointCountEdit;

    bool threadsCheck;
    
    qreal deltaEdit;
    qint32 initialisation;

    tools::alg_loop_shaping alg;

    bool linLogSpace;
    bool debugCheck;
};

#endif // QFTBX_LOOP_SHAPING_DIALOG_H
