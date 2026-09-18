#include "src/gui/common/qt_containers.h"
#include "src/gui/common/plot_export.h"
#include "src/gui/common/number_text.h"
#include "src/gui/loopshaping/loop_boundaries_viewer.h"
#include "ui_loop_boundaries_viewer.h"

#include "src/gui/application/error_message.h"
#include "src/gui/common/plot_setup.h"
#include "src/gui/common/trace_segments.h"


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

    //The column of controls does not take half the card: the chart needs
    //the width more than the buttons do.
    narrowSideColumn(ui->sideLayout);
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

        //Where the boundary of a frequency is more than one curve, the two
        //diagrams are cut at the same places: the Nyquist points are the
        //Nichols ones read in polar form, one for one.
        const std::vector<std::size_t> cuts = qftbx::segmentEnds(boundNichols);

        if (nichols){
            curves.push_back(piecesOf(phases, magnitudes, cuts, color));
            addFrequencyRow(color, frequencyIndex, tr("Nichols"));
        }

        //The Nyquist-only mode drew nothing: the curve also required
        //the Nichols flag.
        if (nyquist){
            curves.push_back(piecesOf(realParts, imaginaryParts, cuts, color2));
            addFrequencyRow(color2, frequencyIndex, tr("Nyquist"));
        }

        frequencyIndex++;
    }

    ui->plot->rescaleAxes();


    ui->plot->replot();
}

//One curve per piece, all of them one row of the legend.
QVector<QCPCurve *> LoopBoundariesViewer::piecesOf(const std::vector<double> & x,
                                                  const std::vector<double> & y,
                                                  const std::vector<std::size_t> & cuts,
                                                  const QColor & color)
{
    QVector<QCPCurve *> pieces;

    std::size_t from = 0;
    for (const std::size_t to : cuts) {
        std::vector<double> partX(x.begin() + std::ptrdiff_t(from), x.begin() + std::ptrdiff_t(to));
        std::vector<double> partY(y.begin() + std::ptrdiff_t(from), y.begin() + std::ptrdiff_t(to));
        from = to;

        if (partX.empty()) {
            continue;
        }

        QCPCurve * curve = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
        curve->setData(qftbx::toQVector(partX), qftbx::toQVector(partY));
        curve->setPen(QPen(color, kCurveWidth));

        if (partX.size() == 1) {
            curve->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));
        }

        pieces.push_back(curve);
    }

    return pieces;
}

void LoopBoundariesViewer::applyCheckboxes(){
    for (qint32 i = 0; i < legend->rowCount() && i < curves.size(); i++){
        for (QCPCurve * piece : curves.at(i)) {
            piece->setVisible(legend->isRowChecked(i));
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
