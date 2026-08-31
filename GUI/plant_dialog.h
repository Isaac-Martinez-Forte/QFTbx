#ifndef QFTBX_PLANT_DIALOG_H
#define QFTBX_PLANT_DIALOG_H

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
#include "Modelo/Herramientas/tools.h"
#include "mpParser.h"

namespace Ui {
class PlantDialog;
}

class PlantDialog : public QDialog
{
    Q_OBJECT

public:

    explicit PlantDialog(Controlador *controlador, QWidget *parent = 0);
    ~PlantDialog();

    bool getTodoCorrecto();
    
    
    
private slots:

    void on_pYCe_toggled(bool checked);

    void on_fT_toggled(bool checked);

    void on_kGa_toggled(bool checked);

    void on_kNoGa_toggled(bool checked);

    void on_cDPol_toggled(bool checked);

    void on_ok_clicked();

    void on_Inertidumbre_clicked();

    void on_Formato_Libre_clicked();
    

private:
    Ui::PlantDialog *ui;
    
    Controlador * controlador = NULL;

    QRadioButton * gFT= NULL;
    QString file;

    UncertaintyDialog * viewIncer= NULL;

    LtiSystem * planta= NULL;

    bool incertidumbreIntroducida;

    void openFile();
    bool guardar ();
    QVector<QVector<QString> *> *seleTabla(QVector<QVector<QString> *> *exp, QVector<QVector<bool> *> *isVar);
    QVector<Parameter *> *crearNumeradorDenominador(QVector<QString> *numeros);
    bool parse(QString cadena);
    bool comprobarParse(QVector<QVector<QString> *> *tabla, QLineEdit * linea, QVector<QVector<QString> *> *exp,
                        QVector<QVector<bool> *> *isVar);
    bool comprobarParseKREt(QVector<QVector <QString> * > * tabla, QLineEdit *linea, QVector<QVector<QString> *> *exp,
                            QVector <QVector <bool> * > * isVar);
    bool comprobarParserFL(QLineEdit * linea, QVector<QVector <QString> * > * tabla, QVector<QVector<QString> *> *exp,
                           QVector <QVector <bool> * > * isVar);


    qreal resultado;
    mup::ParserX p;

    bool todoCorrecto;
};

#endif // QFTBX_PLANT_DIALOG_H
