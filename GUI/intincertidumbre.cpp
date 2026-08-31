#include "intincertidumbre.h"
#include "ui_intincertidumbre.h"

#include "GUI/menerror.h"
#include "GUI/plot_palette.h"

using namespace tools;

IntIncertidumbre::IntIncertidumbre(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IntIncertidumbre)
{
    rango = false;
    ui->setupUi(this);

    ui->boxNume->setVisible(false);
    ui->boxDeno->setVisible(false);

    rango = false;

    setWindowTitle("Introducir Incertidumbre Planta");

    connect(ui->cancelar, SIGNAL(clicked()), this, SLOT(close()));
}

IntIncertidumbre::~IntIncertidumbre(){

    delete ui;

    if (rango == true){
        delete layoutnume;
        delete layoutdeno;
        //Los ParLineEdit y los Parameter de trabajo son del dialogo (la
        //planta recibe clones): antes se abandonaban con clear().
        for (ParLineEdit * par : *parNume){
            delete par;
        }
        delete parNume;
        for (ParLineEdit * par : *parDeno){
            delete par;
        }
        delete parDeno;
        qDeleteAll(*numerador);
        delete numerador;
        qDeleteAll(*denominador);
        delete denominador;
        delete cajas;
    }

    liberarTablas();
}

//Las tablas de entrada llegan de IntroducirPlanta y pasan a ser del dialogo.
void IntIncertidumbre::liberarTablas(){
    if (tabla != nullptr){
        qDeleteAll(*tabla);
        delete tabla;
        tabla = nullptr;
    }
    if (exp != nullptr){
        qDeleteAll(*exp);
        delete exp;
        exp = nullptr;
    }
    if (isVar != nullptr){
        qDeleteAll(*isVar);
        delete isVar;
        isVar = nullptr;
    }
}


bool IntIncertidumbre::lanzarViewIncer(QVector <QVector <QString> * > * tabla, QVector <QVector <QString> * > * exp,
                                       QVector<QVector<bool> *> *isVar, bool rango){

    //Cada lanzamiento reemplaza las tablas anteriores (se fugaban).
    liberarTablas();
    aceptado = false;

    this->tabla = tabla;
    this->exp = exp;
    this->isVar = isVar;

    this->controlador = rango;

    if (rango){
        ui->label->setVisible(false);
        ui->label_2->setVisible(false);
        ui->label_3->setVisible(false);
        ui->rInicioK->setVisible(false);
        ui->rFinK->setVisible(false);

        ui->rInicioRet->setVisible(false);
        ui->rFinRet->setVisible(false);
        ui->label_4->setVisible(false);
        ui->label_5->setVisible(false);
        ui->label_6->setVisible(false);
    }

    formarango();

    return true;
}

void IntIncertidumbre::formarango(){


    numeradorNombre = tabla->at(0);
    denominadorNombre = tabla->at(1);

    if (rango == true){
        delete layoutnume;
        delete layoutdeno;
        for (ParLineEdit * par : *parNume){
            delete par;
        }
        delete parNume;
        for (ParLineEdit * par : *parDeno){
            delete par;
        }
        delete parDeno;
        qDeleteAll(*numerador);
        delete numerador;
        qDeleteAll(*denominador);
        delete denominador;
        for (qint32 i = 0; i < cajas->size(); i++) {
            delete cajas->at(i);
        }
        delete cajas;
    }

    parNume = new std::list <ParLineEdit*> ();
    parDeno = new std::list <ParLineEdit*> ();

    layoutnume=new QVBoxLayout(ui->boxNume);
    layoutdeno=new QVBoxLayout(ui->boxDeno);

    cajas = new QVector <QWidget *> ();

    this->numerador = new QVector <Parameter*> ();
    this->numerador->reserve(numerador->size());
    this->denominador = new QVector <Parameter*> ();
    this->denominador->reserve(denominador->size());

    rango = true;

    QVector <QString> * nombres = new QVector <QString> ();

    qint32 i = 0;

    foreach (const QString &valor, *numeradorNombre){
        if(isVar->at(0)->at(i)){
            if (!nombres->contains(valor)){
                QWidget * widget = new QWidget(ui->boxNume);
                formarLinea(widget, valor, parNume, controlador);
                layoutnume->addWidget(widget);
                cajas->append(widget);
                nombres->append(valor);
            }
        }
        i++;
    }

    i = 0;
    foreach (const QString &valor, *denominadorNombre){
        if(isVar->at(1)->at(i)){
            if (!nombres->contains(valor)){
                QWidget * widget = new QWidget(ui->boxDeno);
                formarLinea(widget, valor, parDeno, controlador);
                layoutdeno->addWidget(widget);
                cajas->append(widget);
                nombres->append(valor);
            }
        }
        i++;
    }

    delete nombres;

    ui->areaScrolldeno->setAutoFillBackground(true);
    ui->areaScrollnume->setAutoFillBackground(true);
    ui->areaScrolldeno->setLayout(layoutdeno);
    ui->areaScrollnume->setLayout(layoutnume);
}

