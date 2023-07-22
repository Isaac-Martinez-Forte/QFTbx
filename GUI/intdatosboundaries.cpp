#include "intdatosboundaries.h"
#include "ui_intdatosboundaries.h"


using namespace tools;
using namespace std;

IntDatosBoundaries::IntDatosBoundaries(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IntDatosBoundaries)
{
    ui->setupUi(this);

    realizado = false;

    ui->fasInit->setValidator(new QDoubleValidator(this));
    ui->fasFin->setValidator(new QDoubleValidator(this));
    ui->magInit->setValidator(new QDoubleValidator(this));
    ui->magFin->setValidator(new QDoubleValidator(this));

    ui->fasPuntos->setValidator(new QIntValidator(this));
    ui->magPuntos->setValidator(new QIntValidator(this));

    ui->infinito->setValidator(new QDoubleValidator(this));

    ui->fasInit->setText("-360");
    ui->fasFin->setText("0");
    ui->fasPuntos->setText("361");

    ui->magInit->setText("-60");
    ui->magFin->setText("60");
    ui->magPuntos->setText("121");

    cuda = false;

    setWindowTitle("Introducir Datos Boundaries");

#ifndef CUDA_AVAILABLE
    ui->cuda->setVisible(false);
#endif

    todoCorrecto = false;
}

IntDatosBoundaries::~IntDatosBoundaries()
{
    delete ui;
}

QPointF IntDatosBoundaries::getDatosFas(){
    return datosFase;
}

QPointF IntDatosBoundaries::getDatosMag(){
    return datosMag;
}

qint32 IntDatosBoundaries::getPuntosFas(){
    return nPuntosFas;
}

qint32 IntDatosBoundaries::getPuntosMag(){
    return nPuntosMag;
}

qreal IntDatosBoundaries::getInfinito(){
    return infinito;
}

bool IntDatosBoundaries::isContornoSelect(){

    if (ui->template_2->isChecked()){
        return false;
    }

    return true;
}

void IntDatosBoundaries::on_buttonBox_accepted()
{
    if (ui->infinito->text().isEmpty()){
        infinito = -1;
    }else{
        infinito = ui->infinito->text().toDouble();
    }


    realizado = true;

    datosFase = QPointF(ui->fasInit->text().toDouble(),ui->fasFin->text().toDouble());
    datosMag = QPointF(ui->magInit->text().toDouble(),ui->magFin->text().toDouble());

    nPuntosFas = ui->fasPuntos->text().toInt();
    nPuntosMag = ui->magPuntos->text().toInt();


    if(ui->cuda->isChecked()){
        cuda = true;
    }

    todoCorrecto = true;
}

bool IntDatosBoundaries::getCUDA(){
    return cuda;
}

bool IntDatosBoundaries::getTodoCorrecto(){
    return todoCorrecto;
}
