#ifndef INTRODUCIRECONTR_H
#define INTRODUCIRECONTR_H

#include <QDialog>
#include <QRegExp>

#include "Modelo/EstructuraSistema//kganancia.h"
#include "Modelo/EstructuraSistema/knganancia.h"
#include "Modelo/EstructuraSistema/cpolinomios.h"
#include "GUI/intincertidumbre.h"
#include "Modelo/controlador.h"
#include "Modelo/Herramientas/tools.h"
#include "muparserx/mpParser.h"


namespace Ui {
class introducirEContr;
}

class introducirEContr : public QDialog
{
    Q_OBJECT
    
public:
    explicit introducirEContr(Controlador * controlador, QWidget *parent = 0);
    ~introducirEContr();

    bool getTodoCorrecto();
    
private slots:
    void on_c_poli_clicked();

    void on_hf_clicked();

    void on_lf_clicked();

    void on_libertad_clicked();

    void on_cancelar_clicked();

    void on_aceptar_clicked();

private:
    Ui::introducirEContr *ui;



    Controlador * controlador = NULL;

    QString file;

    IntIncertidumbre * viewIncer= NULL;

    Sistema * planta= NULL;

    bool incertidumbreIntroducida;

    QVector<QVector<QString> *> * seleTabla(QVector <QVector <QString> * > * exp,
                                            QVector <QVector <bool> * > * isVar);
    QVector<Var *> * crearNumeradorDenominador(QVector<QString> *numeros);
    bool parse(QString cadena);
    bool comprobarParse(QVector<QVector <QString> * > * tabla, QLineEdit *linea,
                        QVector<QVector <QString> * > * exp, QVector <QVector <bool> * > * isVar);
    bool comprobarParseKREt(QVector<QVector <QString> * > * tabla, QLineEdit *linea1, QLineEdit *linea2,
                            QVector <QVector <QString> * > * exp, QVector <QVector <bool> * > * isVar);

    bool comprobarParserFL(QLineEdit * linea, QVector<QVector <QString> * > * tabla,
                                             QVector<QVector<QString> *> *exp,
                                             QVector <QVector <bool> * > * isVar);

    qreal resultado;
    mup::ParserX p;

    bool todoCorrecto;
};

#endif // INTRODUCIRECONTR_H
