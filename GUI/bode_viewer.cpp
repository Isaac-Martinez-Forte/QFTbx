#include "bode_viewer.h"
#include "ui_bode_viewer.h"

#include "GUI/error_message.h"

#include <QFileInfo>
#include "GUI/plot_palette.h"


using namespace std;
using namespace tools;

BodeViewer::BodeViewer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BodeViewer)
{
    ui->setupUi(this);
    setWindowTitle(tr("Bode diagram"));
}

BodeViewer::~BodeViewer()
{
    delete ui;
}

void BodeViewer::dibujarBode(LtiSystem *planta, Omega *omega){

    //Redibujable: sin esto cada llamada apilaba curvas nuevas.
    ui->customPlot->clearPlottables();
    ui->customPlot_2->clearPlottables();

    QVector <qreal> * frecuencias;
    bool frecuenciasPropias = true;

    if (omega->getTipo() == Omega::linSpace){
        frecuencias = linspace(-1, omega->getFinal(),100);
    }else if (omega->getTipo() == Omega::logSpace){
        frecuencias = logspace(-1, omega->getFinal(),100);
    }else {
        //Vector del DAO: no se libera aqui.
        frecuencias = omega->getValores();
        frecuenciasPropias = false;
    }

    QVector <qreal> * ganancia = new QVector <qreal> ();
    QVector <qreal> * fase = new QVector <qreal> ();
    ganancia->reserve(frecuencias->size());
    fase->reserve(frecuencias->size());

    QVector <std::complex<qreal> > * complejos = planta->evaluate(frecuencias);


   foreach (const std::complex<qreal> &comp, *complejos){
        ganancia->append(20*log10(abs(comp)));
        fase->append(arg(comp));
    }

    dibujarDiagrama("Magnitud",ganancia,frecuencias,ui->customPlot);
    dibujarDiagrama("Fase" ,fase,frecuencias,ui->customPlot_2);
    this->setWindowTitle(tr("Bode diagram"));
    ui->customPlot->replot();
    ui->customPlot_2->replot();

    delete complejos;
    delete ganancia;
    delete fase;
    if (frecuenciasPropias){
        delete frecuencias;
    }
}


void BodeViewer::dibujarDiagrama(QString nombreEjeY, QVector<qreal> * ejeY, QVector<qreal> * frecuencias, QCustomPlot * customPlot){

    QCPCurve *curva = new QCPCurve(customPlot->xAxis, customPlot->yAxis);
    curva->setData(*frecuencias, *ejeY);

    customPlot->xAxis->setLabel("w");
    customPlot->yAxis->setLabel(nombreEjeY);

    customPlot->xAxis->setScaleType(QCPAxis::ScaleType::stLogarithmic);
    customPlot->xAxis->setRange(frecuencias->first(), frecuencias->last());
    customPlot->yAxis->setRange(ejeY->first(), ejeY->last());
}

void BodeViewer::on_actionExportar_triggered()
{
    bool noFallo = true;
    QString extension;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save file"),"",
      tr((".png (*.png);;.pdf(*.pdf);; .jpg(*.jpg);; .bmp(*.bmp)")), &extension);
    if (!fileName.isEmpty()){
        //Prefijar la RUTA completa ("0-/home/...") generaba rutas invalidas:
        //el sufijo va en el nombre del fichero.
        QFileInfo info (fileName);
        const QString magnitud = info.dir().filePath(info.completeBaseName() + "-mag." + info.suffix());
        const QString faseNombre = info.dir().filePath(info.completeBaseName() + "-fase." + info.suffix());

        if (extension.contains(".pdf", Qt::CaseInsensitive)){
            noFallo = ui->customPlot->savePdf(magnitud, true);
            noFallo = ui->customPlot_2->savePdf(faseNombre, true) && noFallo;
        }else if (extension.contains(".png", Qt::CaseInsensitive)){
            noFallo = ui->customPlot->savePng(magnitud);
            noFallo = ui->customPlot_2->savePng(faseNombre) && noFallo;
        }else if (extension.contains(".jpg", Qt::CaseInsensitive)){
            noFallo = ui->customPlot->saveJpg(magnitud);
            noFallo = ui->customPlot_2->saveJpg(faseNombre) && noFallo;
        }else if (extension.contains(".bmp", Qt::CaseInsensitive)){
            noFallo = ui->customPlot->saveBmp(magnitud);
            noFallo = ui->customPlot_2->saveBmp(faseNombre) && noFallo;
        }else{
            noFallo = false;
        }

        if (!noFallo)
            errorMessage(tr("The image could not be saved"), tr("Bode diagram"));
    }
}
