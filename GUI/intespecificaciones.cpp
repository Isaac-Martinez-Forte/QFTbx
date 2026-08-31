#include "intespecificaciones.h"

#include "src/core/specifications/specification.h"
#include "ui_intespecificaciones.h"

#include "GUI/menerror.h"
#include "GUI/plot_palette.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"

using namespace tools;
using namespace mup;

IntEspecificaciones::IntEspecificaciones(Controlador *controlador, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IntEspecificaciones)
{
    ui->setupUi(this);

    this->controlador = controlador;
    this->omega = controlador->getOmega()->getValores();

    setWindowTitle("Introducir Datos Especificaciones");

    retorno = NULL;

    //Establecemos las figures de las plantas:

    QPixmap imagen1 (":/figures/kgan.png");
    ui->imagHF->setPixmap(imagen1);
    ui->imgKGse1->setPixmap(imagen1);
    ui->imgKGse1_2->setPixmap(imagen1);

    QPixmap imagen2 (":/figures/knogan.png");
    ui->imagLF->setPixmap(imagen2);
    ui->imgKNGse1->setPixmap(imagen2);
    ui->imgKNGse1_2->setPixmap(imagen2);

    QPixmap imagen3 (":/figures/copol.png");
    ui->imagCP->setPixmap(imagen3);
    ui->imgCPse1->setPixmap(imagen3);
    ui->imgCPse1_2->setPixmap(imagen3);

    activado = 0;

    ui->k->setText("1");
    ui->ret->setText("0");

    seguimiento = new dBND();
    seguimiento_2 = new dBND();
    estabilidad = new dBND();
    ruido = new dBND();
    RPS = new dBND();
    RPE = new dBND();
    EC = new dBND();

    //figures
    seguimiento_img = QPixmap (":/figures/seguimiento.png");
    EC_img = QPixmap (":/figures/EC.png");
    RPS_img= QPixmap (":/figures/RPS.png");
    RPE_img= QPixmap (":/figures/RPE.png");
    ruidosensor_img= QPixmap (":/figures/ruidosensor.png");
    estabilidad_img= QPixmap (":/figures/estabilidad.png");

    ui->frecini->setText(QString::number(omega->first()));
    ui->frecfin->setText(QString::number(omega->last()));

    ui->imagen2->setPixmap(seguimiento_img);


    todoCorrecto = false;
}

IntEspecificaciones::~IntEspecificaciones()
{
    delete ui;
}

void IntEspecificaciones::setDatos(dBND * datos)
{
    if (datos->utilizado){

        ui->frecini->setText(QString::number(datos->frecinicio));
        ui->frecfin->setText(QString::number(datos->frecfinal));

        if (datos->constante){
            //altura es lineal: se pinta tal cual (antes se le aplicaba
            //10^(x/20) y el valor derivaba en cada apertura del dialogo).
            //Pendiente fase 7 (B4): marcar el radio "lineal" al restaurar.
            ui->altura->setText(QString::number(datos->altura));
        } else{
            ui->nume->setText(datos->sistema->numeratorString());
            ui->deno->setText(datos->sistema->denominatorString());
            ui->k->setText(QString::number(datos->sistema->gain()->nominal()));
            ui->ret->setText(QString::number(datos->sistema->delay()->nominal()));
        }
    } else {
        ui->altura->setText("");
        ui->nume->setText("");
        ui->deno->setText("");
        ui->k->setText("1");
        ui->ret->setText("0");
    }
}


void IntEspecificaciones::setDatos(dBND *datos, dBND *datos1){

    if (datos->utilizado){

        ui->frecini->setText(QString::number(datos->frecinicio));
        ui->frecfin->setText(QString::number(datos->frecfinal));

        if (datos->constante){
            ui->altura_se1->setText(QString::number(datos->altura));

            ui->altura_se1_2->setText(QString::number(datos1->altura));
        }else{
            ui->NumeSe1->setText(datos->sistema->numeratorString());
            ui->DenoSe1->setText(datos->sistema->denominatorString());
            ui->KSe1->setText(QString::number(datos->sistema->gain()->nominal()));
            ui->RetSe1->setText(QString::number(datos->sistema->delay()->nominal()));

            ui->NumeSe1_2->setText(datos1->sistema->numeratorString());
            ui->DenoSe1_2->setText(datos1->sistema->denominatorString());
            ui->KSe1_2->setText(QString::number(datos1->sistema->gain()->nominal()));
            ui->RetSe1_2->setText(QString::number(datos1->sistema->delay()->nominal()));
        }
    } else {
        ui->altura_se1->setText("");
        ui->NumeSe1->setText("");
        ui->DenoSe1->setText("");
        ui->KSe1->setText("1");
        ui->RetSe1->setText("0");

        ui->altura_se1_2->setText("");
        ui->NumeSe1_2->setText("");
        ui->DenoSe1_2->setText("");
        ui->KSe1_2->setText("1");
        ui->RetSe1_2->setText("0");
    }
}

