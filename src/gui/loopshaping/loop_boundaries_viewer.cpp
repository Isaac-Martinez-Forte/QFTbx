/**
 * @file
 * @brief Draws the two diagrams of the union, cut at the same places.
 *
 * Each frequency gets two rows, one per diagram, sharing its place in the
 * sweep and differing in shade so the pair reads as one. Where a boundary
 * is more than one curve the cuts are found on the Nichols trace and
 * applied to both diagrams, the Nyquist points being the Nichols ones read
 * in polar form, one for one. Every piece is its own curve under the one
 * legend row; a piece of one point is drawn as a disc.
 */

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

    qint32 c = 0;
    for (const qftbx::Trace & boundNichols : nicholsData->unionBoundaries()) {

        const qftbx::NyquistTrace & boundNyquist =
                nyquistTraces.at(static_cast<std::size_t>(frequencyIndex));

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

        const std::vector<std::size_t> cuts = qftbx::segmentEnds(boundNichols);

        if (nichols){
            curves.push_back(piecesOf(phases, magnitudes, cuts, color));
            addFrequencyRow(color, frequencyIndex, tr("Nichols"));
        }

        if (nyquist){
            curves.push_back(piecesOf(realParts, imaginaryParts, cuts, color2));
            addFrequencyRow(color2, frequencyIndex, tr("Nyquist"));
        }

        frequencyIndex++;
    }

    ui->plot->rescaleAxes();

    ui->plot->replot();
}

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

}
