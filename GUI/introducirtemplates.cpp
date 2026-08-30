#include "introducirtemplates.h"
#include "ui_introducirtemplates.h"

#include "GUI/menerror.h"
#include "GUI/plot_palette.h"

using namespace tools;
using namespace mup;

IntroducirTemplates::IntroducirTemplates(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IntroducirTemplates)
{
    ui->setupUi(this);
    ui->todasNPuntos->setValidator(new QDoubleValidator(this));

    setWindowTitle("Introducir Datos Template");

    ui->todasNPuntos->setText("10");

    //conectamos el boton cancelar
    connect(ui->cancelar, SIGNAL(clicked()), this, SLOT(close()));
    connect (this, SIGNAL(close_ok()), this,SLOT(close()));

#ifndef CUDA_AVAILABLE
    ui->CUDA->setVisible(false);
#endif

    parser =  new ParserX (pckALL_NON_COMPLEX);

    todoCorrecto = false;
}

IntroducirTemplates::~IntroducirTemplates()
{
    delete ui;
    if (variablesCreadas){
        parNume->clear();
        parDeno->clear();
        radioButtonsNume->clear();
        radioButtonsDeno->clear();
    }
}

QVector<qreal> * IntroducirTemplates::getEpsilon(){
    return epsilon;
}

void IntroducirTemplates::lanzarViewTemp(LtiSystem *planta, qint32 numOmegas){

    this->planta = planta;
    this->numOmegas = numOmegas;


    formartablas(planta->numerator(), planta->denominator());
}

void IntroducirTemplates::formartablas(QVector<Parameter *> *numerador, QVector<Parameter *> *denominador){

    this->numerador = numerador;
    this->denominador = denominador;

    if (variablesCreadas){
        parNume->clear();
        parDeno->clear();
        radioButtonsNume->clear();
        radioButtonsDeno->clear();
    }
    parNume = new QVector <ParLineEdit*> ();
    parDeno = new QVector <ParLineEdit*> ();
    radioButtonsNume = new QVector <tresRadioButton> ();
    radioButtonsDeno = new QVector <tresRadioButton> ();

    variablesCreadas = true;


    qint32 indice = 0;
    foreach (Parameter * variable, *numerador){
        QString valor = variable->name();
        if(variable->isUncertain()){
            QWidget * widget = new QWidget(ui->agrNuDe);
            crearWidget(widget,parNume, radioButtonsNume);
            ui->tabNume->insertTab(indice,widget,valor);
            indice++;
        }
        ui->tabNume->removeTab(indice);
        ui->tabNume->removeTab(indice+1);
    }
    indice = 0;
    foreach (Parameter * variable, *denominador){
        QString valor = variable->name();
        if(variable->isUncertain()){
            QWidget * widget = new QWidget(ui->agrNuDe);
            crearWidget(widget, parDeno, radioButtonsDeno);
            ui->tabDeno->insertTab(indice,widget, valor);
            indice++;
        }
        ui->tabDeno->removeTab(indice);
        ui->tabDeno->removeTab(indice+1);
    }
}

void IntroducirTemplates::crearWidget(QWidget *widget, QVector <ParLineEdit*> * par, QVector <tresRadioButton> * radioButtons){

    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QRadioButton *rLin;
    QLineEdit *lin;
    QHBoxLayout *horizontalLayout_2;
    QRadioButton *rLog;
    QLineEdit *log;
    QHBoxLayout *horizontalLayout_3;
    QRadioButton *rManual;
    QLineEdit *manual;

    widget->setObjectName(QString::fromUtf8("widget"));
    widget->setGeometry(QRect(10, 10, 141, 86));
    verticalLayout = new QVBoxLayout(widget);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));

    rLin = new QRadioButton(widget);
    rLin->setObjectName(QString::fromUtf8("rLin"));

    horizontalLayout->addWidget(rLin);

    lin = new QLineEdit(widget);
    lin->setValidator(new QDoubleValidator(widget));
    lin->setObjectName(QString::fromUtf8("lin"));

    horizontalLayout->addWidget(lin);


    verticalLayout->addLayout(horizontalLayout);

    horizontalLayout_2 = new QHBoxLayout();
    horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
    rLog = new QRadioButton(widget);
    rLog->setObjectName(QString::fromUtf8("rLog"));

    horizontalLayout_2->addWidget(rLog);

    log = new QLineEdit(widget);
    log->setValidator(new QDoubleValidator(widget));
    log->setObjectName(QString::fromUtf8("log"));

    horizontalLayout_2->addWidget(log);


    verticalLayout->addLayout(horizontalLayout_2);

    horizontalLayout_3 = new QHBoxLayout();
    horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
    rManual = new QRadioButton(widget);
    rManual->setObjectName(QString::fromUtf8("rManual"));

    horizontalLayout_3->addWidget(rManual);

    manual = new QLineEdit(widget);
    manual->setObjectName(QString::fromUtf8("manual"));

    horizontalLayout_3->addWidget(manual);


    verticalLayout->addLayout(horizontalLayout_3);

    rLin->setText(QApplication::translate("Template", "LinSpace", 0));
    rLog->setText(QApplication::translate("Template", "LogSpace", 0));
    rManual->setText(QApplication::translate("Template", "Manual", 0));

    par->append(new ParLineEdit(lin,log,manual));

    struct tresRadioButton radio;
    radio.uno = rLin;
    radio.dos = rLog;
    radio.tres = rManual;

    radioButtons->append(radio);

}