bool IntEspecificaciones::getDatos(dBND * datos, QString nombre)
{

    if (datos->utilizado && !datos->constante){
        delete datos->sistema;
        datos->constante = false;
        datos->utilizado = false;
    }

    if (ui->frecini->text().isEmpty()){
        datos->frecinicio = omega->first();
    } else {
        ParserX p (pckALL_NON_COMPLEX);
        p.SetExpr(ui->frecini->text().toStdString());

        try {
            datos->frecinicio = p.Eval().GetFloat();
            ui->frecini->setStyleSheet("background : white");
        }catch (ParserError &e){
            datos->utilizado = false;
            ui->frecini->setStyleSheet("background : red");
            menerror("Error al introducir las frecuencias de uso.", "Introducir especificaciones.");
            return false;
        }
    }

    if (ui->frecfin->text().isEmpty()){
        datos->frecfinal = omega->last();
    } else {
        ParserX p (pckALL_NON_COMPLEX);
        p.SetExpr(ui->frecfin->text().toStdString());

        try {
            datos->frecfinal = p.Eval().GetFloat();
            ui->frecfin->setStyleSheet("background : white");
        }catch (ParserError &e){
            datos->utilizado = false;
            ui->frecfin->setStyleSheet("background : red");
            menerror("Error al introducir las frecuencias de uso.", "Introducir especificaciones.");
            return false;
        }
    }

    if (ui->cons->isChecked()){

        if (!ui->altura->text().isEmpty()){

            ParserX p (pckALL_NON_COMPLEX);
            p.SetExpr(ui->altura->text().toStdString());

            datos->constante = true;

            try {

                qreal alt = p.Eval().GetFloat();

                if (ui->decibelios->isChecked()){
                    datos->altura = qftbx::dbToLinear(alt);
                }else {
                    datos->altura = alt;
                }

                ui->altura->setStyleSheet("background : white");
            }catch (ParserError &e){
                datos->utilizado = false;
                ui->altura->setStyleSheet("background : red");
                menerror("Error al introducir la Altura.", "Introducir especificaciones.");
                return false;
            }

            ui->altura->setStyleSheet("background : white");
            datos->utilizado = true;
        } else {
            datos->utilizado = false;
        }
    }else {

        if (ui->deno->text().isEmpty()){
            datos->utilizado = false;
            return true;
        }

        QVector <Parameter *> * nume = NULL;
        QVector <Parameter *> * deno = NULL;
        Parameter * k = NULL;
        Parameter * ret = NULL;

        if (!ui->libre->isChecked()){

            nume = crearNumeradorDenominador(ui->nume->text());

            if (nume == NULL){
                menerror("Error en el numerador.", "Introducir especificaciones.");
                ui->nume->setStyleSheet("background : red");
                datos->utilizado = false;
                return false;
            } else {
                ui->nume->setStyleSheet("background : white");
            }

            deno = crearNumeradorDenominador(ui->deno->text());

            if (deno == NULL){
                menerror("Error en el denominador.", "Introducir especificaciones.");
                ui->deno->setStyleSheet("background : red");
                datos->utilizado = false;
                return false;
            } else {
                ui->deno->setStyleSheet("background : white");
            }

            k = crearKRet(ui->k->text(), true);

            if (k == NULL){
                menerror("Error en la ganancia.", "Introducir especificaciones.");
                ui->k->setStyleSheet("background : red");
                datos->utilizado = false;
                return false;
            } else {
                ui->k->setStyleSheet("background : white");
            }

            ret = crearKRet(ui->ret->text(), false);

            if (ret == NULL){
                menerror("Error en el retardo.", "Introducir especificaciones.");
                ui->ret->setStyleSheet("background : red");
                datos->utilizado = false;
                return false;
            } else {
                ui->ret->setStyleSheet("background : white");
            }
        }


        datos->constante = false;
        if(ui->lfgain->isChecked()){
            datos->sistema = new ZeroPoleGain (nombre, nume, deno, k, ret);
        }else if (ui->hfgain->isChecked()){
            datos->sistema = new TimeConstantGain (nombre, nume, deno, k, ret);
        }else if (ui->poli->isChecked()) {
            datos->sistema = new PolynomialForm (nombre, nume, deno, k, ret);
        }else {
            datos->sistema = new FreeForm (nombre, new QVector <Parameter *> (), new QVector <Parameter *> (), crearKRet(ui->k->text(), true),
                                               crearKRet(ui->ret->text(), false), ui->nume->text(), ui->deno->text());
        }
        datos->utilizado = true;
    }

    datos->nombre = nombre;

    return true;
}

