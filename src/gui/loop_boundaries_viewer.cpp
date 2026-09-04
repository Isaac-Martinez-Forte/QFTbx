#include "qt_containers.h"
#include "src/gui/plot_export.h"
#include "src/gui/number_text.h"
#include "loop_boundaries_viewer.h"
#include "ui_loop_boundaries_viewer.h"

#include "src/gui/error_message.h"
#include "src/gui/plot_palette.h"


using namespace tools;

LoopBoundariesViewer::LoopBoundariesViewer(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::LoopBoundariesViewer>())
{
    ui->setupUi(this);
    setWindowTitle(tr("Boundary union"));


    frequenciesBox = new QGroupBox(this);
    frequenciesBox->setObjectName("frequenciesBox");
    frequenciesBox->setGeometry(QRect(660, 0, 141, 461));
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

    //Qt's own mechanism: destroying the row widget is how a widget leaves
    //a layout, and it takes its checkbox with it.
    for (QCheckBox * che : checkboxes) {
        delete che->parentWidget();
    }
    checkboxes.clear();

    curves.clear();

    //Also Qt's: a widget holds exactly one layout, so rebuilding the
    //frequency box means destroying the one it has.
    delete colorsLayout;
    colorsLayout = nullptr;

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

    colorsLayout = new QVBoxLayout (frequenciesBox);

    plotted = true;

    qint32 frequencyIndex = 0;

    QVector <QColor> rowColors;

    //Sweep the design frequencies.

    qint32 c = 0;
    for (const qftbx::Trace & boundNichols : nicholsData->unionBoundaries()) {

        const qftbx::NyquistTrace & boundNyquist =
                nyquistTraces.at(static_cast<std::size_t>(frequencyIndex));


        QColor color = randomColor(c);
        c++;
        QColor color2 = randomColor(c);
        c++;

        rowColors.push_back(color);
        rowColors.push_back(color2);

        std::vector<double> ejex;
        std::vector<double> ejey;

        std::vector<double> ejex1;
        std::vector<double> ejey1;

        qint32 secondIndex = 0;

        for (const qftbx::NicholsPoint & pNichols : boundNichols) {
            const qftbx::NyquistPoint pNyquist = boundNyquist.at(static_cast<std::size_t>(secondIndex));
            ejex.push_back(pNichols.phase);
            ejey.push_back(pNichols.magnitude);

            ejex1.push_back(pNyquist.re);
            ejey1.push_back(pNyquist.im);


            secondIndex++;
        }

        if (nichols){
            QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
            curva->setData(tools::toQVector(ejex), tools::toQVector(ejey));
            curva->setPen(color);
            addFrequencyRow(color, frequencyIndex, tr("Nichols"));
            curves.push_back(curva);
        }

        //The Nyquist-only mode drew nothing: the curve also required
        //the Nichols flag.
        if (nyquist){
            QCPCurve *curva2 = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
            curva2->setData(tools::toQVector(ejex1), tools::toQVector(ejey1));
            curva2->setPen(color2);
            addFrequencyRow(color2, frequencyIndex, tr("Nyquist"));
            curves.push_back(curva2);
        }

        frequencyIndex++;
    }

    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui->plot->axisRect()->setupFullAxesBox();
    ui->plot->rescaleAxes();



    ui->plot->replot();
}

void LoopBoundariesViewer::applyCheckboxes(){
    for (qint32 i = 0; i < checkboxes.size(); i++){
        if (checkboxes.at(i)->checkState() == Qt::Unchecked){
            curves.at(i)->setVisible(false);
        }else {
            curves.at(i)->setVisible(true);
        }
    }
    ui->plot->replot();
}

void LoopBoundariesViewer::addFrequencyRow(QColor color, qint32 pos, QString diagram){

    QWidget *widget;
    QCheckBox *checkBox;

    widget = new QWidget(frequenciesBox);
    widget->setObjectName("widget");
    widget->setGeometry(QRect(10, 10, 111, 23));
    checkBox = new QCheckBox(widget);
    checkBox->setObjectName("checkBox");

    checkBox->setText(tools::numberText(omega->at(pos)) + " " + diagram);

    checkBox->setStyleSheet("color : " + color.name());

    colorsLayout->addWidget(widget);
    checkboxes.push_back(checkBox);
    checkBox->setCheckState(Qt::Checked);

    connect(checkBox, SIGNAL (clicked()), this, SLOT (applyCheckboxes()));
}


void LoopBoundariesViewer::on_saveImage_clicked()
{
    tools::exportPlot(this, *ui->plot, tr("Boundary plot"));
}