void IntroducirTemplates::on_Todas_clicked()
{
    ui->agrupador->setCurrentIndex(0);
}

void IntroducirTemplates::on_unaxuna_clicked()
{
    ui->agrupador->setCurrentIndex(2);
}

void IntroducirTemplates::on_radioNumerador_clicked()
{
    ui->agrNuDe->setCurrentIndex(1);
}

void IntroducirTemplates::on_radioDenominador_clicked()
{
    ui->agrNuDe->setCurrentIndex(2);
}

void IntroducirTemplates::on_cancelar_clicked()
{
    emit (close_ok());
}
void IntroducirTemplates::on_Aceptar_clicked()
{
    if (ui->nyquist->isChecked())
        diagrama = false;
    else if (ui->nicols->isChecked())
        diagrama = true;

    if (ui->CUDA->isChecked()){
        cuda = true;
    }

    epsilon = new QVector <qreal> ();

    if (ui->epsilon->text().isEmpty()){
        menerror("No ha introducido un valor para epsilon", "Calculo de Templates");
        ui->epsilon->setStyleSheet("background : red");
        return;
    }else {

        ui->epsilon->setStyleSheet("background : white");
        QVector <QString> * v = tools::srtovectorString(ui->epsilon->text());

        qreal ultimaEpsilon = 0;

        qint32 contador = 0;

        foreach (QString s, *v) {
            parser->SetExpr(s.toStdString());
            ultimaEpsilon = parser->Eval().GetFloat();
            epsilon->append(ultimaEpsilon);
            contador++;
        }

        for (; contador < numOmegas; contador++){
            epsilon->append(ultimaEpsilon);
        }
    }

    bool linsp = false;
    bool logsp = false;

    if (mapa != NULL){
        mapa->clear();
    }

    mapa = new QHash <QString, QVector<qreal> * > ();

    if (ui->seleLinSpace->isChecked() && !ui->todasNPuntos->text().isEmpty()){
        linsp = true;
    }else if (ui->selecLogSpace->isChecked() && !ui->todasNPuntos->text().isEmpty()){
        logsp = true;
    }else {
        menerror("ERROR: Tiene señalar logspace o linspace en el apartado general.", "Cálculo de Templates");
        return;
    }


    struct tresRadioButton radioButtons;
    ParLineEdit * parlines;
    Parameter* var;
    qint32 contVar = 0;
    for (qint32 i = 0; i < numerador->size(); i++){
        var = numerador->at(i);
        if (var->isUncertain()){
            parlines = parNume->at(contVar);
            radioButtons = radioButtonsNume->at(contVar);
            contVar++;
            if (!extraerVariable(parlines, radioButtons,var,linsp,logsp)){
                menerror("ERROR: Los valores introducidos para la variable \"" + var->name()
                         + "\" son incorrectos", "Cálculo de Templates");
                return;
            }
        }/*else {
            QVector <qreal> * vector = new QVector <qreal> ();
            vector->append(var->nominal());;
            mapa->insert(var->name(), vector);
        }*/
    }

    contVar = 0;

    for (qint32 i = 0; i < denominador->size(); i++){

        var = denominador->at(i);
        if (var->isUncertain()){

            parlines = parDeno->at(contVar);
            radioButtons = radioButtonsDeno->at(contVar);
            contVar++;
            if (!extraerVariable(parlines, radioButtons,var,linsp,logsp)){
                menerror("ERROR: Los valores introducidos para la variable \"" + var->name()
                         + "\" son incorrectos", "Cálculo de Templates");
                return;
            }
        }/*else {
            QVector <qreal> * vector = new QVector <qreal> ();
            vector->append(var->nominal());
            mapa->insert(var->name(), vector);
        }*/
    }

    if (!planta->gain()->isUncertain()){
        mapa->insert(planta->gain()->name(), new QVector <qreal> (1, planta->gain()->nominal()));
    }
    else{

        qreal inicio;
        qreal final;
        qint32 npuntos;

        inicio = planta->gain()->range().x();
        final = planta->gain()->range().y();
        parser->SetExpr(ui->todasNPuntos->text().toStdString());
        npuntos = parser->Eval().GetFloat();

        if (linsp){
            mapa->insert(planta->gain()->name(), linspace(inicio, final, npuntos));
        } else {
            mapa->insert(planta->gain()->name(), logspace(inicio, final, npuntos));
        }
    }

    if (!planta->delay()->isUncertain()){
        mapa->insert(planta->delay()->name(), new QVector <qreal> (1, planta->delay()->nominal()));
    }else {

        qreal inicio;
        qreal final;
        qint32 npuntos;

        inicio = planta->delay()->range().x();
        final = planta->delay()->range().y();
        parser->SetExpr(ui->todasNPuntos->text().toStdString());
        npuntos = parser->Eval().GetFloat();

        //Antes esta rama insertaba la rejilla del retardo bajo la CLAVE de
        //la ganancia: machacaba la de la ganancia y dejaba al retardo sin
        //entrada (crash en el barrido con retardo incierto).
        if (linsp){
            mapa->insert(planta->delay()->name(), linspace(inicio, final, npuntos));
        } else {
            mapa->insert(planta->delay()->name(), logspace(inicio, final, npuntos));
        }
    }

    todoCorrecto = true;
    emit (close_ok());
}

