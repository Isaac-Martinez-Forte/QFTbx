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

void BodeViewer::drawBode(LtiSystem *planta, Omega *omega){

    //Replottable: without this every call piled new curves up.
    ui->magnitudePlot->clearPlottables();
    ui->phasePlot->clearPlottables();

    QVector <qreal> * frequencies;
    bool ownFrequencies = true;

    if (omega->type() == Omega::LinSpace){
        frequencies = linspace(-1, omega->end(),100);
    }else if (omega->type() == Omega::LogSpace){
        frequencies = logspace(-1, omega->end(),100);
    }else {
        //DAO's vector: not freed here.
        frequencies = omega->values();
        ownFrequencies = false;
    }

    QVector <qreal> * magnitude = new QVector <qreal> ();
    QVector <qreal> * phase = new QVector <qreal> ();
    magnitude->reserve(frequencies->size());
    phase->reserve(frequencies->size());

    QVector <std::complex<qreal> > * values = planta->evaluate(frequencies);


   foreach (const std::complex<qreal> &comp, *values){
        magnitude->append(20*log10(abs(comp)));
        phase->append(arg(comp));
    }

    drawAxis("Magnitud",magnitude,frequencies,ui->magnitudePlot);
    drawAxis("Fase" ,phase,frequencies,ui->phasePlot);
    this->setWindowTitle(tr("Bode diagram"));
    ui->magnitudePlot->replot();
    ui->phasePlot->replot();

    delete values;
    delete magnitude;
    delete phase;
    if (ownFrequencies){
        delete frequencies;
    }
}


void BodeViewer::drawAxis(QString yAxisName, QVector<qreal> * yAxis_values, QVector<qreal> * frequencies, QCustomPlot * magnitudePlot){

    QCPCurve *curva = new QCPCurve(magnitudePlot->xAxis, magnitudePlot->yAxis);
    curva->setData(*frequencies, *yAxis_values);

    magnitudePlot->xAxis->setLabel("w");
    magnitudePlot->yAxis->setLabel(yAxisName);

    magnitudePlot->xAxis->setScaleType(QCPAxis::ScaleType::stLogarithmic);
    magnitudePlot->xAxis->setRange(frequencies->first(), frequencies->last());
    magnitudePlot->yAxis->setRange(yAxis_values->first(), yAxis_values->last());
}

void BodeViewer::on_actionExport_triggered()
{
    bool noFallo = true;
    QString extension;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save file"),"",
      tr((".png (*.png);;.pdf(*.pdf);; .jpg(*.jpg);; .bmp(*.bmp)")), &extension);
    if (!fileName.isEmpty()){
        //Prefixing the FULL path ("0-/home/...") produced invalid paths:
        //the suffix goes on the file name.
        QFileInfo info (fileName);
        const QString magnitud = info.dir().filePath(info.completeBaseName() + "-mag." + info.suffix());
        const QString faseNombre = info.dir().filePath(info.completeBaseName() + "-phase." + info.suffix());

        if (extension.contains(".pdf", Qt::CaseInsensitive)){
            noFallo = ui->magnitudePlot->savePdf(magnitud, true);
            noFallo = ui->phasePlot->savePdf(faseNombre, true) && noFallo;
        }else if (extension.contains(".png", Qt::CaseInsensitive)){
            noFallo = ui->magnitudePlot->savePng(magnitud);
            noFallo = ui->phasePlot->savePng(faseNombre) && noFallo;
        }else if (extension.contains(".jpg", Qt::CaseInsensitive)){
            noFallo = ui->magnitudePlot->saveJpg(magnitud);
            noFallo = ui->phasePlot->saveJpg(faseNombre) && noFallo;
        }else if (extension.contains(".bmp", Qt::CaseInsensitive)){
            noFallo = ui->magnitudePlot->saveBmp(magnitud);
            noFallo = ui->phasePlot->saveBmp(faseNombre) && noFallo;
        }else{
            noFallo = false;
        }

        if (!noFallo)
            errorMessage(tr("The image could not be saved"), tr("Bode diagram"));
    }
}