bool IntEspecificaciones::getDatos(dBND *datos, dBND *datos1, QString nombre){

    if (datos->utilizado && !datos->constante){
        delete datos->sistema;
        datos->constante = false;
        datos->utilizado = false;
    }

    if (datos1->utilizado && !datos1->constante){
        delete datos1->sistema;
        datos1->constante = false;
        datos1->utilizado = false;
    }

    if (ui->frecini->text().isEmpty()){
        datos->frecinicio = omega->first();
        datos1->frecinicio = omega->first();
    } else {
        ParserX p (pckALL_NON_COMPLEX);
        p.SetExpr(ui->frecini->text().toStdString());

        try {
            datos->frecinicio = p.Eval().GetFloat();
            datos1->frecinicio = datos->frecinicio;
            ui->frecini->setStyleSheet("background : white");
        }catch (ParserError &e){
            datos->utilizado = false;
            datos1->utilizado = false;
            ui->frecini->setStyleSheet("background : red");
            menerror("Error al introducir las frecuencias de uso.", "Introducir especificaciones.");
            return false;
        }
    }

    if (ui->frecfin->text().isEmpty()){
        datos->frecfinal = omega->last();
        datos1->frecfinal = omega->last();
    } else {
        ParserX p (pckALL_NON_COMPLEX);
        p.SetExpr(ui->frecfin->text().toStdString());

        try {
            datos->frecfinal = p.Eval().GetFloat();
            datos1->frecfinal = datos->frecfinal;
            ui->frecfin->setStyleSheet("background : white");
        }catch (ParserError &e){
            datos->utilizado = false;
            datos1->utilizado = false;
            ui->frecfin->setStyleSheet("background : red");
            menerror("Error al introducir las frecuencias de uso.", "Introducir especificaciones.");
            return false;
        }
    }

    if (ui->cons->isChecked()){

        if (!ui->altura_se1->text().isEmpty()){

            ParserX p (pckALL_NON_COMPLEX);
            p.SetExpr(ui->altura_se1->text().toStdString());

            datos->constante = true;

            try {

                qreal alt = p.Eval().GetFloat();

                //dBND::altura es magnitud LINEAL: antes esta ruta tenia las
                //dos ramas intercambiadas respecto a la ruta simple.
                if (ui->decibelios_se1->isChecked()){
                    datos->altura = qftbx::dbToLinear(alt);
                }else {
                    datos->altura = alt;
                }

                ui->altura_se1->setStyleSheet("background : white");
            }catch (ParserError &e){
                datos->utilizado = false;
                ui->altura_se1->setStyleSheet("background : red");
                menerror("Error al introducir la Altura.", "Introducir especificaciones.");
                return false;
            }

            ui->altura_se1->setStyleSheet("background : white");
            datos->utilizado = true;
        } else {
            datos->utilizado = false;
        }
    }else {

        if (ui->DenoSe1->text().isEmpty()){
            datos->utilizado = false;
            return true;
        }

        QVector <Parameter *> * nume = NULL;
        QVector <Parameter *> * deno = NULL;
        Parameter * k = NULL;
        Parameter * ret = NULL;

        if(!ui->FLSe1->isChecked()){

            nume = crearNumeradorDenominador(ui->NumeSe1->text());

            if (nume == NULL){
                menerror("Error en el numerador.", "Introducir especificaciones.");
                ui->NumeSe1->setStyleSheet("background : red");
                datos->utilizado = false;
                return false;
            } else {
                ui->NumeSe1->setStyleSheet("background : white");
            }

            deno = crearNumeradorDenominador(ui->DenoSe1->text());

            if (deno == NULL){
                menerror("Error en el denominador.", "Introducir especificaciones.");
                ui->DenoSe1->setStyleSheet("background : red");
                datos->utilizado = false;
                return false;
            } else {
                ui->DenoSe1->setStyleSheet("background : white");
            }

            k = crearKRet(ui->KSe1->text(), true);

            if (k == NULL){
                menerror("Error en la ganancia.", "Introducir especificaciones.");
                ui->KSe1->setStyleSheet("background : red");
                datos->utilizado = false;
                return false;
            } else {
                ui->KSe1->setStyleSheet("background : white");
            }

            ret = crearKRet(ui->RetSe1->text(), false);

            if (ret == NULL){
                menerror("Error en el retardo.", "Introducir especificaciones.");
                ui->RetSe1->setStyleSheet("background : red");
                datos->utilizado = false;
                return false;
            } else {
                ui->RetSe1->setStyleSheet("background : white");
            }

        }


        datos->constante = false;
        if(ui->KGSe1->isChecked()){
            datos->sistema = new ZeroPoleGain (nombre, nume, deno, k, ret);
        }else if (ui->KNGSe1->isChecked()){
            datos->sistema = new TimeConstantGain (nombre, nume, deno, k, ret);
        }else if (ui->CPoliSe1->isChecked()) {
            datos->sistema = new PolynomialForm (nombre, nume, deno, k, ret);
        }else {
            datos->sistema = new FreeForm (nombre, new QVector <Parameter *> (), new QVector <Parameter *> (), crearKRet(ui->KSe1->text(), true),
                                               crearKRet(ui->RetSe1->text(), false), ui->NumeSe1->text(), ui->DenoSe1->text());
        }
        datos->utilizado = true;
    }


    if (ui->cons->isChecked()){

        if (!ui->altura_se1_2->text().isEmpty()){

            ParserX p (pckALL_NON_COMPLEX);
            p.SetExpr(ui->altura_se1_2->text().toStdString());

            datos1->constante = true;

            try {

                qreal alt = p.Eval().GetFloat();

                if (ui->decibelios_se1_2->isChecked()){
                    datos1->altura = qftbx::dbToLinear(alt);
                }else {
                    datos1->altura = alt;
                }

                ui->altura_se1_2->setStyleSheet("background : white");
            }catch (ParserError &e){
                datos1->utilizado = false;
                ui->altura_se1_2->setStyleSheet("background : red");
                menerror("Error al introducir la Altura.", "Introducir especificaciones.");
                return false;
            }

            ui->altura_se1_2->setStyleSheet("background : white");
            datos1->utilizado = true;
        } else {
            datos1->utilizado = false;
        }
    }else {

        if (ui->DenoSe1_2->text().isEmpty()){
            datos1->utilizado = false;
            return true;
        }

        QVector <Parameter *> * nume = NULL;
        QVector <Parameter *> * deno = NULL;
        Parameter * k = NULL;
        Parameter * ret = NULL;

        if (!ui->FLSe1_2->isChecked()){

            nume = crearNumeradorDenominador(ui->NumeSe1_2->text());

            if (nume == NULL){
                menerror("Error en el numerador.", "Introducir especificaciones.");
                ui->NumeSe1_2->setStyleSheet("background : red");
                datos1->utilizado = false;
                return false;
            } else {
                ui->NumeSe1_2->setStyleSheet("background : white");
            }

            deno = crearNumeradorDenominador(ui->DenoSe1_2->text());

            if (deno == NULL){
                menerror("Error en el denominador.", "Introducir especificaciones.");
                ui->DenoSe1_2->setStyleSheet("background : red");
                datos1->utilizado = false;
                return false;
            } else {
                ui->DenoSe1_2->setStyleSheet("background : white");
            }

            k = crearKRet(ui->KSe1_2->text(), true);

            if (k == NULL){
                menerror("Error en la ganancia.", "Introducir especificaciones.");
                ui->KSe1_2->setStyleSheet("background : red");
                datos1->utilizado = false;
                return false;
            } else {
                ui->KSe1_2->setStyleSheet("background : white");
            }

            ret = crearKRet(ui->RetSe1_2->text(), false);

            if (ret == NULL){
                menerror("Error en el retardo.", "Introducir especificaciones.");
                ui->RetSe1_2->setStyleSheet("background : red");
                datos1->utilizado = false;
                return false;
            } else {
                ui->RetSe1_2->setStyleSheet("background : white");
            }
        }

        datos1->constante = false;
        if(ui->KGSe1_2->isChecked()){
            datos1->sistema = new ZeroPoleGain (nombre, nume, deno, k, ret);
        }else if (ui->KNGSe1_2->isChecked()){
            datos1->sistema = new TimeConstantGain (nombre, nume, deno, k, ret);
        }else if (ui->CPoliSe1_2->isChecked()) {
            datos1->sistema = new PolynomialForm (nombre, nume, deno, k, ret);
        }else {
            datos1->sistema = new FreeForm (nombre, new QVector <Parameter *> (), new QVector <Parameter *> (), crearKRet(ui->KSe1_2->text(), true),
                                                crearKRet(ui->RetSe1_2->text(), false), ui->NumeSe1_2->text(), ui->DenoSe1_2->text());
        }
        datos1->utilizado = true;
    }

    datos->nombre = nombre;
    datos1->nombre = QStringLiteral("TrackingUpper");

    return true;
}

