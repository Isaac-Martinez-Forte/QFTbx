#include <algorithm>

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

    //By value, so the sweep is a plain local: this used to be a pointer plus
    //an ownFrequencies flag, because the manual branch borrowed the project's
    //vector while the other two allocated their own.
    QVector <qreal> frequencies;

    //The sweep starts where the DESIGN starts. Both branches used a
    //hardcoded -1: on the logarithmic path that silently replaced the user's
    //first exponent, and on the linear one it asked for frequencies from
    //-1 rad/s - negative frequencies, on an axis that is logarithmic below.
    if (omega->type() == Omega::LinSpace){
        frequencies = linspace(omega->start(), omega->end(), 100);
    }else if (omega->type() == Omega::LogSpace){
        //start()/end() hold the EXPONENTS on this path, as logspace expects.
        frequencies = logspace(omega->start(), omega->end(), 100);
    }else {
        frequencies = *omega->values();
    }

    QVector <qreal> magnitude;
    QVector <qreal> phase;
    magnitude.reserve(frequencies.size());
    phase.reserve(frequencies.size());

    foreach (const std::complex<qreal> &comp, planta->evaluate(frequencies)){
        magnitude.append(20*log10(abs(comp)));

        //Degrees: a Bode phase plot is read in degrees, and arg() answers
        //radians (the axis was labelled in Spanish and scaled in radians).
        phase.append(arg(comp) * 180.0 / M_PI);
    }

    drawAxis(tr("Magnitude (dB)"), magnitude, frequencies, ui->magnitudePlot);
    drawAxis(tr("Phase (deg)"), phase, frequencies, ui->phasePlot);
    this->setWindowTitle(tr("Bode diagram"));
    ui->magnitudePlot->replot();
    ui->phasePlot->replot();
}


void BodeViewer::drawAxis(QString yAxisName, const QVector<qreal> & yAxis_values,
                          const QVector<qreal> & frequencies, QCustomPlot * magnitudePlot){

    //QCPCurve attaches itself to the plot, which owns it from then on.
    QCPCurve *curva = new QCPCurve(magnitudePlot->xAxis, magnitudePlot->yAxis);
    curva->setData(frequencies, yAxis_values);

    magnitudePlot->xAxis->setLabel("w");
    magnitudePlot->yAxis->setLabel(yAxisName);

    magnitudePlot->xAxis->setScaleType(QCPAxis::ScaleType::stLogarithmic);

    //Both axes span the EXTREMES of the data. They used to span first to
    //last, which only frames the curve when it happens to be monotonic and
    //the frequencies happen to be sorted (a manual set need not be).
    const auto frequencyEnds = std::minmax_element(frequencies.begin(), frequencies.end());
    const auto valueEnds = std::minmax_element(yAxis_values.begin(), yAxis_values.end());

    magnitudePlot->xAxis->setRange(*frequencyEnds.first, *frequencyEnds.second);
    magnitudePlot->yAxis->setRange(*valueEnds.first, *valueEnds.second);
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
