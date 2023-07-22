#include "diagramabode.h"
#include "ui_diagramabode.h"


using namespace std;
using namespace tools;

DiagramaBode::DiagramaBode(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DiagramaBode)
{
    ui->setupUi(this);
    setWindowTitle("Diagrama de Bode");
}

DiagramaBode::~DiagramaBode()
{
    delete ui;
}

void DiagramaBode::dibujarBode(Sistema *planta, Omega *omega){

    QVector <qreal> * frecuencias;

    if (omega->getTipo() == linSpace){
        frecuencias = linspace(-1, omega->getFinal(),100);
    }else if (omega->getTipo() == logSpace){
        frecuencias = logspace(-1, omega->getFinal(),100);
    }else {
        frecuencias = omega->getValores();
    }

    QVector <qreal> * ganancia = new QVector <qreal> ();
    QVector <qreal> * fase = new QVector <qreal> ();
    ganancia->reserve(frecuencias->size());
    fase->reserve(frecuencias->size());

    QVector <std::complex<qreal> > * complejos = planta->getPunto(frecuencias);


   foreach (const std::complex<qreal> &comp, *complejos){
        ganancia->append(20*log10(abs(comp)));
        fase->append(arg(comp));
    }

    dibujarDiagrama("Magnitud",ganancia,frecuencias,ui->customPlot);
    dibujarDiagrama("Fase" ,fase,frecuencias,ui->customPlot_2);
    this->setWindowTitle("Diagrama de Bode");
    ui->customPlot->replot();
    ui->customPlot_2->replot();

    ganancia->clear();
    fase->clear();
}


void DiagramaBode::dibujarDiagrama(QString nombreEjeY, QVector<qreal> * ejeY, QVector<qreal> * frecuencias, QCustomPlot * customPlot){

    QCPCurve *curva = new QCPCurve(customPlot->xAxis, customPlot->yAxis);
    curva->setData(*frecuencias, *ejeY);

    customPlot->xAxis->setLabel("w");
    customPlot->yAxis->setLabel(nombreEjeY);

    customPlot->xAxis->setScaleType(QCPAxis::ScaleType::stLogarithmic);
    customPlot->xAxis->setRange(frecuencias->first(), frecuencias->last());
    customPlot->yAxis->setRange(ejeY->first(), ejeY->last());
}

void DiagramaBode::on_actionExportar_triggered()
{
    bool noFallo = true;
    QString * extension = new QString();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Guardar Fichero"),"",
      tr((".png (*.png);;.pdf(*.pdf);; .jpg(*.jpg);; .bmp(*.bmp)")), extension);
    if (!fileName.isEmpty()){
        if (extension->contains(".pdf", Qt::CaseInsensitive)){
            noFallo = ui->customPlot->savePdf("0-" + fileName, true);
            noFallo = ui->customPlot_2->savePdf("1-" + fileName, true);
        }else if (extension->contains(".png", Qt::CaseInsensitive)){
            noFallo = ui->customPlot->savePng("0-" + fileName);
            noFallo = ui->customPlot_2->savePng("1-" + fileName);
        }else if (extension->contains(".jpg", Qt::CaseInsensitive)){
            noFallo = ui->customPlot->saveJpg("0-" + fileName);
            noFallo = ui->customPlot_2->saveJpg("1-" + fileName);
        }else if (extension->contains(".bmp", Qt::CaseInsensitive)){
            noFallo = ui->customPlot->saveBmp("0-" + fileName);
            noFallo = ui->customPlot_2->saveBmp("1-" + fileName);
        }else{
            noFallo = false;
        }

        if (!noFallo)
            menerror("No se ha podido guardar la imagen", "Grafico Template");
    }
    delete extension;
}
