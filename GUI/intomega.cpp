#include "intomega.h"
#include "ui_intomega.h"

using namespace tools;


IntOmega::IntOmega(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IntOmega)
{

    fallo = false;
    ui->setupUi(this);
    setWindowTitle("Introducir Omega");

    //Los lineedit para las funciones linspace y logspace solo admiten números reales.
    ui->logfin->setValidator(new QDoubleValidator(this));
    ui->loginicio->setValidator(new QDoubleValidator(this));
    ui->logn->setValidator(new QDoubleValidator(this));

    ui->linfin->setValidator(new QDoubleValidator(this));
    ui->lininicio->setValidator(new QDoubleValidator(this));
    ui->linn->setValidator(new QDoubleValidator(this));

    todoCorrecto = false;

    connect(ui->cancel, SIGNAL(clicked()), this, SLOT(close()));
    connect (this, SIGNAL(close_ok()), this,SLOT(close()));
}

IntOmega::IntOmega(Controlador *controlador, QWidget *parent):IntOmega(parent){
    this->controlador = controlador;
}

IntOmega::~IntOmega()
{
    delete ui;
}
void IntOmega::on_buttonFic_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()){
        file=fileName;
        ui->mosFic->setText(file);
    }
}

void IntOmega::on_ok_clicked()
{
    qreal inicio = 0;
    qreal final = 0;
    tiposOmega tipo;
    QVector <qreal> * frecuencias;

    if (ui->selecforma->currentIndex() == 0){ //manual
        frecuencias = srtovectorReal(ui->mavect->text());
        tipo = manual;
        if (frecuencias == NULL){
            ui->mavect->setStyleSheet("background : red");
            fallo = true;
            frecuencias->clear();
        }else{
            ui->mavect->setStyleSheet("background : white");
        }

    } else if (ui->selecforma->currentIndex() == 1) { //logspace
        frecuencias = logspace(ui->loginicio->text().toDouble(),ui->logfin->text().toDouble(),
                               ui->logn->text().toDouble());
        inicio = ui->loginicio->text().toDouble();
        final = ui->logfin->text().toDouble();
        tipo = logSpace;

    }else if (ui->selecforma->currentIndex() == 2) { //linspace

        frecuencias = linspace(ui->lininicio->text().toDouble(),ui->linfin->text().toDouble(),
                               ui->linn->text().toDouble());

        inicio = ui->lininicio->text().toDouble();
        final = ui->lininicio->text().toDouble();
        tipo = linSpace;

    } else {
        frecuencias = fichero(file);
        tipo = tiposOmega::fichero;
    }

    Omega * omega = new Omega(inicio, final, frecuencias->size(),frecuencias,tipo);
    controlador->setOmega(omega);

    todoCorrecto = true;

    emit (close_ok());
}

QVector <qreal> * IntOmega::fichero(QString ruta){
    QFile fichero (ruta);
    QTextStream in (&fichero);

    if (fichero.open(QIODevice::ReadOnly))
        menerror("No se puede leer el fichero, hay un error","Introducir Omega");

    return srtovectorReal(in.readAll());

}

bool IntOmega::getTodoCorrecto(){
    return todoCorrecto;
}
