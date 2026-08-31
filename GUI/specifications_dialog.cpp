#include "specifications_dialog.h"

#include "src/core/specifications/specification.h"
#include "ui_specifications_dialog.h"

#include "GUI/error_message.h"
#include "GUI/plot_palette.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"

using namespace tools;
using namespace mup;

SpecificationsDialog::SpecificationsDialog(Controlador *controlador, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SpecificationsDialog)
{
    ui->setupUi(this);

    this->controlador = controlador;
    this->omega = controlador->getOmega()->getValores();

    setWindowTitle(tr("Specifications input"));

    retorno = nullptr;

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

    //Estados por defecto de los radios (el .ui no marca ninguno): constante,
    //forma polinomica y unidades lineales. Sin tipo marcado, la cascada de
    //lectura caia en un FreeForm accidental.
    ui->cons->setChecked(true);
    on_cons_clicked();
    ui->lineal->setChecked(true);
    ui->linea_se1->setChecked(true);
    ui->linea_se1_2->setChecked(true);
    ui->poli->setChecked(true);
    on_poli_clicked();
    ui->CPoliSe1->setChecked(true);
    on_CPoliSe1_clicked();
    ui->CPoliSe1_2->setChecked(true);
    on_CPoliSe1_2_clicked();

    //Si el proyecto trae especificaciones (fichero cargado), el dialogo parte
    //de ELLAS: antes arrancaba con 7 registros vacios y el primer Aceptar
    //machacaba lo cargado (perdida de datos silenciosa).
    QVector <dBND *> * cargadas = controlador->getEspecificaciones();
    if (cargadas != nullptr && cargadas->size() == 7){
        delete seguimiento;
        delete seguimiento_2;
        delete estabilidad;
        delete ruido;
        delete RPS;
        delete RPE;
        delete EC;

        seguimiento = cargadas->at(0)->clone();
        seguimiento_2 = cargadas->at(1)->clone();
        estabilidad = cargadas->at(2)->clone();
        ruido = cargadas->at(3)->clone();
        RPS = cargadas->at(4)->clone();
        RPE = cargadas->at(5)->clone();
        EC = cargadas->at(6)->clone();
    }

    //Pestana de seguimiento seleccionada y restaurada desde el arranque: sin
    //esto ningun radio estaba marcado y Aceptar publicaba los 7 vacios.
    ui->radioButton->setChecked(true);
    on_radioButton_clicked();

    todoCorrecto = false;
}

SpecificationsDialog::~SpecificationsDialog()
{
    //Los 7 registros de trabajo (y sus plantas) son del dialogo: el DAO
    //recibio clones profundos.
    dBND * registros[] = {seguimiento, seguimiento_2, estabilidad,
                          ruido, RPS, RPE, EC};
    for (dBND * registro : registros) {
        delete registro->sistema;
        delete registro;
    }

    delete ui;
}

//Coeficientes nominales en el formato que espera crearNumeradorDenominador
//(separados por espacios). Los tipos no-FreeForm no tienen representacion
//textual propia: antes se pintaba numeratorString()=="" y la especificacion
//se perdia en silencio al reabrir.
QString SpecificationsDialog::coefficientsText(QVector <Parameter *> * parametros)
{
    QString texto;
    foreach (Parameter * parametro, *parametros) {
        texto += QString::number(parametro->nominal()) + " ";
    }
    return texto.trimmed();
}

QString SpecificationsDialog::numeratorText(LtiSystem * sistema)
{
    if (sistema->type() == LtiSystem::SystemType::FreeForm){
        return sistema->numeratorString();
    }
    return coefficientsText(sistema->numerator());
}

QString SpecificationsDialog::denominatorText(LtiSystem * sistema)
{
    if (sistema->type() == LtiSystem::SystemType::FreeForm){
        return sistema->denominatorString();
    }
    return coefficientsText(sistema->denominator());
}

