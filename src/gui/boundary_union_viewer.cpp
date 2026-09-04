#include "qt_containers.h"
#include "boundary_union_viewer.h"
#include "ui_boundary_union_viewer.h"

#include "src/gui/error_message.h"
#include "src/gui/number_text.h"
#include "src/gui/plot_export.h"
#include "src/gui/plot_palette.h"


namespace qftbx {

BoundaryUnionViewer::BoundaryUnionViewer(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::BoundaryUnionViewer>())
{
    ui->setupUi(this);
    setWindowTitle(tr("Boundary union"));

    frequenciesBox = new QGroupBox(this);
    frequenciesBox->setObjectName("frequenciesBox");
    frequenciesBox->setGeometry(QRect(10, 120, 120, 451));

    //Connected ONCE (every replot used to add a duplicated connection).
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

    //Qt's own mechanism: destroying the row widget is how a widget leaves
    //a layout, and it takes its checkbox with it.
    for (QCheckBox * checkbox : checkboxes) {
        delete checkbox->parentWidget();
    }
    checkboxes.clear();

    curves.clear();

    //Also Qt's: a widget holds exactly one layout, so rebuilding the
    //frequency box means destroying the one it has.
    delete colorsLayout;
    colorsLayout = nullptr;

    plotted = false;
}

void BoundaryUnionViewer::setData(const qftbx::UnionTraces & unionTraces, std::vector<double> *omega){
    this->unionTraces = unionTraces;
    this->omega = omega;
}

void BoundaryUnionViewer::showDiagram(){

    clearDiagram();

    colorsLayout = new QVBoxLayout (frequenciesBox);

    plotted = true;

    //One curve per design frequency, in the union's order.
    qint32 frequencyIndex = 0;
    for (const qftbx::Trace & bound : unionTraces) {
        const QColor color = randomColor(frequencyIndex);

        std::vector<double> phases;
        std::vector<double> magnitudes;
        phases.reserve(bound.size());
        magnitudes.reserve(bound.size());

        for (const qftbx::NicholsPoint & p : bound) {
            phases.push_back(p.phase);
            magnitudes.push_back(p.magnitude);
        }

        QCPCurve *curve = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
        curve->setData(qftbx::toQVector(phases), qftbx::toQVector(magnitudes));
        curve->setPen(color);
        curves.push_back(curve);
        addFrequencyRow(color, frequencyIndex);

        frequencyIndex++;
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

void BoundaryUnionViewer::applyCheckboxes(){
    for (qint32 i = 0; i < checkboxes.size(); i++){
        curves.at(i)->setVisible(checkboxes.at(i)->checkState() != Qt::Unchecked);
    }
    ui->plot->replot();
}

void BoundaryUnionViewer::addFrequencyRow(QColor color, qint32 pos){

    QWidget * widget = new QWidget(frequenciesBox);
    widget->setObjectName("widget");
    widget->setGeometry(QRect(10, 10, 111, 23));

    QCheckBox * checkBox = new QCheckBox(widget);
    checkBox->setObjectName("checkBox");
    checkBox->setText(qftbx::numberText(omega->at(pos)));
    checkBox->setStyleSheet("color : " + color.name());
    checkBox->setCheckState(Qt::Checked);

    colorsLayout->addWidget(widget);
    checkboxes.push_back(checkBox);

    connect(checkBox, SIGNAL (clicked()), this, SLOT (applyCheckboxes()));
}

void BoundaryUnionViewer::on_saveImage_clicked()
{
    qftbx::exportPlot(this, *ui->plot, tr("Boundary plot"));
}

} // namespace qftbx
