#include "frequencies_dialog.h"
#include "ui_frequencies_dialog.h"

#include <QMessageBox>

#include "Modelo/Herramientas/exception.h"

using namespace tools;


FrequenciesDialog::FrequenciesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FrequenciesDialog)
{

    ui->setupUi(this);
    setWindowTitle(tr("Design frequencies input"));

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

FrequenciesDialog::FrequenciesDialog(Controlador *controlador, QWidget *parent):FrequenciesDialog(parent){
    this->controlador = controlador;
}

FrequenciesDialog::~FrequenciesDialog()
{
    delete ui;
}
void FrequenciesDialog::on_buttonFic_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()){
        file=fileName;
        ui->mosFic->setText(file);
    }
}

void FrequenciesDialog::on_ok_clicked()
{
    qreal inicio = 0;
    qreal final = 0;
    Omega::tiposOmega tipo;
    QVector <qreal> * frecuencias;

    if (ui->selecforma->currentIndex() == 0){ //manual
        frecuencias = srtovectorReal(ui->mavect->text());
        tipo = Omega::manual;
        if (frecuencias == NULL){
            //Entrada invalida: antes se seguia adelante y se desreferenciaba
            //el puntero nulo unas lineas mas abajo.
            ui->mavect->setStyleSheet("background : red");
            return;
        }
        ui->mavect->setStyleSheet("background : white");

    } else if (ui->selecforma->currentIndex() == 1) { //logspace
        frecuencias = logspace(ui->loginicio->text().toDouble(),ui->logfin->text().toDouble(),
                               ui->logn->text().toDouble());
        inicio = ui->loginicio->text().toDouble();
        final = ui->logfin->text().toDouble();
        tipo = Omega::logSpace;

    }else if (ui->selecforma->currentIndex() == 2) { //linspace

        frecuencias = linspace(ui->lininicio->text().toDouble(),ui->linfin->text().toDouble(),
                               ui->linn->text().toDouble());

        inicio = ui->lininicio->text().toDouble();
        //Antes se releia lininicio: todo Omega lineal se guardaba con
        //final == inicio (y asi viajaba al .qft y al diagrama de Bode).
        final = ui->linfin->text().toDouble();
        tipo = Omega::linSpace;

    } else {
        try {
            frecuencias = Omega::valuesFromFile(file);
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Design frequencies input"), e.what());
            return;
        }
        tipo = Omega::fichero;
    }

    Omega * omega = new Omega(inicio, final, frecuencias->size(),frecuencias,tipo);
    controlador->setOmega(omega);

    todoCorrecto = true;

    emit (close_ok());
}


bool FrequenciesDialog::getTodoCorrecto(){
    return todoCorrecto;
}
