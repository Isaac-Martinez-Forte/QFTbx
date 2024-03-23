#include "introducirplanta.h"
#include "ui_introducirplanta.h"

using namespace tools;

IntroducirPlanta::IntroducirPlanta(Controlador * controlador, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IntroducirPlanta)
{
    ui->setupUi(this);
    setWindowTitle("Introducir Planta");

    this->controlador = controlador;
    
    //ocultamos al inicio los botones no necesarios
    ui->pYCe->setVisible(false);
    ui->cDPol->setVisible(false);
    ui->kGa->setVisible(false);
    ui->kNoGa->setVisible(false);

    ui->kCP->setText("1");
    ui->retCP->setText("0");

    ui->kKG->setText("1");
    ui->retKG->setText("0");


    ui->kkNG->setText("1");
    ui->retKNG->setText("0");

    ui->kFL->setText("1");
    ui->retFL->setText("0");


    incertidumbreIntroducida = false;

    todoCorrecto = false;

    //Creamos la pantalla para introducir la incertidumbre que usaremos despues.
    viewIncer = new IntIncertidumbre (this);

    //Establecemos las imagenes de las plantas:

    QPixmap imagen1 (":/imagenes/kgan.png");
    ui->imaKG->setPixmap(imagen1);

    QPixmap imagen2 (":/imagenes/knogan.png");
    ui->imaKNG->setPixmap(imagen2);

    QPixmap imagen3 (":/imagenes/copol.png");
    ui->imaCP->setPixmap(imagen3);

    //conectamos el boton cancelar
    connect(ui->cancel, SIGNAL(clicked()), this, SLOT(close()));
}

IntroducirPlanta::~IntroducirPlanta()
{
    delete ui;
    delete viewIncer;
}

void IntroducirPlanta::on_pYCe_toggled(bool checked)
{
    ui->kGa->setVisible(checked);
    ui->kNoGa->setVisible(checked);
    ui->pYCe->setVisible(checked);
    ui->cDPol->setVisible(checked);

    if (checked == true)
        ui->agrupador->setCurrentIndex(0);
}

void IntroducirPlanta::on_fT_toggled(bool checked)
{
    ui->pYCe->setVisible(checked);
    ui->cDPol->setVisible(checked);

    if (checked == true)
        ui->agrupador->setCurrentIndex(0);
}

void IntroducirPlanta::on_kGa_toggled(bool checked)
{
    ui->pYCe->setVisible(checked);
    ui->cDPol->setVisible(checked);
    ui->kGa->setVisible(checked);
    ui->kNoGa->setVisible(checked);

    if (checked == true)
        ui->agrupador->setCurrentIndex(3);

}

void IntroducirPlanta::on_kNoGa_toggled(bool checked)
{

    ui->pYCe->setVisible(checked);
    ui->cDPol->setVisible(checked);
    ui->kGa->setVisible(checked);
    ui->kNoGa->setVisible(checked);

    if (checked == true)
        ui->agrupador->setCurrentIndex(4);
}

void IntroducirPlanta::on_cDPol_toggled(bool checked)
{
    ui->pYCe->setVisible(checked);
    ui->cDPol->setVisible(checked);

    if (checked == true)
        ui->agrupador->setCurrentIndex(2);
}

