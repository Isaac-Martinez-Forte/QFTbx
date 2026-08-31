#include "boundary_grid_dialog.h"
#include "ui_boundary_grid_dialog.h"

#include "GUI/error_message.h"


using namespace tools;
using namespace std;

BoundaryGridDialog::BoundaryGridDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BoundaryGridDialog)
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

    setWindowTitle(tr("Boundary grid input"));

#ifndef CUDA_AVAILABLE
    ui->cuda->setVisible(false);
#endif

    todoCorrecto = false;
}

BoundaryGridDialog::~BoundaryGridDialog()
{
    delete ui;
}

QPointF BoundaryGridDialog::getDatosFas(){
    return datosFase;
}

QPointF BoundaryGridDialog::getDatosMag(){
    return datosMag;
}

qint32 BoundaryGridDialog::getPuntosFas(){
    return nPuntosFas;
}

qint32 BoundaryGridDialog::getPuntosMag(){
    return nPuntosMag;
}

qreal BoundaryGridDialog::getInfinito(){
    return infinito;
}

bool BoundaryGridDialog::isContornoSelect(){

    if (ui->template_2->isChecked()){
        return false;
    }

    return true;
}

void BoundaryGridDialog::on_buttonBox_accepted()
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
        tools::errorMessage(tr("The grid ranges must be increasing, with at least 2 points per axis."), tr("Boundary grid input"));
        todoCorrecto = false;
        return;
    }

    realizado = true;

    //Lectura directa: el latch anterior dejaba CUDA activado para siempre
    //tras marcarlo una vez.
    cuda = ui->cuda->isChecked();

    todoCorrecto = true;
}

void BoundaryGridDialog::showEvent(QShowEvent * event)
{
    //Reabrir y cancelar no debe relanzar el calculo con los datos antiguos.
    todoCorrecto = false;
    QDialog::showEvent(event);
}

bool BoundaryGridDialog::getCUDA(){
    return cuda;
}

bool BoundaryGridDialog::getTodoCorrecto(){
    return todoCorrecto;
}