void IntIncertidumbre:: formarLinea(QWidget *widget, QString variable, std::list <ParLineEdit*> * vector, bool rango){

    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *inicio;
    QLabel *label_2;
    QLineEdit *fin;
    QLabel *label_3;
    QLineEdit *nominal;


    widget->setObjectName(QString::fromUtf8("widget"));
    widget->setGeometry(QRect(20, 30, 221, 25));

    horizontalLayout = new QHBoxLayout(widget);
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    label = new QLabel(widget);
    label->setObjectName(QString::fromUtf8("label"));

    horizontalLayout->addWidget(label);

    inicio = new QLineEdit(widget);
    inicio->setObjectName(QString::fromUtf8("inicio"));
    horizontalLayout->addWidget(inicio);

    label_2 = new QLabel(widget);
    label_2->setObjectName(QString::fromUtf8("label_2"));

    horizontalLayout->addWidget(label_2);

    fin = new QLineEdit(widget);
    fin->setObjectName(QString::fromUtf8("fin"));
    horizontalLayout->addWidget(fin);

    label_3 = new QLabel(widget);
    label_3->setObjectName(QString::fromUtf8("label_3"));

    horizontalLayout->addWidget(label_3);


    nominal = new QLineEdit(widget);
    nominal->setObjectName(QString::fromUtf8("nominal"));
    horizontalLayout->addWidget(nominal);

    if (rango){
        nominal->setVisible(false);
    }

    inicio->raise();

    QString cad = variable + ": [";

    label->setText(QApplication::tr("%1").arg(cad));
    label_2->setText(QApplication::translate("IntIncertidumbre", ",", 0));

    if (!controlador){
        label_3->setText(QApplication::translate("IntIncertidumbre", "] Nominal:", 0));
    } else {
        label_3->setText(QApplication::translate("IntIncertidumbre", "]", 0));
    }

    ParLineEdit * par = new ParLineEdit(inicio, fin, nominal);
    vector->push_back(par);

    //cajas->append(horizontalLayout);
}

void IntIncertidumbre::on_numerador_clicked()
{
    ui->boxDeno->setVisible(false);
    ui->numdeno->setCurrentIndex(0);
    ui->boxNume->setVisible(true);
}

void IntIncertidumbre::on_denominador_clicked()
{
    ui->boxNume->setVisible(false);
    ui->numdeno->setCurrentIndex(1);
    ui->boxDeno->setVisible(true);
}

QVector <Parameter*> * IntIncertidumbre::numerator(){
    return numerador;
}

QVector<Parameter*> *IntIncertidumbre::denominator(){
    return denominador;
}

