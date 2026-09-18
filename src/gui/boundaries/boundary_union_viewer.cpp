#include "src/gui/common/qt_containers.h"
#include "src/gui/boundaries/boundary_union_viewer.h"
#include "ui_boundary_union_viewer.h"

#include "src/gui/application/error_message.h"
#include "src/gui/common/number_text.h"
#include "src/gui/common/plot_export.h"
#include "src/gui/common/plot_setup.h"
#include "src/gui/common/trace_segments.h"


namespace qftbx {

BoundaryUnionViewer::BoundaryUnionViewer(QWidget *parent) :
    QWidget(parent),
    ui(std::make_unique<Ui::BoundaryUnionViewer>())
{
    ui->setupUi(this);
    qftbx::setUpPlot(*ui->plot, tr("phase (degrees)"), tr("magnitude (dB)"));
    setWindowTitle(tr("Boundary union"));

    legend = new FrequencyLegend(ui->legendHolder);
    ui->legendHolder->layout()->addWidget(legend);

    //The column of controls does not take half the card: the chart needs
    //the width more than the buttons do.
    narrowSideColumn(ui->sideLayout);
    connect(legend, &FrequencyLegend::rowToggled, this, &BoundaryUnionViewer::applyCheckboxes);

    //Connected ONCE: a connection per replot duplicates the handler.
    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->plot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->yAxis2, SLOT(setRange(QCPRange)));
}

BoundaryUnionViewer::~BoundaryUnionViewer()
{
    clearDiagram();
}

void BoundaryUnionViewer::clearDiagram(){

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

void BoundaryUnionViewer::clear(){

    clearDiagram();
    unionTraces.clear();
    omega = nullptr;
    ui->plot->replot();
}

void BoundaryUnionViewer::setData(const qftbx::UnionTraces & unionTraces, std::vector<double> *omega){
    this->unionTraces = unionTraces;
    this->omega = omega;
}

void BoundaryUnionViewer::showDiagram(){

    clearDiagram();


    plotted = true;

    //One curve per design frequency, in the union's order.
    qint32 frequencyIndex = 0;
    for (const qftbx::Trace & bound : unionTraces) {
        const QColor color = frequencyColour(frequencyIndex, static_cast<int>(unionTraces.size()));

        //The boundary of a frequency is not always one curve, and the list
        //it arrives in does not say where one ends: drawn as a single
        //polyline it closed itself with a long straight line across the
        //chart. One curve per piece, all of them the frequency's colour.
        QVector<QCPCurve *> pieces;

        for (const qftbx::Trace & piece : qftbx::continuousSegments(bound)) {
            std::vector<double> phases;
            std::vector<double> magnitudes;
            phases.reserve(piece.size());
            magnitudes.reserve(piece.size());

            for (const qftbx::NicholsPoint & p : piece) {
                phases.push_back(p.phase);
                magnitudes.push_back(p.magnitude);
            }

            QCPCurve * curve = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
            curve->setData(qftbx::toQVector(phases), qftbx::toQVector(magnitudes));
            curve->setPen(QPen(color, kCurveWidth));

            //A point the ordering left on its own: drawn as the point it
            //is, because a curve of one draws nothing at all.
            if (piece.size() == 1) {
                curve->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));
            }

            pieces.push_back(curve);
        }

        curves.push_back(pieces);
        addFrequencyRow(color, frequencyIndex);

        frequencyIndex++;
    }


    ui->plot->rescaleAxes();


    ui->plot->replot();
}

void BoundaryUnionViewer::applyCheckboxes(){
    for (qint32 i = 0; i < legend->rowCount() && i < curves.size(); i++){
        for (QCPCurve * piece : curves.at(i)) {
            piece->setVisible(legend->isRowChecked(i));
        }
    }
    ui->plot->replot();
}

void BoundaryUnionViewer::addFrequencyRow(QColor color, qint32 pos){
    legend->addRow(numberText(omega->at(pos)), color);
}

void BoundaryUnionViewer::on_saveImage_clicked()
{
    qftbx::exportPlot(this, *ui->plot, tr("Boundary plot"));
}

} // namespace qftbx
