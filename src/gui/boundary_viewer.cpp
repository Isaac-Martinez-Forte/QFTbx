#include "qt_containers.h"
#include "src/gui/plot_export.h"
#include "src/gui/number_text.h"
#include "boundary_viewer.h"
#include "ui_boundary_viewer.h"

#include "src/gui/error_message.h"
#include "src/gui/plot_palette.h"


namespace qftbx {

BoundaryViewer::BoundaryViewer(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::BoundaryViewer>())
{
    ui->setupUi(this);
    setWindowTitle(tr("Boundaries"));

    frequenciesBox = new QGroupBox(this);
    frequenciesBox->setObjectName("frequenciesBox");
    frequenciesBox->setGeometry(QRect(660, 0, 141, 461));

    //Mirrored secondary axes, connected ONCE: every repaint used to add a
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

    //Qt's own mechanism, and the only reason there is a delete left here:
    //destroying the row widget is how a widget leaves a layout, and it
    //takes its checkbox with it.
    for (QCheckBox * che : checkboxes) {
        delete che->parentWidget();
    }
    checkboxes.clear();

    //Also Qt's: a widget holds exactly one layout, so rebuilding the
    //frequency box means destroying the one it has.
    delete colorsLayout;
    colorsLayout = nullptr;

    plotted = false;
}

void BoundaryViewer::setData(const BoundaryData *data, std::vector<double> * omega){

    boundaryData = data;
    this->omega = omega;
}

void BoundaryViewer::showDiagram(){

    qint32 k = 0;

    clearDiagram();

    colorsLayout = new QVBoxLayout (frequenciesBox);
    plotted = true;

    const qftbx::BoundarySet & boundarySet = this->boundaryData->boundaries();

    //Sweep the design frequencies.
    for (qint32 i = 0; i < static_cast<qint32>(boundarySet.size()); i++) {

        QVector <QCPCurve *> frequencyCurves;

        QColor color = randomColor(i);

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
                curve->setPen(color);
                frequencyCurves.push_back(curve);

                k++;
            }
        }

        curves.push_back(std::move(frequencyCurves));
    }

    ui->plot->xAxis2->setVisible(true);
    ui->plot->xAxis2->setTickLabels(false);
    ui->plot->yAxis2->setVisible(true);
    ui->plot->yAxis2->setTickLabels(false);

    ui->plot->axisRect()->setupFullAxesBox();
    ui->plot->rescaleAxes();

    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    ui->plot->replot();

}

void BoundaryViewer::addFrequencyRow(QColor color, qint32 pos){

    QWidget *widget;
    QCheckBox *checkBox;

    widget = new QWidget(frequenciesBox);
    widget->setObjectName("widget");
    widget->setGeometry(QRect(10, 10, 111, 23));
    checkBox = new QCheckBox(widget);
    checkBox->setObjectName("checkBox");

    checkBox->setText(qftbx::numberText(omega->at(pos)));

    checkBox->setStyleSheet("color : " + color.name());

    colorsLayout->addWidget(widget);
    checkboxes.push_back(checkBox);
    checkBox->setCheckState(Qt::Checked);

    connect(checkBox, SIGNAL (clicked()), this, SLOT (applyCheckboxes()));
}

void BoundaryViewer::applyCheckboxes(){
    for (qint32 i = 0; i < checkboxes.size(); i++){
        if (checkboxes.at(i)->checkState() == Qt::Unchecked){

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