void SpecificationsDialog::setDatos(dBND * datos)
{
    if (datos->utilizado){

        ui->frecini->setText(QString::number(datos->frecinicio));
        ui->frecfin->setText(QString::number(datos->frecfinal));

        if (datos->constante){
            ui->cons->setChecked(true);
            on_cons_clicked();
            //altura es lineal: se pinta tal cual y se marca el radio lineal
            //para que Aceptar no la reinterprete como dB.
            ui->lineal->setChecked(true);
            ui->altura->setText(QString::number(datos->altura));
        } else{
            ui->fun->setChecked(true);
            on_fun_clicked();

            switch (datos->sistema->type()){
            case LtiSystem::SystemType::ZeroPoleGain:
                ui->lfgain->setChecked(true);
                on_lfgain_clicked();
                break;
            case LtiSystem::SystemType::TimeConstantGain:
                ui->hfgain->setChecked(true);
                on_hfgain_clicked();
                break;
            case LtiSystem::SystemType::PolynomialForm:
                ui->poli->setChecked(true);
                on_poli_clicked();
                break;
            default:
                ui->libre->setChecked(true);
                on_libre_clicked();
                break;
            }

            ui->nume->setText(numeratorText(datos->sistema));
            ui->deno->setText(denominatorText(datos->sistema));
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


void SpecificationsDialog::setDatos(dBND *datos, dBND *datos1){

    if (datos->utilizado){

        ui->frecini->setText(QString::number(datos->frecinicio));
        ui->frecfin->setText(QString::number(datos->frecfinal));

        if (datos->constante){
            ui->cons->setChecked(true);
            on_cons_clicked();
            ui->linea_se1->setChecked(true);
            ui->linea_se1_2->setChecked(true);
            ui->altura_se1->setText(QString::number(datos->altura));

            ui->altura_se1_2->setText(QString::number(datos1->altura));
        }else{
            ui->fun->setChecked(true);
            on_fun_clicked();

            switch (datos->sistema->type()){
            case LtiSystem::SystemType::ZeroPoleGain:
                ui->KGSe1->setChecked(true);
                on_KGSe1_clicked();
                break;
            case LtiSystem::SystemType::TimeConstantGain:
                ui->KNGSe1->setChecked(true);
                on_KNGSe1_clicked();
                break;
            case LtiSystem::SystemType::PolynomialForm:
                ui->CPoliSe1->setChecked(true);
                on_CPoliSe1_clicked();
                break;
            default:
                ui->FLSe1->setChecked(true);
                on_FLSe1_clicked();
                break;
            }

            switch (datos1->sistema->type()){
            case LtiSystem::SystemType::ZeroPoleGain:
                ui->KGSe1_2->setChecked(true);
                on_KGSe1_2_clicked();
                break;
            case LtiSystem::SystemType::TimeConstantGain:
                ui->KNGSe1_2->setChecked(true);
                on_KNGSe1_2_clicked();
                break;
            case LtiSystem::SystemType::PolynomialForm:
                ui->CPoliSe1_2->setChecked(true);
                on_CPoliSe1_2_clicked();
                break;
            default:
                ui->FLSe1_2->setChecked(true);
                on_FLSe1_2_clicked();
                break;
            }

            ui->NumeSe1->setText(numeratorText(datos->sistema));
            ui->DenoSe1->setText(denominatorText(datos->sistema));
            ui->KSe1->setText(QString::number(datos->sistema->gain()->nominal()));
            ui->RetSe1->setText(QString::number(datos->sistema->delay()->nominal()));

            ui->NumeSe1_2->setText(numeratorText(datos1->sistema));
            ui->DenoSe1_2->setText(denominatorText(datos1->sistema));
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

bool SpecificationsDialog::getDatos(dBND * datos, QString nombre)
{

    if (datos->utilizado && !datos->constante){
        //sistema debe quedar nulo: si esta lectura acaba en no-utilizado, el
        //clone() de Aceptar clonaba un puntero colgante.
        delete datos->sistema;
        datos->sistema = nullptr;
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
            errorMessage(tr("Invalid frequency band."), tr("Specifications input"));
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
            errorMessage(tr("Invalid frequency band."), tr("Specifications input"));
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
                errorMessage(tr("Invalid magnitude value."), tr("Specifications input"));
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

        QVector <Parameter *> * nume = nullptr;
        QVector <Parameter *> * deno = nullptr;

        //Ganancia y retardo se validan SIEMPRE: la rama libre construia el
        //FreeForm con los crearKRet sin comprobar (nullptr ante un error de
        //sintaxis y crash posterior).
        Parameter * k = crearKRet(ui->k->text(), true);

        if (k == nullptr){
            errorMessage(tr("Invalid gain."), tr("Specifications input"));
            ui->k->setStyleSheet("background : red");
            datos->utilizado = false;
            return false;
        }
        ui->k->setStyleSheet("background : white");

        Parameter * ret = crearKRet(ui->ret->text(), false);

        if (ret == nullptr){
            errorMessage(tr("Invalid delay."), tr("Specifications input"));
            ui->ret->setStyleSheet("background : red");
            delete k;
            datos->utilizado = false;
            return false;
        }
        ui->ret->setStyleSheet("background : white");

        if (!ui->libre->isChecked()){

            nume = crearNumeradorDenominador(ui->nume->text());

            if (nume == nullptr){
                errorMessage(tr("Invalid numerator."), tr("Specifications input"));
                ui->nume->setStyleSheet("background : red");
                delete k;
                delete ret;
                datos->utilizado = false;
                return false;
            }
            ui->nume->setStyleSheet("background : white");

            deno = crearNumeradorDenominador(ui->deno->text());

            if (deno == nullptr){
                errorMessage(tr("Invalid denominator."), tr("Specifications input"));
                ui->deno->setStyleSheet("background : red");
                qDeleteAll(*nume);
                delete nume;
                delete k;
                delete ret;
                datos->utilizado = false;
                return false;
            }
            ui->deno->setStyleSheet("background : white");
        }


        datos->constante = false;
        if(ui->lfgain->isChecked()){
            datos->sistema = new ZeroPoleGain (nombre, nume, deno, k, ret);
        }else if (ui->hfgain->isChecked()){
            datos->sistema = new TimeConstantGain (nombre, nume, deno, k, ret);
        }else if (ui->poli->isChecked()) {
            datos->sistema = new PolynomialForm (nombre, nume, deno, k, ret);
        }else {
            //Si se llego aqui con coeficientes construidos (sin radio de
            //tipo marcado), no deben fugarse.
            if (nume != nullptr){
                qDeleteAll(*nume);
                delete nume;
            }
            if (deno != nullptr){
                qDeleteAll(*deno);
                delete deno;
            }
            datos->sistema = new FreeForm (nombre, new QVector <Parameter *> (), new QVector <Parameter *> (), k, ret,
                                               ui->nume->text(), ui->deno->text());
        }
        datos->utilizado = true;
    }

    datos->nombre = nombre;

    return true;
}

bool SpecificationsDialog::getDatos(dBND *datos, dBND *datos1, QString nombre){

    if (datos->utilizado && !datos->constante){
        delete datos->sistema;
        datos->sistema = nullptr;
        datos->constante = false;
        datos->utilizado = false;
    }

    if (datos1->utilizado && !datos1->constante){
        delete datos1->sistema;
        datos1->sistema = nullptr;
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
            errorMessage(tr("Invalid frequency band."), tr("Specifications input"));
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
            errorMessage(tr("Invalid frequency band."), tr("Specifications input"));
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
                errorMessage(tr("Invalid magnitude value."), tr("Specifications input"));
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

        QVector <Parameter *> * nume = nullptr;
        QVector <Parameter *> * deno = nullptr;

        //Ganancia y retardo se validan SIEMPRE: la rama libre construia el
        //FreeForm con los crearKRet sin comprobar (nullptr ante un error de
        //sintaxis y crash posterior).
        Parameter * k = crearKRet(ui->KSe1->text(), true);

        if (k == nullptr){
            errorMessage(tr("Invalid gain."), tr("Specifications input"));
            ui->KSe1->setStyleSheet("background : red");
            datos->utilizado = false;
            return false;
        }
        ui->KSe1->setStyleSheet("background : white");

        Parameter * ret = crearKRet(ui->RetSe1->text(), false);

        if (ret == nullptr){
            errorMessage(tr("Invalid delay."), tr("Specifications input"));
            ui->RetSe1->setStyleSheet("background : red");
            delete k;
            datos->utilizado = false;
            return false;
        }
        ui->RetSe1->setStyleSheet("background : white");

        if (!ui->FLSe1->isChecked()){

            nume = crearNumeradorDenominador(ui->NumeSe1->text());

            if (nume == nullptr){
                errorMessage(tr("Invalid numerator."), tr("Specifications input"));
                ui->NumeSe1->setStyleSheet("background : red");
                delete k;
                delete ret;
                datos->utilizado = false;
                return false;
            }
            ui->NumeSe1->setStyleSheet("background : white");

            deno = crearNumeradorDenominador(ui->DenoSe1->text());

            if (deno == nullptr){
                errorMessage(tr("Invalid denominator."), tr("Specifications input"));
                ui->DenoSe1->setStyleSheet("background : red");
                qDeleteAll(*nume);
                delete nume;
                delete k;
                delete ret;
                datos->utilizado = false;
                return false;
            }
            ui->DenoSe1->setStyleSheet("background : white");
        }


        datos->constante = false;
        if(ui->KGSe1->isChecked()){
            datos->sistema = new ZeroPoleGain (nombre, nume, deno, k, ret);
        }else if (ui->KNGSe1->isChecked()){
            datos->sistema = new TimeConstantGain (nombre, nume, deno, k, ret);
        }else if (ui->CPoliSe1->isChecked()) {
            datos->sistema = new PolynomialForm (nombre, nume, deno, k, ret);
        }else {
            //Si se llego aqui con coeficientes construidos (sin radio de
            //tipo marcado), no deben fugarse.
            if (nume != nullptr){
                qDeleteAll(*nume);
                delete nume;
            }
            if (deno != nullptr){
                qDeleteAll(*deno);
                delete deno;
            }
            datos->sistema = new FreeForm (nombre, new QVector <Parameter *> (), new QVector <Parameter *> (), k, ret,
                                               ui->NumeSe1->text(), ui->DenoSe1->text());
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
                errorMessage(tr("Invalid magnitude value."), tr("Specifications input"));
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

        QVector <Parameter *> * nume = nullptr;
        QVector <Parameter *> * deno = nullptr;

        //Ganancia y retardo se validan SIEMPRE: la rama libre construia el
        //FreeForm con los crearKRet sin comprobar (nullptr ante un error de
        //sintaxis y crash posterior).
        Parameter * k = crearKRet(ui->KSe1_2->text(), true);

        if (k == nullptr){
            errorMessage(tr("Invalid gain."), tr("Specifications input"));
            ui->KSe1_2->setStyleSheet("background : red");
            datos1->utilizado = false;
            return false;
        }
        ui->KSe1_2->setStyleSheet("background : white");

        Parameter * ret = crearKRet(ui->RetSe1_2->text(), false);

        if (ret == nullptr){
            errorMessage(tr("Invalid delay."), tr("Specifications input"));
            ui->RetSe1_2->setStyleSheet("background : red");
            delete k;
            datos1->utilizado = false;
            return false;
        }
        ui->RetSe1_2->setStyleSheet("background : white");

        if (!ui->FLSe1_2->isChecked()){

            nume = crearNumeradorDenominador(ui->NumeSe1_2->text());

            if (nume == nullptr){
                errorMessage(tr("Invalid numerator."), tr("Specifications input"));
                ui->NumeSe1_2->setStyleSheet("background : red");
                delete k;
                delete ret;
                datos1->utilizado = false;
                return false;
            }
            ui->NumeSe1_2->setStyleSheet("background : white");

            deno = crearNumeradorDenominador(ui->DenoSe1_2->text());

            if (deno == nullptr){
                errorMessage(tr("Invalid denominator."), tr("Specifications input"));
                ui->DenoSe1_2->setStyleSheet("background : red");
                qDeleteAll(*nume);
                delete nume;
                delete k;
                delete ret;
                datos1->utilizado = false;
                return false;
            }
            ui->DenoSe1_2->setStyleSheet("background : white");
        }

        datos1->constante = false;
        if(ui->KGSe1_2->isChecked()){
            datos1->sistema = new ZeroPoleGain (nombre, nume, deno, k, ret);
        }else if (ui->KNGSe1_2->isChecked()){
            datos1->sistema = new TimeConstantGain (nombre, nume, deno, k, ret);
        }else if (ui->CPoliSe1_2->isChecked()) {
            datos1->sistema = new PolynomialForm (nombre, nume, deno, k, ret);
        }else {
            //Si se llego aqui con coeficientes construidos (sin radio de
            //tipo marcado), no deben fugarse.
            if (nume != nullptr){
                qDeleteAll(*nume);
                delete nume;
            }
            if (deno != nullptr){
                qDeleteAll(*deno);
                delete deno;
            }
            datos1->sistema = new FreeForm (nombre, new QVector <Parameter *> (), new QVector <Parameter *> (), k, ret,
                                                ui->NumeSe1_2->text(), ui->DenoSe1_2->text());
        }
        datos1->utilizado = true;
    }

    datos->nombre = nombre;
    datos1->nombre = QStringLiteral("TrackingUpper");

    return true;
}

Parameter * SpecificationsDialog::crearKRet(QString linea, bool isK){
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
            return nullptr;
        }

        return new Parameter(res);
    }
}

QVector <Parameter * > * SpecificationsDialog::crearNumeradorDenominador(QString linea){

    ParserX p (pckALL_NON_COMPLEX);

    QVector <QString> * numeros = tools::srtovectorString(linea);

    QVector <Parameter *> * var = new QVector <Parameter *> ();
    var->reserve(numeros->size());

    if (numeros->isEmpty()){
        delete numeros;
        return var;
    }

    foreach (const QString &string, *numeros) {
        p.SetExpr(string.toStdString());
        qreal res;
        try {
            res = p.Eval().GetFloat();
        }catch (ParserError &e){
            //Antes se abandonaban los Parameter ya creados y ambos vectores.
            delete numeros;
            qDeleteAll(*var);
            delete var;
            return nullptr;
        }

        var->append(new Parameter(res));
    }

    delete numeros;

    return var;
}

void SpecificationsDialog::seleccionar()
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

void SpecificationsDialog::on_radioButton_clicked()
{
    ui->principal->setCurrentIndex(1);
    seleccionar();
    activado = 1;
    setDatos(seguimiento, seguimiento_2);
    this->resize(867, 363);
    ui->layoutWidget6->move(670, 320);
}

void SpecificationsDialog::on_radioButton_2_clicked()
{
    ui->principal->setCurrentIndex(0);
    seleccionar();
    activado = 2;
    setDatos(estabilidad);
    ui->imagen->setPixmap(estabilidad_img);
    this->resize(647, 363);
    ui->layoutWidget6->move(450, 320);
}

void SpecificationsDialog::on_radioButton_3_clicked()
{
    ui->principal->setCurrentIndex(0);
    seleccionar();
    activado = 3;
    setDatos(ruido);
    ui->imagen->setPixmap(ruidosensor_img);
    this->resize(647, 363);
    ui->layoutWidget6->move(450, 320);
}

void SpecificationsDialog::on_radioButton_4_clicked()
{
    ui->principal->setCurrentIndex(0);
    seleccionar();
    activado = 4;
    setDatos(RPS);
    ui->imagen->setPixmap(RPS_img);
    this->resize(647, 363);
}

void SpecificationsDialog::on_radioButton_5_clicked()
{
    ui->principal->setCurrentIndex(0);
    seleccionar();
    activado = 5;
    setDatos(RPE);
    ui->imagen->setPixmap(RPE_img);
    this->resize(647, 363);
    ui->layoutWidget6->move(450, 320);
}

void SpecificationsDialog::on_radioButton_6_clicked()
{
    ui->principal->setCurrentIndex(0);
    seleccionar();
    activado = 6;
    setDatos(EC);
    ui->imagen->setPixmap(EC_img);
    this->resize(647, 363);
    ui->layoutWidget6->move(450, 320);
}

void SpecificationsDialog::on_cons_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->Se_1->setCurrentIndex(1);
    ui->Se_2->setCurrentIndex(1);
}

void SpecificationsDialog::on_fun_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
    ui->Se_1->setCurrentIndex(2);
    ui->Se_2->setCurrentIndex(2);
}

void SpecificationsDialog::on_Cancel_clicked()
{
    emit(close());
}

void SpecificationsDialog::on_OK_clicked()
{
    //El vector anterior es ya propiedad del DAO (se le entrego): aqui solo
    //se suelta la referencia. Antes se fugaba un QVector por cada Aceptar.
    retorno = nullptr;

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

bool SpecificationsDialog::getTodoCorrecto(){
    return todoCorrecto;
}



void SpecificationsDialog::on_CPoliSe1_clicked()
{
    ui->imagen_se_1-> setCurrentIndex(3);
}

void SpecificationsDialog::on_FLSe1_clicked()
{
    ui->imagen_se_1-> setCurrentIndex(0);
}

void SpecificationsDialog::on_KGSe1_clicked()
{
    ui->imagen_se_1-> setCurrentIndex(1);
}

void SpecificationsDialog::on_KNGSe1_clicked()
{
    ui->imagen_se_1-> setCurrentIndex(2);
}

void SpecificationsDialog::on_CPoliSe1_2_clicked()
{
    ui->imagen_se_2-> setCurrentIndex(3);
}

void SpecificationsDialog::on_KGSe1_2_clicked()
{
    ui->imagen_se_2-> setCurrentIndex(1);
}

void SpecificationsDialog::on_KNGSe1_2_clicked()
{
    ui->imagen_se_2-> setCurrentIndex(2);
}

void SpecificationsDialog::on_FLSe1_2_clicked()
{
    ui->imagen_se_2-> setCurrentIndex(0);
}

void SpecificationsDialog::on_poli_clicked()
{
    ui->figures->setCurrentIndex(1);
}

void SpecificationsDialog::on_hfgain_clicked()
{
    ui->figures->setCurrentIndex(3);
}

void SpecificationsDialog::on_libre_clicked()
{
    ui->figures->setCurrentIndex(0);
}

void SpecificationsDialog::on_lfgain_clicked()
{
    ui->figures->setCurrentIndex(2);
}

