#include "src/gui/common/qt_containers.h"
#include "src/gui/common/plot_export.h"
#include "src/gui/common/number_text.h"
#include "src/gui/boundaries/boundary_viewer.h"
#include "ui_boundary_viewer.h"

#include "src/gui/application/error_message.h"
#include "src/gui/common/plot_setup.h"


namespace qftbx {

BoundaryViewer::BoundaryViewer(QWidget *parent) :
    QWidget(parent),
    ui(std::make_unique<Ui::BoundaryViewer>())
{
    ui->setupUi(this);
    qftbx::setUpPlot(*ui->plot, tr("phase (degrees)"), tr("magnitude (dB)"));
    setWindowTitle(tr("Boundaries"));

    legend = new FrequencyLegend(ui->legendHolder);
    ui->legendHolder->layout()->addWidget(legend);

    //The column of controls does not take half the card: the chart needs
    //the width more than the buttons do.
    narrowSideColumn(ui->sideLayout);
    connect(legend, &FrequencyLegend::rowToggled, this, &BoundaryViewer::applyCheckboxes);

    //Mirrored secondary axes, connected ONCE: a connection per repaint adds a
    //duplicate connection.
    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->plot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->yAxis2, SLOT(setRange(QCPRange)));
}

BoundaryViewer::~BoundaryViewer()
{
    clearDiagram();

}

void BoundaryViewer::clearDiagram(){

    if (!plotted){
        return;
    }

    ui->plot->clearFocus();
    ui->plot->clearGraphs();
    ui->plot->clearItems();
    //QCustomPlot owns the curves: clearPlottables frees them.
    ui->plot->clearPlottables();

    //The containers are members, so only the frequency-box ROWS are freed
    //here: deleting just the checkbox left its container widget piling up
    //in the layout on every replot.
    curves.clear();

    legend->clear();


    plotted = false;
}

void BoundaryViewer::clear(){

    clearDiagram();
    boundaryData = nullptr;
    omega = nullptr;
    ui->plot->replot();
}

void BoundaryViewer::setData(const BoundaryData *data, std::vector<double> * omega){

    boundaryData = data;
    this->omega = omega;
}

void BoundaryViewer::showDiagram(){

    qint32 k = 0;

    clearDiagram();

    plotted = true;

    const qftbx::BoundarySet & boundarySet = this->boundaryData->boundaries();

    //Sweep the design frequencies.
    for (qint32 i = 0; i < static_cast<qint32>(boundarySet.size()); i++) {

        QVector <QCPCurve *> frequencyCurves;

        QColor color = frequencyColour(i, static_cast<int>(boundarySet.size()));

        addFrequencyRow(color, i);

        const auto & map = boundarySet.at(static_cast<std::size_t>(i));
        for (const auto & entry : map) {
            const qftbx::TraceSet & b = entry.second;
            for (const qftbx::Trace & bound : b) {

                std::vector<double> phases;
                std::vector<double> magnitudes;
                phases.reserve(static_cast<qsizetype>(bound.size()));
                magnitudes.reserve(static_cast<qsizetype>(bound.size()));

                for (const qftbx::NicholsPoint & p : bound) {
                   phases.push_back(p.phase);
                   magnitudes.push_back(p.magnitude);
                }


                QCPCurve *curve = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
                curve->setData(qftbx::toQVector(phases), qftbx::toQVector(magnitudes));
                curve->setPen(QPen(color, kCurveWidth));
                frequencyCurves.push_back(curve);

                k++;
            }
        }

        curves.push_back(std::move(frequencyCurves));
    }


    ui->plot->rescaleAxes();


    ui->plot->replot();

}

void BoundaryViewer::addFrequencyRow(QColor color, qint32 pos){
    legend->addRow(numberText(omega->at(pos)), color);
}

void BoundaryViewer::applyCheckboxes(){
    for (qint32 i = 0; i < legend->rowCount(); i++){
        if (!legend->isRowChecked(i)){

            for (qint32 j = 0; j < curves.at(i).size(); j++){
                curves.at(i).at(j)->setVisible(false);
            }
        }else {
            for (qint32 j = 0; j < curves.at(i).size(); j++){
                curves.at(i).at(j)->setVisible(true);
            }
        }
    }
    ui->plot->replot();
}

void BoundaryViewer::on_saveImage_clicked()
{
    qftbx::exportPlot(this, *ui->plot, tr("Boundary plot"));
}

} // namespace qftbx