bool IntroducirPlanta::comprobarParse(QVector<QVector <QString> * > * tabla, QLineEdit *linea,
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

bool IntroducirPlanta::comprobarParseKREt(QVector<QVector <QString> * > * tabla, QLineEdit *linea,
                                          QVector <QVector <QString> * > * exp, QVector <QVector <bool> * > * isVar){


    QString aux = linea->text();
    aux = aux.trimmed();

    QVector <QString> * vec1 = new QVector <QString> (1, aux);
    QVector <QString> * vec = new QVector <QString> ();
    QVector <bool> * vec2 = new QVector <bool> ();

    QRegularExpression re("[a-zA-Z]+");
    QRegularExpressionMatch match = re.match(aux);
    qint32 i = 0;
    QString captura = match.captured(i);

    bool isVariable = false;

    while (!captura.isNull()){

        if (!p.IsFunDefined(captura.toStdString())){
            vec->append(captura);
            captura = QString();
            isVariable = true;
            break;
        }
        match = re.match(aux);
        captura = match.captured(0);
        aux.remove(captura);
    }

    vec2->append(isVariable);

    if (!isVariable){
        vec->append(aux);
    }

    tabla->append(vec);
    exp->append(vec1);
    isVar->append(vec2);

    return true;
}

bool IntroducirPlanta::comprobarParserFL(QLineEdit * linea, QVector<QVector <QString> * > * tabla, QVector<QVector<QString> *> *exp,
                       QVector <QVector <bool> * > * isVar){

    QVector <QString> * vec_exp = new QVector <QString> ();
    QVector <QString> * vec_tabla = new QVector <QString> ();
    QVector <bool> * vec_isVar  = new QVector <bool> ();

    QString nume_s = linea->text();

    QRegularExpression re("[a-zA-Z]+");
    QRegularExpressionMatch match = re.match(nume_s);
    QString captura = match.captured(0);

    nume_s.remove(captura);

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


QVector<QVector <QString> * > * IntroducirPlanta::seleTabla(QVector <QVector <QString> * > * exp, QVector <QVector <bool> * > * isVar){

    QVector <QVector <QString> * > * devolver = new QVector <QVector <QString> * > ();

    if (ui->cDPol->isChecked()){
        comprobarParse(devolver, ui->numeCP, exp, isVar);
        comprobarParse(devolver, ui->denoCP, exp, isVar);
        comprobarParseKREt(devolver, ui->kCP, exp, isVar);
        comprobarParseKREt(devolver, ui->retCP, exp, isVar);
    }else if (ui->kGa->isChecked()){
        comprobarParse(devolver, ui->numeKG, exp, isVar);
        comprobarParse(devolver, ui->denoKG, exp, isVar);
        comprobarParseKREt(devolver, ui->kKG, exp, isVar);
        comprobarParseKREt(devolver, ui->retKG, exp, isVar);
    }else if (ui->kNoGa->isChecked()){
        comprobarParse(devolver, ui->numeKNG, exp, isVar);
        comprobarParse(devolver, ui->denoKNG, exp, isVar);
        comprobarParseKREt(devolver, ui->kkNG, exp, isVar);
        comprobarParseKREt(devolver, ui->retKNG, exp, isVar);
    }else if (ui->Formato_Libre->isChecked()){
        comprobarParserFL(ui->numeFL, devolver, exp, isVar);
        comprobarParserFL(ui->denoFL, devolver, exp, isVar);
        comprobarParseKREt(devolver, ui->kFL, exp, isVar);
        comprobarParseKREt(devolver, ui->retFL, exp, isVar);
    }else{
        devolver->clear();
        exp->clear();
        isVar->clear();
        return NULL;
    }

    /* if (!valido){
        devolver->clear();
        return NULL;
    }*/

    return devolver;
}

void IntroducirPlanta::on_ok_clicked()
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

        QPointF punto = viewIncer->getK();
        p.SetExpr(exp->at(2)->at(0).toStdString());
        qreal d = p.Eval().GetFloat();

        if (d == punto.x() && d == punto.y()){
            kv = new Var (d);
        }else {
            kv = new Var ("kv", punto, d, "kv");
        }
    }

    if (datosTabla->at(3)->size() == 0){
        retv = new Var (qreal(0));
    }else{

        QPointF punto = viewIncer->getRet();
        p.SetExpr(exp->at(3)->at(0).toStdString());
        qreal d = p.Eval().GetFloat();

        if (d == punto.x() && d == punto.y()){
            retv = new Var (d);
        }else {
            retv = new Var ("ret", punto, d, "ret");
        }
    }

    if (incertidumbreIntroducida){
        if (ui->kGa->isChecked()){
            planta = new KGanancia(ui->nombre->text(),viewIncer->getNumerador(), viewIncer->getDenominador(),kv,retv);
        }else if(ui->kNoGa->isChecked()){
            planta = new KNGanancia(ui->nombre->text(),viewIncer->getNumerador(), viewIncer->getDenominador(),kv,retv);
        }else if (ui->cDPol->isChecked()){
            planta = new CPolinomios(ui->nombre->text(), viewIncer->getNumerador(), viewIncer->getDenominador(),kv,retv);
        }else{
            planta = new FormatoLibre(ui->nombre->text(), viewIncer->getNumerador(), viewIncer->getDenominador(),kv,retv,
                                      ui->numeFL->text(), ui->denoFL->text());
        }
    }else{
        if (ui->kGa->isChecked()){
            planta = new KGanancia(ui->nombre->text(),crearNumeradorDenominador(datosTabla->at(0)),
                                   crearNumeradorDenominador(datosTabla->at(1)),kv,retv );
        }else if(ui->kNoGa->isChecked()){
            planta = new KNGanancia(ui->nombre->text(),crearNumeradorDenominador(datosTabla->at(0)),
                                    crearNumeradorDenominador(datosTabla->at(1)),kv,retv);
        }else if (ui->cDPol->isChecked()){
            planta = new CPolinomios(ui->nombre->text(), crearNumeradorDenominador(datosTabla->at(0)),
                                     crearNumeradorDenominador(datosTabla->at(1)),kv,retv);
        }else {
            planta = new FormatoLibre(ui->nombre->text(), crearNumeradorDenominador(datosTabla->at(0)),
                                      crearNumeradorDenominador(datosTabla->at(1)),kv,retv,
                                      ui->numeFL->text(), ui->denoFL->text());
        }


    }

    controlador->setPlanta(planta);
    datosTabla->clear();
    exp->clear();
    isVar->clear();

    todoCorrecto = true;

    this->close();
}

QVector <Var * > * IntroducirPlanta::crearNumeradorDenominador(QVector <QString> * numeros){
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

void IntroducirPlanta::on_Inertidumbre_clicked()
{
    QVector <QVector <QString> * > * exp  = new QVector <QVector <QString> * > ();
    QVector <QVector <bool> * > *  isVar = new QVector <QVector <bool> * >  ();
    QVector <QVector <QString> * > * datosTabla = seleTabla(exp, isVar);

    if (datosTabla == NULL){
        menerror("Hay un error en los datos de la planta","Introducir Planta");
        return;
    }

    if (ui->nombre->text().isEmpty()){
        datosTabla->clear();
        menerror("Hay un error en el nombre de la planta.","Introducir Planta");
        ui->nombre->setStyleSheet("background : red");
        return;
    }else{
        ui->nombre->setStyleSheet("background : white");
    }


    viewIncer->lanzarViewIncer(datosTabla, exp, isVar, false);
    viewIncer->show();
    incertidumbreIntroducida = true;
}

void IntroducirPlanta::on_Formato_Libre_clicked()
{
    ui->agrupador->setCurrentIndex(1);
}


bool IntroducirPlanta::getTodoCorrecto(){
    return todoCorrecto;
}

