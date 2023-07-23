#include "introducirecontr.h"
#include "ui_introducirecontr.h"

using namespace tools;
using namespace mup;

introducirEContr::introducirEContr(Controlador * cont, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::introducirEContr)
{
    ui->setupUi(this);

    this->controlador = cont;

    setWindowTitle("Introducir Datos Controlador");

    QPixmap imagen1 (":/imagenes/kgan.png");
    ui->label_hf->setPixmap(imagen1);

    QPixmap imagen2 (":/imagenes/knogan.png");
    ui->label_hl->setPixmap(imagen2);

    QPixmap imagen3 (":/imagenes/copol.png");
    ui->label_poli->setPixmap(imagen3);

    ui->kInicio->setText("1");
    ui->kFinal->setText("1");

    incertidumbreIntroducida = false;

    //Creamos la pantalla para introducir la incertidumbre que usaremos despues.
    viewIncer = new IntIncertidumbre (this);

    todoCorrecto = false;
}

introducirEContr::~introducirEContr()
{
    delete ui;
}

void introducirEContr::on_c_poli_clicked()
{
    ui->imagenes->setCurrentIndex(1);
}

void introducirEContr::on_hf_clicked()
{
    ui->imagenes->setCurrentIndex(2);
}

void introducirEContr::on_lf_clicked()
{
    ui->imagenes->setCurrentIndex(3);
}

void introducirEContr::on_libertad_clicked()
{
    QVector <QVector <QString> * > * exp  = new QVector <QVector <QString> * > ();
    QVector <QVector <bool> * > *  isVar = new QVector <QVector <bool> * >  ();
    QVector <QVector <QString> * > * datosTabla = seleTabla(exp, isVar);

    if (datosTabla == NULL){
        menerror("Hay un error en los datos de la planta","Introducir Planta");
        return;
    }


    viewIncer->lanzarViewIncer(datosTabla, exp, isVar, true);
    viewIncer->show();
    incertidumbreIntroducida = true;
}

QVector<QVector <QString> * > * introducirEContr::seleTabla(QVector <QVector <QString> * > * exp,
                                                            QVector <QVector <bool> * > * isVar){

    bool valido = true;
    QVector <QVector <QString> * > * devolver = new QVector <QVector <QString> * > ();
    devolver->reserve(4);

    if (ui->fl->isChecked()){
        valido = comprobarParserFL(ui->nume, devolver, exp, isVar);
        valido = comprobarParserFL(ui->deno, devolver, exp, isVar);
        valido = comprobarParseKREt(devolver, ui->kInicio, ui->kFinal, exp, isVar);
    }else {
        valido = comprobarParse(devolver, ui->nume, exp, isVar);
        valido = comprobarParse(devolver, ui->deno, exp, isVar);
        valido = comprobarParseKREt(devolver, ui->kInicio, ui->kFinal, exp, isVar);
    }

    if (!valido){
        devolver->clear();
        return NULL;
    }

    return devolver;
}

bool introducirEContr::comprobarParse(QVector<QVector <QString> * > * tabla, QLineEdit *linea,
                                      QVector<QVector <QString> * > * exp, QVector <QVector <bool> * > * isVar){

    QVector <QString> * vec1 = tools::srtovectorString(linea->text());
    QVector <QString> * vec = new QVector <QString> ();
    QVector <bool> * vec2  = new QVector <bool> ();

    if (linea->text().isEmpty()){
        vec1->append("1");
        vec2->append(false);
    } else{

        foreach (QString e, *vec1) {

            QRegularExpression re("[a-zA-Z]+");

            QRegularExpressionMatch match = re.match(e);
            QString captura = match.captured(0);
            e.remove(captura);

            bool isVariable = false;

            while (!captura.isNull()){

                if (!p.IsFunDefined(captura.toStdString())){
                    vec->append(captura);
                    captura = QString();
                    isVariable = true;
                    break;
                }
                match = re.match(e);
                captura = match.captured(0);
                e.remove(captura);
            }

            vec2->append(isVariable);

            if (!isVariable){
                vec->append(e);
            }
        }
    }

    tabla->append(vec);
    isVar->append(vec2);
    exp->append(vec1);

    return true;
}

bool introducirEContr::comprobarParseKREt(QVector<QVector <QString> * > * tabla, QLineEdit *linea1, QLineEdit * linea2,
                                          QVector <QVector <QString> * > * exp, QVector <QVector <bool> * > * isVar){


    QString aux = linea1->text();
    QString aux1 = linea2->text();
    aux = aux.trimmed();

    QVector <QString> * vec1 = new QVector <QString> ();
    QVector <QString> * vec = new QVector <QString> ();
    QVector <bool> * vec2 = new QVector <bool> ();
    vec2->append(true);

    vec->append(aux);
    vec->append(aux1);

    vec1->append(aux);
    vec1->append(aux1);

    tabla->append(vec);
    exp->append(vec1);
    isVar->append(vec2);

    return true;
}