Parameter * IntEspecificaciones::crearKRet(QString linea, bool isK){
    ParserX p (pckALL_NON_COMPLEX);

    if (linea.isEmpty()){
        if (isK){
            return new Parameter (1);
        }else{
            return new Parameter (qreal(0));
        }
    }else{
        qreal res;
        p.SetExpr(linea.toStdString());
        try {
            res = p.Eval().GetFloat();
        }catch (ParserError &e){
            return NULL;
        }

        return new Parameter(res);
    }
}

QVector <Parameter * > * IntEspecificaciones::crearNumeradorDenominador(QString linea){

    ParserX p (pckALL_NON_COMPLEX);

    QVector <QString> * numeros = tools::srtovectorString(linea);

    QVector <Parameter *> * var = new QVector <Parameter *> ();
    var->reserve(numeros->size());

    if (numeros->isEmpty()){
        return var;
    }

    foreach (const QString &string, *numeros) {
        p.SetExpr(string.toStdString());
        qreal res;
        try {
            res = p.Eval().GetFloat();
        }catch (ParserError &e){
            numeros->clear();
            var->clear();
            return NULL;
        }

        var->append(new Parameter(res));
    }

    numeros->clear();

    return var;
}

void IntEspecificaciones::seleccionar()
{
    if (activado == 1){
        getDatos(seguimiento, seguimiento_2, "TrackingLower");
    }else if (activado == 2){
        getDatos(estabilidad, "Stability");
    }else if (activado == 3){
        getDatos(ruido, "SensorNoise");
    }else if (activado == 4){
        getDatos(RPS, "OutputDisturbance");
    }else if (activado == 5){
        getDatos(RPE, "InputDisturbance");
    }else if (activado == 6){
        getDatos(EC, "ControlEffort");
    }
}

