#include "intdatosboundaries.h"
#include "ui_intdatosboundaries.h"

#include "GUI/menerror.h"


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

    datosFase = QPointF(ui->fasInit->text().toDouble(),ui->fasFin->text().toDouble());
    datosMag = QPointF(ui->magInit->text().toDouble(),ui->magFin->text().toDouble());

    nPuntosFas = ui->fasPuntos->text().toInt();
    nPuntosMag = ui->magPuntos->text().toInt();

    //La rejilla debe tener sentido antes de lanzar el calculo: rangos
    //crecientes y al menos dos puntos por eje (antes cualquier valor pasaba
    //directo al motor).
    if (datosFase.x() >= datosFase.y() || datosMag.x() >= datosMag.y() ||
            nPuntosFas < 2 || nPuntosMag < 2){
        tools::menerror("Los rangos de la rejilla deben ser crecientes y con al menos 2 puntos por eje.",
                        "Introducir Datos Boundaries");
        todoCorrecto = false;
        return;
    }

    realizado = true;

    //Lectura directa: el latch anterior dejaba CUDA activado para siempre
    //tras marcarlo una vez.
    cuda = ui->cuda->isChecked();

    todoCorrecto = true;
}

void IntDatosBoundaries::showEvent(QShowEvent * event)
{
    //Reabrir y cancelar no debe relanzar el calculo con los datos antiguos.
    todoCorrecto = false;
    QDialog::showEvent(event);
}

bool IntDatosBoundaries::getCUDA(){
    return cuda;
}

bool IntDatosBoundaries::getTodoCorrecto(){
    return todoCorrecto;
}