bool introducirEContr::comprobarParserFL(QLineEdit * linea, QVector<QVector <QString> * > * tabla, QVector<QVector<QString> *> *exp,
                                         QVector <QVector <bool> * > * isVar){

    QVector <QString> * vec_exp = new QVector <QString> ();
    QVector <QString> * vec_tabla = new QVector <QString> ();
    QVector <bool> * vec_isVar  = new QVector <bool> ();

    QString nume_s = linea->text();

    QRegularExpression re("[a-zA-Z]+");
    QRegularExpressionMatch match = re.match(nume_s);
    QString captura = match.captured(0);


    while (!captura.isNull()){

        if (!p.IsFunDefined(captura.toStdString()) && captura != "s"){

            vec_exp->append(captura);
            vec_tabla->append(captura);
            vec_isVar->append(true);

            captura = QString();
        }
        match = re.match(nume_s);
        captura = match.captured(0);
        nume_s.remove(captura);
    }


    tabla->append(vec_tabla);
    exp->append(vec_exp);
    isVar->append(vec_isVar);

    return true;
}


void introducirEContr::on_cancelar_clicked()
{
    emit(close());
}

void introducirEContr::on_aceptar_clicked()
{

    QVector <QVector <QString> * > * exp = new QVector <QVector <QString> * > ();
    QVector <QVector <bool> * > *  isVar = new QVector <QVector <bool> * >  ();
    QVector <QVector <QString> * > * datosTabla = seleTabla(exp, isVar);

    if (datosTabla == NULL){
        menerror("Hay un error en los datos de la planta","Introducir Planta");
        return;
    }


    Var * kv;
    Var * retv;

    if (datosTabla->at(2)->size() == 0){
        kv = new Var (1);
    }else{

        QPointF punto;
        p.SetExpr(exp->at(2)->at(0).toStdString());
        punto.setX(p.Eval().GetFloat());

        p.SetExpr(exp->at(2)->at(1).toStdString());
        punto.setY(p.Eval().GetFloat());

        if (punto.x() == punto.y()){
            kv = new Var (punto.x());
        }else {

            if (punto.x() > punto.y()){
                qreal a = punto.x();
                punto.setX(punto.y());
                punto.setY(a);
            }

            kv = new Var ("kv", punto, (punto.x() + punto.y()) / 2);
        }
    }

    retv = new Var (0.0);


    if (incertidumbreIntroducida){
        if (ui->hf->isChecked()){
            planta = std::make_shared<KGanancia>("",viewIncer->getNumerador(), viewIncer->getDenominador(),kv,retv);
        }else if(ui->lf->isChecked()){
            planta = std::make_shared<KNGanancia>("",viewIncer->getNumerador(), viewIncer->getDenominador(),kv,retv);
        }else if (ui->c_poli->isChecked()){
            planta = std::make_shared<CPolinomios>("", viewIncer->getNumerador(), viewIncer->getDenominador(),kv,retv);
        }else{
            planta = std::make_shared<FormatoLibre>("", viewIncer->getNumerador(), viewIncer->getDenominador(),kv,retv,
                                      ui->nume->text(), ui->deno->text());
        }
    }else{
        if (ui->hf->isChecked()){
            planta = std::make_shared<KGanancia>("",crearNumeradorDenominador(datosTabla->at(0)),
                                   crearNumeradorDenominador(datosTabla->at(1)),kv,retv );
        }else if(ui->lf->isChecked()){
            planta = std::make_shared<KNGanancia>("",crearNumeradorDenominador(datosTabla->at(0)),
                                    crearNumeradorDenominador(datosTabla->at(1)),kv,retv);
        }else if (ui->c_poli->isChecked()){
            planta = std::make_shared<CPolinomios>("", crearNumeradorDenominador(datosTabla->at(0)),
                                     crearNumeradorDenominador(datosTabla->at(1)),kv,retv);
        }else {
            planta = std::make_shared<FormatoLibre>("", crearNumeradorDenominador(datosTabla->at(0)),
                                      crearNumeradorDenominador(datosTabla->at(1)),kv,retv,
                                      ui->nume->text(), ui->deno->text());
        }


    }

    controlador->setControlador(planta);
    datosTabla->clear();
    exp->clear();
    isVar->clear();

    todoCorrecto = true;

    this->close();
}


QVector <Var * > * introducirEContr::crearNumeradorDenominador(QVector <QString> * numeros){
    QVector <Var *> * var = new QVector <Var *> ();
    var->reserve(numeros->size());

    if (numeros->isEmpty()){
        return var;
    }

    foreach (const QString &string, *numeros) {
        p.SetExpr(string.toStdString());
        var->append(new Var(p.Eval().GetFloat()));
    }

    return var;
}

bool introducirEContr::getTodoCorrecto(){
    return todoCorrecto;
}