void IntEspecificaciones::on_radioButton_clicked()
{
    ui->principal->setCurrentIndex(1);
    seleccionar();
    activado = 1;
    setDatos(seguimiento, seguimiento_2);
    this->resize(867, 363);
    ui->layoutWidget6->move(670, 320);
}

void IntEspecificaciones::on_radioButton_2_clicked()
{
    ui->principal->setCurrentIndex(0);
    seleccionar();
    activado = 2;
    setDatos(estabilidad);
    ui->imagen->setPixmap(estabilidad_img);
    this->resize(647, 363);
    ui->layoutWidget6->move(450, 320);
}

void IntEspecificaciones::on_radioButton_3_clicked()
{
    ui->principal->setCurrentIndex(0);
    seleccionar();
    activado = 3;
    setDatos(ruido);
    ui->imagen->setPixmap(ruidosensor_img);
    this->resize(647, 363);
    ui->layoutWidget6->move(450, 320);
}

void IntEspecificaciones::on_radioButton_4_clicked()
{
    ui->principal->setCurrentIndex(0);
    seleccionar();
    activado = 4;
    setDatos(RPS);
    ui->imagen->setPixmap(RPS_img);
    this->resize(647, 363);
}

void IntEspecificaciones::on_radioButton_5_clicked()
{
    ui->principal->setCurrentIndex(0);
    seleccionar();
    activado = 5;
    setDatos(RPE);
    ui->imagen->setPixmap(RPE_img);
    this->resize(647, 363);
    ui->layoutWidget6->move(450, 320);
}