bool IntroducirTemplates::extraerVariable(ParLineEdit *parlines, tresRadioButton radioButtons,
                                    Parameter *var, bool linsp, bool logsp){

    //Politica historica: para nombres repetidos (p.ej. la misma 'a' en
    //numerador y denominador) gana la PRIMERA rejilla introducida. Con la
    //clave por nombre, sin esta guarda ganaria la ultima. Pendiente: avisar
    //al usuario del duplicado (ver REFACTOR_PLAN).
    if (mapa->contains(var->name())){
        return true;
    }

    qreal inicio;
    qreal final;
    qreal npuntos;


    if (radioButtons.uno->isChecked() && !parlines->getX()->text().isEmpty()){

        inicio = var->range().x();
        final = var->range().y();

        parser->SetExpr(parlines->getX()->text().toStdString());
        npuntos = parser->Eval().GetFloat();

        mapa->insert(var->name(), linspace(inicio,final, npuntos));

    }else if (radioButtons.dos->isChecked() && !parlines->getY()->text().isEmpty()){

        inicio = var->range().x();
        final = var->range().y();
        parser->SetExpr(parlines->getY()->text().toStdString());
        npuntos = parser->Eval().GetFloat();

        mapa->insert(var->name(), logspace(inicio, final, npuntos));

    }else if(radioButtons.tres->isChecked() && !parlines->nominal()->text().isEmpty()){

        QVector <QString> * vector = srtovectorString(parlines->nominal()->text());
        QVector <qreal> * vector2 = new QVector <qreal> ();

        foreach (QString numeroS, *vector) {
            parser->SetExpr(numeroS.toStdString());
            vector2->append(parser->Eval().GetFloat());
        }
        mapa->insert(var->name(), vector2);
    }else if (linsp || logsp){

        inicio = var->range().x();
        final = var->range().y();

        parser->SetExpr(ui->todasNPuntos->text().toStdString());
        npuntos = parser->Eval().GetFloat();

        if(linsp){
            mapa->insert(var->name(), linspace(inicio, final, npuntos));
        }else {
            mapa->insert(var->name(), logspace(inicio, final, npuntos));
        }
    }else{
        return false;
    }

    return true;
}

QHash <QString, QVector<qreal> * > * IntroducirTemplates::getMapa(){
    return mapa;
}


bool IntroducirTemplates::getElecDiagram(){
    return diagrama;
}

bool IntroducirTemplates::getElecCUDA(){
    return cuda;
}

bool IntroducirTemplates::getTodoCorrecto(){
    return todoCorrecto;
}