QPointF IntIncertidumbre::gain(){

    mup::ParserX p;

    if (ui->rInicioK->text().isEmpty() || ui->rFinK->text().isEmpty()){
        ui->rInicioK->setText("1");
        ui->rFinK->setText("1");
    }

    p.SetExpr(ui->rInicioK->text().toStdString());

    QPointF rango;

    rango.setX(p.Eval().GetFloat());

    p.SetExpr(ui->rFinK->text().toStdString());

    rango.setY(p.Eval().GetFloat());

    return rango;
}

QPointF IntIncertidumbre::delay(){

    mup::ParserX p;

    if (ui->rInicioRet->text().isEmpty() || ui->rFinRet->text().isEmpty()){
        ui->rInicioRet->setText("0");
        ui->rFinRet->setText("0");
    }

    p.SetExpr(ui->rInicioRet->text().toStdString());

    QPointF rango;

    rango.setX(p.Eval().GetFloat());

    p.SetExpr(ui->rFinRet->text().toStdString());

    rango.setY(p.Eval().GetFloat());

    return rango;
}

qreal IntIncertidumbre::parse(QString cadena)
{
    p.SetExpr(cadena.toStdString());

    return p.Eval().GetFloat();
}

bool IntIncertidumbre::guardarrango(){

    //Reintento idempotente: tras un error parcial, la ejecucion anterior
    //dejaba parametros ya insertados y el siguiente Aceptar los DUPLICABA.
    qDeleteAll(*numerador);
    numerador->clear();
    qDeleteAll(*denominador);
    denominador->clear();

    QLineEdit * rangoX;
    QLineEdit * rangoY;
    QLineEdit * nominal;

    qreal rangoX_real = 0;
    qreal rangoY_real = 0;
    qreal nominal_real = 0;

    bool validoGlobal = true;
    bool valido = true;

    QVector <QString> * nombres = new QVector <QString> ();

    for (qint32 i = 0; i < numeradorNombre->size(); i++){
        Parameter * variable = NULL;
        valido = true;
        if(isVar->at(0)->at(i)){
            if (!nombres->contains(numeradorNombre->at(i))){

                ParLineEdit * aux = parNume->front();

                rangoX = aux->getX();
                rangoY = aux->getY();
                nominal= aux->nominal();
                if (controlador){
                    nominal->setText(QString::number((rangoX->text().toDouble() + rangoY->text().toDouble()) / 2));
                }

                if (rangoX->text().isEmpty() || rangoY->text().isEmpty() || nominal->text().isEmpty()){
                    rangoX->setStyleSheet("background : red");
                    rangoY->setStyleSheet("background : red");
                    nominal->setStyleSheet("background : red");
                    valido = false;
                }else{
                    try {
                        rangoX_real = parse(rangoX->text());
                        rangoY_real = parse(rangoY->text());
                        nominal_real = parse(nominal->text());
                    } catch (mup::ParserError &) {
                        //Expresion invalida: antes reventaba el dialogo.
                        rangoX->setStyleSheet("background : red");
                        rangoY->setStyleSheet("background : red");
                        nominal->setStyleSheet("background : red");
                        valido = false;
                        rangoX_real = 1;
                        rangoY_real = 0;
                        nominal_real = 0;
                    }

                    if (valido && (rangoX_real <= nominal_real) && (nominal_real <= rangoY_real)){
                        QPointF rango (rangoX_real, rangoY_real);
                        variable = new Parameter (numeradorNombre->at(i), rango, nominal_real, exp->at(0)->at(i));

                        rangoX->setStyleSheet("background : white");
                        rangoY->setStyleSheet("background : white");
                        nominal->setStyleSheet("background : white");

                        parNume->pop_front();
                        delete aux;
                        valido = true;
                    } else {
                        rangoX->setStyleSheet("background : red");
                        rangoY->setStyleSheet("background : red");
                        nominal->setStyleSheet("background : red");
                        valido = false;
                    }
                }
            } else{
                for (qint32 x = 0; x < numerador->size(); x++){
                    if (numerador->at(x)->name() == numeradorNombre->at(i)){
                        Parameter * v = numerador->at(x);
                        variable = new Parameter(v->name(),v->rawRange(),v->rawNominal(),v->expression());
                        break;
                    }
                }
            }
        }else {
            variable = new Parameter (numeradorNombre->at(i).toDouble());
        }

        if (valido && variable != NULL){
            numerador->insert(i,variable);
            nombres->append(numeradorNombre->at(i));
        }else{
            validoGlobal = false;
        }
    }

    if (!validoGlobal){
        delete nombres;
        menerror("Hay errores en los rango de las variables", "Introducir incertidumbre");
        return false;
    }

    validoGlobal = true;

    for (qint32 i = 0; i < denominadorNombre->size(); i++){
        Parameter * variable = NULL;
        if(isVar->at(1)->at(i)){
            if (!nombres->contains(denominadorNombre->at(i))){

                ParLineEdit * aux = parDeno->front();

                rangoX = aux->getX();
                rangoY = aux->getY();
                nominal = aux->nominal();

                if (controlador){
                    nominal->setText(QString::number((rangoX->text().toDouble() + rangoY->text().toDouble()) / 2));
                }

                if (rangoX->text().isEmpty() || rangoY->text().isEmpty() || nominal->text().isEmpty()){
                    valido = false;
                    rangoX->setStyleSheet("background : red");
                    rangoY->setStyleSheet("background : red");
                    nominal->setStyleSheet("background : red");
                }else{
                    try {
                        rangoX_real = parse(rangoX->text());
                        rangoY_real = parse(rangoY->text());
                        nominal_real = parse(nominal->text());
                    } catch (mup::ParserError &) {
                        rangoX->setStyleSheet("background : red");
                        rangoY->setStyleSheet("background : red");
                        nominal->setStyleSheet("background : red");
                        valido = false;
                    }

                    if (valido){
                        if ((rangoX_real <= nominal_real) && (nominal_real <= rangoY_real)){
                            QPointF rango (rangoX_real, rangoY_real);
                            variable = new Parameter (denominadorNombre->at(i), rango, nominal_real, exp->at(1)->at(i));

                            rangoX->setStyleSheet("background : white");
                            rangoY->setStyleSheet("background : white");
                            nominal->setStyleSheet("background : white");

                            parDeno->pop_front();
                            delete aux;
                        } else {
                            valido = false;
                            rangoX->setStyleSheet("background : red");
                            rangoY->setStyleSheet("background : red");
                            nominal->setStyleSheet("background : red");
                        }
                    }
                }
            } else{
                bool elegido = false;
                for (qint32 x = 0; x < numerador->size(); x++){
                    if (numerador->at(x)->name() == denominadorNombre->at(i)){
                        Parameter * v = numerador->at(x);
                        variable = new Parameter(v->name(),v->rawRange(),v->rawNominal(),v->expression());
                        break;
                    }
                }

                if (!elegido){
                    for (qint32 x = 0; x < denominador->size(); x++){
                        if (denominador->at(x)->name() == denominadorNombre->at(i)){
                            Parameter * v = denominador->at(x);
                            variable = new Parameter(v->name(),v->rawRange(),v->rawNominal(),v->expression());
                            break;
                        }
                    }
                }
            }
        }else {
            variable = new Parameter (denominadorNombre->at(i).toDouble());
        }

        if (valido && variable != NULL){
            denominador->insert(i,variable);
            nombres->append(denominadorNombre->at(i));
        }else{
            validoGlobal = false;
        }
    }

    delete nombres;

    if (!validoGlobal){
        menerror("Hay errores en los rango de las variables", "Introducir incertidumbre");
        return false;
    }

    return true;
}

void IntIncertidumbre::on_aceptar_clicked()
{
    if (ui->elegirforma->currentIndex() == 0){
        if (guardarrango()){
            aceptado = true;
            this->close();
        }
    }
}

bool IntIncertidumbre::getTodoCorrecto(){
    return aceptado;
}