void IntEspecificaciones::on_radioButton_6_clicked()
{
    ui->principal->setCurrentIndex(0);
    seleccionar();
    activado = 6;
    setDatos(EC);
    ui->imagen->setPixmap(EC_img);
    this->resize(647, 363);
    ui->layoutWidget6->move(450, 320);
}

void IntEspecificaciones::on_cons_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->Se_1->setCurrentIndex(1);
    ui->Se_2->setCurrentIndex(1);
}

void IntEspecificaciones::on_fun_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
    ui->Se_1->setCurrentIndex(2);
    ui->Se_2->setCurrentIndex(2);
}

void IntEspecificaciones::on_Cancel_clicked()
{
    emit(close());
}

void IntEspecificaciones::on_OK_clicked()
{
    //El vector anterior es ya propiedad del DAO (se le entrego): aqui solo
    //se suelta la referencia. Antes se fugaba un QVector por cada Aceptar.
    retorno = NULL;

    bool correcto = true;

    if (ui->radioButton->isChecked()){
        correcto = getDatos(seguimiento, seguimiento_2, "TrackingLower");
    }else if (ui->radioButton_2->isChecked()){
        correcto = getDatos(estabilidad, "Stability");
    }else if (ui->radioButton_3->isChecked()){
        correcto = getDatos(ruido, "SensorNoise");
    }else if (ui->radioButton_4->isChecked()){
        correcto = getDatos(RPS, "OutputDisturbance");
    }else if (ui->radioButton_5->isChecked()){
        correcto = getDatos(RPE, "InputDisturbance");
    }else if (ui->radioButton_6->isChecked()){
        correcto =  getDatos(EC, "ControlEffort");
    }

    if (!correcto){
        return;
    }

    retorno = new QVector <dBND *> ();

    //El DAO toma propiedad: se le entregan clones profundos y el dialogo
    //conserva sus originales para seguir editando.
    retorno->append(seguimiento->clone());
    retorno->append(seguimiento_2->clone());
    retorno->append(estabilidad->clone());
    retorno->append(ruido->clone());
    retorno->append(RPS->clone());
    retorno->append(RPE->clone());
    retorno->append(EC->clone());

    controlador->setEspecificaciones(retorno);

    todoCorrecto = true;

    emit (close());
}

bool IntEspecificaciones::getTodoCorrecto(){
    return todoCorrecto;
}



void IntEspecificaciones::on_CPoliSe1_clicked()
{
    ui->imagen_se_1-> setCurrentIndex(3);
}

void IntEspecificaciones::on_FLSe1_clicked()
{
    ui->imagen_se_1-> setCurrentIndex(0);
}

void IntEspecificaciones::on_KGSe1_clicked()
{
    ui->imagen_se_1-> setCurrentIndex(1);
}

void IntEspecificaciones::on_KNGSe1_clicked()
{
    ui->imagen_se_1-> setCurrentIndex(2);
}

void IntEspecificaciones::on_CPoliSe1_2_clicked()
{
    ui->imagen_se_2-> setCurrentIndex(3);
}

void IntEspecificaciones::on_KGSe1_2_clicked()
{
    ui->imagen_se_2-> setCurrentIndex(1);
}

void IntEspecificaciones::on_KNGSe1_2_clicked()
{
    ui->imagen_se_2-> setCurrentIndex(2);
}

void IntEspecificaciones::on_FLSe1_2_clicked()
{
    ui->imagen_se_2-> setCurrentIndex(0);
}

void IntEspecificaciones::on_poli_clicked()
{
    ui->figures->setCurrentIndex(1);
}

void IntEspecificaciones::on_hfgain_clicked()
{
    ui->figures->setCurrentIndex(3);
}

void IntEspecificaciones::on_libre_clicked()
{
    ui->figures->setCurrentIndex(0);
}

void IntEspecificaciones::on_lfgain_clicked()
{
    ui->figures->setCurrentIndex(2);
}

