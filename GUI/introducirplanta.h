#ifndef INTRODUCIRPLANTA_H
#define INTRODUCIRPLANTA_H

#include <QDialog>
#include <qvalidator.h>
#include <QRadioButton>
#include <QList>
#include <QFileDialog>
#include <QRegularExpression>
#include <QRegExp>

#include "Modelo/EstructuraSistema//kganancia.h"
#include "Modelo/EstructuraSistema/knganancia.h"
#include "Modelo/EstructuraSistema/cpolinomios.h"
#include "Modelo/EstructuraSistema/formatolibre.h"
#include "GUI/intincertidumbre.h"
#include "Modelo/controlador.h"
#include "Modelo/Herramientas/tools.h"
#include "muparserx/mpParser.h"

namespace Ui {
class IntroducirPlanta;
}

class IntroducirPlanta : public QDialog
{
    Q_OBJECT

public:

    explicit IntroducirPlanta(Controlador *controlador, QWidget *parent = 0);
    ~IntroducirPlanta();

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
    Ui::IntroducirPlanta *ui;
    
    Controlador * controlador = NULL;

    QRadioButton * gFT= NULL;
    QString file;

    IntIncertidumbre * viewIncer= NULL;

    std::shared_ptr<Sistema> planta;

    bool incertidumbreIntroducida;

    void openFile();
    bool guardar ();
    QVector<QVector<QString> *> *seleTabla(QVector<QVector<QString> *> *exp, QVector<QVector<bool> *> *isVar);
    QVector<Var *> *crearNumeradorDenominador(QVector<QString> *numeros);
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

#endif // INTRODUCIRPLANTA_H
