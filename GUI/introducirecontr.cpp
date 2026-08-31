#include "introducirecontr.h"
#include "ui_introducirecontr.h"

#include "GUI/menerror.h"
#include "GUI/plot_palette.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"

using namespace tools;
using namespace mup;

namespace {

//Las tres tablas paralelas del parseo se liberan juntas (antes se
//abandonaban con clear() o directamente se perdian en los errores).
void liberarTablas(QVector <QVector <QString> * > * datosTabla,
                   QVector <QVector <QString> * > * exp,
                   QVector <QVector <bool> * > * isVar){
    if (datosTabla != nullptr){
        qDeleteAll(*datosTabla);
        delete datosTabla;
    }
    qDeleteAll(*exp);
    delete exp;
    qDeleteAll(*isVar);
    delete isVar;
}

} // namespace

introducirEContr::introducirEContr(Controlador * cont, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::introducirEContr)
{
    ui->setupUi(this);

    this->controlador = cont;

    setWindowTitle("Introducir Datos Controlador");

    QPixmap imagen1 (":/figures/kgan.png");
    ui->label_hf->setPixmap(imagen1);

    QPixmap imagen2 (":/figures/knogan.png");
    ui->label_hl->setPixmap(imagen2);

    QPixmap imagen3 (":/figures/copol.png");
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
    ui->figures->setCurrentIndex(1);
}

void introducirEContr::on_hf_clicked()
{
    ui->figures->setCurrentIndex(2);
}

void introducirEContr::on_lf_clicked()
{
    ui->figures->setCurrentIndex(3);
}

void introducirEContr::on_libertad_clicked()
{
    QVector <QVector <QString> * > * exp  = new QVector <QVector <QString> * > ();
    QVector <QVector <bool> * > *  isVar = new QVector <QVector <bool> * >  ();
    QVector <QVector <QString> * > * datosTabla = seleTabla(exp, isVar);

    if (datosTabla == NULL){
        qDeleteAll(*exp);
        delete exp;
        qDeleteAll(*isVar);
        delete isVar;
        menerror("Hay un error en los datos del controlador","Introducir Controlador");
        return;
    }


    //Las tablas pasan a ser propiedad del dialogo de incertidumbre.
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
        qDeleteAll(*devolver);
        delete devolver;
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

            bool isUncertain = false;

            while (!captura.isNull()){

                if (!p.IsFunDefined(captura.toStdString())){
                    vec->append(captura);
                    captura = QString();
                    isUncertain = true;
                    break;
                }
                match = re.match(e);
                captura = match.captured(0);
                e.remove(captura);
            }

            vec2->append(isUncertain);

            if (!isUncertain){
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


    QString aux = linea1->text().trimmed();
    QString aux1 = linea2->text().trimmed();

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

    //La primera captura debe salir de la cadena ANTES del bucle (como en
    //IntroducirPlanta): sin esto cada variable incierta se registraba DOS
    //veces en el camino de formato libre.
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
        qDeleteAll(*exp);
        delete exp;
        qDeleteAll(*isVar);
        delete isVar;
        menerror("Hay un error en los datos del controlador","Introducir Controlador");
        return;
    }


    Parameter * kv = nullptr;
    Parameter * retv = nullptr;

    //Expresiones del usuario: un error de sintaxis lanzaba y tiraba la
    //aplicacion.
    try {
        if (datosTabla->at(2)->size() == 0){
            kv = new Parameter (1);
        }else{

            QPointF punto;
            p.SetExpr(exp->at(2)->at(0).toStdString());
            punto.setX(p.Eval().GetFloat());

            p.SetExpr(exp->at(2)->at(1).toStdString());
            punto.setY(p.Eval().GetFloat());

            if (punto.x() == punto.y()){
                kv = new Parameter (punto.x());
            }else {

                if (punto.x() > punto.y()){
                    qreal a = punto.x();
                    punto.setX(punto.y());
                    punto.setY(a);
                }

                kv = new Parameter ("kv", punto, (punto.x() + punto.y()) / 2);
            }
        }
    } catch (mup::ParserError &) {
        liberarTablas(datosTabla, exp, isVar);
        menerror("Hay un error en los datos del controlador","Introducir Controlador");
        return;
    }

    retv = new Parameter (0.0);


    //La incertidumbre solo cuenta si su dialogo se ACEPTO.
    if (incertidumbreIntroducida && viewIncer->getTodoCorrecto()){
        //La planta toma propiedad de sus variables: se le entregan copias y
        //el dialogo de incertidumbre conserva sus originales para editar.
        QVector <Parameter*> * nume = Parameter::cloneVector(viewIncer->numerator());
        QVector <Parameter*> * deno = Parameter::cloneVector(viewIncer->denominator());

        if (ui->hf->isChecked()){
            planta = new ZeroPoleGain("",nume, deno,kv,retv);
        }else if(ui->lf->isChecked()){
            planta = new TimeConstantGain("",nume, deno,kv,retv);
        }else if (ui->c_poli->isChecked()){
            planta = new PolynomialForm("", nume, deno,kv,retv);
        }else{
            planta = new FreeForm("", nume, deno,kv,retv,
                                      ui->nume->text(), ui->deno->text());
        }
    }else{
        if (ui->hf->isChecked()){
            planta = new ZeroPoleGain("",crearNumeradorDenominador(datosTabla->at(0)),
                                   crearNumeradorDenominador(datosTabla->at(1)),kv,retv );
        }else if(ui->lf->isChecked()){
            planta = new TimeConstantGain("",crearNumeradorDenominador(datosTabla->at(0)),
                                    crearNumeradorDenominador(datosTabla->at(1)),kv,retv);
        }else if (ui->c_poli->isChecked()){
            planta = new PolynomialForm("", crearNumeradorDenominador(datosTabla->at(0)),
                                     crearNumeradorDenominador(datosTabla->at(1)),kv,retv);
        }else {
            planta = new FreeForm("", crearNumeradorDenominador(datosTabla->at(0)),
                                      crearNumeradorDenominador(datosTabla->at(1)),kv,retv,
                                      ui->nume->text(), ui->deno->text());
        }


    }

    controlador->setControlador(planta);
    liberarTablas(datosTabla, exp, isVar);

    todoCorrecto = true;

    this->close();
}


QVector <Parameter * > * introducirEContr::crearNumeradorDenominador(QVector <QString> * numeros){
    QVector <Parameter *> * var = new QVector <Parameter *> ();
    var->reserve(numeros->size());

    if (numeros->isEmpty()){
        return var;
    }

    foreach (const QString &string, *numeros) {
        p.SetExpr(string.toStdString());
        try {
            var->append(new Parameter(p.Eval().GetFloat()));
        } catch (mup::ParserError &) {
            //Coeficiente invalido: 0 en vez de tirar la aplicacion.
            var->append(new Parameter(qreal(0)));
        }
    }

    return var;
}

bool introducirEContr::getTodoCorrecto(){
    return todoCorrecto;
}
