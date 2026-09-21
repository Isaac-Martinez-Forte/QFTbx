/**
 * @file
 * @brief Draws the boundary union as one curve per continuous piece.
 *
 * The boundary of a frequency is not always one curve and the list it
 * arrives in does not say where one ends, so each continuous piece is its
 * own curve in the frequency's colour; a piece of one point is drawn as a
 * disc, since a curve of one draws nothing. A legend row shows or hides
 * all the pieces of its frequency. The secondary axes mirror the primary
 * ones and are connected once, in the constructor; the side column is
 * narrowed so the chart gets the width.
 */

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

    narrowSideColumn(ui->sideLayout);
    connect(legend, &FrequencyLegend::rowToggled, this, &BoundaryUnionViewer::applyCheckboxes);

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

    qint32 frequencyIndex = 0;
    for (const qftbx::Trace & bound : unionTraces) {
        const QColor color = frequencyColour(frequencyIndex, static_cast<int>(unionTraces.size()));

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

}
