#include "src/gui/common/qt_containers.h"
#include "src/gui/common/plot_export.h"
#include "src/gui/common/number_text.h"
#include "src/gui/loopshaping/loop_boundaries_viewer.h"
#include "ui_loop_boundaries_viewer.h"

#include "src/gui/application/error_message.h"
#include "src/gui/common/plot_setup.h"


namespace qftbx {

LoopBoundariesViewer::LoopBoundariesViewer(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::LoopBoundariesViewer>())
{
    ui->setupUi(this);
    qftbx::setUpPlot(*ui->plot, tr("phase (degrees)"), tr("magnitude (dB)"));
    setWindowTitle(tr("Boundary union"));


    legend = new FrequencyLegend(ui->legendHolder);
    ui->legendHolder->layout()->addWidget(legend);
    connect(legend, &FrequencyLegend::rowToggled, this, &LoopBoundariesViewer::applyCheckboxes);
}

LoopBoundariesViewer::~LoopBoundariesViewer()
{
    clearDiagram();

}

void LoopBoundariesViewer::clearDiagram(){

    if (!plotted){
        return;
    }

    ui->plot->clearFocus();
    ui->plot->clearGraphs();
    ui->plot->clearItems();
    //QCustomPlot owns the curves: clearPlottables frees them.
    ui->plot->clearPlottables();

    legend->clear();

    curves.clear();


    plotted = false;
}


void LoopBoundariesViewer::setData(const BoundaryData *nicholsData,
                             const qftbx::NyquistTraces & nyquistTraces, std::vector<double> *omega,
                             LtiSystem *plant, LtiSystem *controller, bool nichols, bool nyquist){
    this->nicholsData = nicholsData;
    this->nyquistTraces = nyquistTraces;
    this->plant = plant;
    this->controller = controller;
    this->omega = omega;
    this->nichols = nichols;
    this->nyquist = nyquist;
}

void LoopBoundariesViewer::showDiagram(){


    clearDiagram();


    plotted = true;

    qint32 frequencyIndex = 0;

    QVector <QColor> rowColors;

    //Sweep the design frequencies.

    qint32 c = 0;
    for (const qftbx::Trace & boundNichols : nicholsData->unionBoundaries()) {

        const qftbx::NyquistTrace & boundNyquist =
                nyquistTraces.at(static_cast<std::size_t>(frequencyIndex));


        //Two rows per frequency, one per diagram: the pair shares the
        //frequency's place in the sweep and differs in shade, so the Nichols
        //and the Nyquist curve of one frequency read as a pair.
        const int frequencies = static_cast<int>(nicholsData->unionBoundaries().size());
        QColor color = frequencyColour(c / 2, frequencies);
        QColor color2 = color.lighter(145);
        c += 2;

        rowColors.push_back(color);
        rowColors.push_back(color2);

        std::vector<double> phases;
        std::vector<double> magnitudes;

        std::vector<double> realParts;
        std::vector<double> imaginaryParts;

        qint32 secondIndex = 0;

        for (const qftbx::NicholsPoint & pNichols : boundNichols) {
            const qftbx::NyquistPoint pNyquist = boundNyquist.at(static_cast<std::size_t>(secondIndex));
            phases.push_back(pNichols.phase);
            magnitudes.push_back(pNichols.magnitude);

            realParts.push_back(pNyquist.re);
            imaginaryParts.push_back(pNyquist.im);


            secondIndex++;
        }

        if (nichols){
            QCPCurve *curve = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
            curve->setData(qftbx::toQVector(phases), qftbx::toQVector(magnitudes));
            curve->setPen(QPen(color, kCurveWidth));
            addFrequencyRow(color, frequencyIndex, tr("Nichols"));
            curves.push_back(curve);
        }

        //The Nyquist-only mode drew nothing: the curve also required
        //the Nichols flag.
        if (nyquist){
            QCPCurve *nyquistCurve = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
            nyquistCurve->setData(qftbx::toQVector(realParts), qftbx::toQVector(imaginaryParts));
            nyquistCurve->setPen(QPen(color2, kCurveWidth));
            addFrequencyRow(color2, frequencyIndex, tr("Nyquist"));
            curves.push_back(nyquistCurve);
        }

        frequencyIndex++;
    }

    ui->plot->rescaleAxes();


    ui->plot->replot();
}

void LoopBoundariesViewer::applyCheckboxes(){
    for (qint32 i = 0; i < legend->rowCount(); i++){
        if (!legend->isRowChecked(i)){
            curves.at(i)->setVisible(false);
        }else {
            curves.at(i)->setVisible(true);
        }
    }
    ui->plot->replot();
}

void LoopBoundariesViewer::addFrequencyRow(QColor color, qint32 pos, QString diagram){
    legend->addRow(numberText(omega->at(pos)) + " " + diagram, color);
}


void LoopBoundariesViewer::on_saveImage_clicked()
{
    qftbx::exportPlot(this, *ui->plot, tr("Boundary plot"));
}

} // namespace qftbx
