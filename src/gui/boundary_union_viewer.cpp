#include "qt_containers.h"
#include "boundary_union_viewer.h"
#include "ui_boundary_union_viewer.h"

#include "src/gui/error_message.h"
#include "src/gui/plot_palette.h"


using namespace tools;

BoundaryUnionViewer::BoundaryUnionViewer(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::BoundaryUnionViewer>())
{
    ui->setupUi(this);
    setWindowTitle(tr("Boundary union"));
    plotted = false;


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

    //boxCurves/boxCurves2 observe plottables that clearPlottables frees:
    //keeping them across replots left dangling pointers in
    //applyCheckboxes, so they are emptied always.
    boxCurves.clear();
    boxCurves2.clear();

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

    colors.clear();

    plotted = false;
}


void BoundaryUnionViewer::setData(const qftbx::UnionTraces & unionTraces, std::vector<double> *omega){
    this->unionTraces = unionTraces;
    this->omega = omega;
    bucketMode = false;
    //Without this, a previous single-boundary use left the index stuck and
    //this mode painted a single frequency.
    singleBoundary = -1;
    b.clear();
}

void BoundaryUnionViewer::setData(const qftbx::UnionTraces & unionTraces, std::vector<double> *omega, qint32 singleBoundary){
    this->unionTraces = unionTraces;
    this->omega = omega;
    bucketMode = false;
    this->singleBoundary = singleBoundary;
}

void BoundaryUnionViewer::setData (const qftbx::UnionBuckets & unionTraces, std::vector<double> *omega) {
    unionBuckets = unionTraces;
    this->omega = omega;
    bucketMode = true;
    singleBoundary = -1;
    b.clear();
}

void BoundaryUnionViewer::setData (const qftbx::UnionBuckets & unionTraces, std::vector<double> *omega, const qftbx::Trace & b) {
    unionBuckets = unionTraces;
    this->omega = omega;
    bucketMode = true;
    singleBoundary = -1;
    this->b = b;
}

void BoundaryUnionViewer::showDiagram(){

    clearDiagram();

    colorsLayout = new QVBoxLayout (frequenciesBox);


    plotted = true;

    qint32 k = 0;

    //Sweep the design frequencies.

    qint32 frequencyIndex = 0;

    if (singleBoundary < 0) {

        if (!bucketMode){
            for (const qftbx::Trace & bound : unionTraces) {
                QColor color = randomColor(frequencyIndex);
                frequencyIndex++;
                colors.push_back(color);

                std::vector<double> ejex;
                std::vector<double> ejey;

                for (const qftbx::NicholsPoint & p : bound) {
                    ejex.push_back(p.phase);
                    ejey.push_back(p.magnitude);
                }

                QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
                curva->setData(tools::toQVector(ejex), tools::toQVector(ejey));
                curva->setPen(color);
                curves.push_back(curva);
                addFrequencyRow(color, k);


                k++;
            }
        } else {

            //The container belongs to the DAO: removeLast() used to be
            //called on it and the last frequency vanished PERMANENTLY from
            //the project.
            const qint32 frequencyCount = static_cast<qint32>(
                        this->b.empty() ? unionBuckets.size() : unionBuckets.size() - 1);

            for (qint32 f = 0; f < frequencyCount; f++) {
                const qftbx::TraceSet & bound = unionBuckets.at(static_cast<std::size_t>(f));
                QColor color = randomColor(frequencyIndex);
                frequencyIndex++;
                colors.push_back(color);

                std::vector<double> ejex;
                std::vector<double> ejey;

                for (const qftbx::Trace & bucket : bound) {
                    for (const qftbx::NicholsPoint & p : bucket) {
                        ejex.push_back(p.phase);
                        ejey.push_back(p.magnitude);
                    }
                }

                QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
                curva->setData(tools::toQVector(ejex), tools::toQVector(ejey));
                curva->setPen(color);
                curves.push_back(curva);

                addFrequencyRow(color, k);


                k++;
            }
            if (!this->b.empty()){
                QColor color = randomColor(frequencyIndex);
                frequencyIndex++;
                colors.push_back(color);

                std::vector<double> ejex;
                std::vector<double> ejey;

                for (const qftbx::NicholsPoint & p : this->b) {
                    ejex.push_back(p.phase);
                    ejey.push_back(p.magnitude);
                }

                QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
                curva->setData(tools::toQVector(ejex), tools::toQVector(ejey));
                curva->setPen(color);
                curves.push_back(curva);

                addFrequencyRow(color, k);


                k++;
            }

        }
    } else {
        QColor color = randomColor(frequencyIndex);
        frequencyIndex++;
        colors.push_back(color);

                std::vector<double> ejex;
                std::vector<double> ejey;

        for (const qftbx::NicholsPoint & p : unionTraces.at(static_cast<std::size_t>(singleBoundary))) {
            ejex.push_back(p.phase);
            ejey.push_back(p.magnitude);
        }

        QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
        curva->setData(tools::toQVector(ejex), tools::toQVector(ejey));
        curva->setPen(color);
        curves.push_back(curva);
        addFrequencyRow(color, k);


        k++;
    }

    ui->plot->xAxis2->setVisible(true);
    ui->plot->xAxis2->setTickLabels(false);
    ui->plot->yAxis2->setVisible(true);
    ui->plot->yAxis2->setTickLabels(false);

    ui->plot->axisRect()->setupFullAxesBox();
    ui->plot->rescaleAxes();

    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    finalCurveIndex = k++;

    ui->plot->replot();
}

void BoundaryUnionViewer::applyCheckboxes(){
    for (qint32 i = 0; i < checkboxes.size(); i++){
        if (checkboxes.at(i)->checkState() == 0){
            curves.at(i)->setVisible(false);
            if (i < boxCurves.size()){
                boxCurves.at(i)->setVisible(false);
            }
            if (i < boxCurves2.size()){
                boxCurves2.at(i)->setVisible(false);
            }
        }else {
            curves.at(i)->setVisible(true);
            if (i < boxCurves.size()){
                boxCurves.at(i)->setVisible(true);
            }
            if (i < boxCurves2.size()){
                boxCurves2.at(i)->setVisible(true);
            }
        }
    }
    ui->plot->replot();
}

void BoundaryUnionViewer::addFrequencyRow(QColor color, qint32 pos){

    QWidget *widget;
    QCheckBox *checkBox;

    widget = new QWidget(frequenciesBox);
    widget->setObjectName("widget");
    widget->setGeometry(QRect(10, 10, 111, 23));
    checkBox = new QCheckBox(widget);
    checkBox->setObjectName("checkBox");

    QMetaObject::connectSlotsByName(widget);


    if (pos < static_cast<std::int32_t>(omega->size())){
        checkBox->setText(QString::number(omega->at(pos)));
    } else {
        checkBox->setText(tr("union"));
    }

    checkBox->setStyleSheet("color : " + color.name());

    colorsLayout->addWidget(widget);
    checkboxes.push_back(checkBox);
    checkBox->setCheckState(Qt::Checked);

    connect(checkBox, SIGNAL (clicked()), this, SLOT (applyCheckboxes()));
}


void BoundaryUnionViewer::drawBox(QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, qint32 frequencyIndex){

    std::vector<double> ejex;
    std::vector<double> ejey;

    ejex.push_back(uno.x());
    ejex.push_back(dos.x());
    ejex.push_back(tres.x());
    ejex.push_back(cuatro.x());
    ejex.push_back(uno.x());

    ejey.push_back(uno.y());
    ejey.push_back(dos.y());
    ejey.push_back(tres.y());
    ejey.push_back(cuatro.y());
    ejey.push_back(uno.y());

    /*boxCurves.push_back(ui->plot->addGraph());
    ui->plot->graph(finalCurveIndex)->setData(tools::toQVector(ejex), tools::toQVector(ejey));

    ui->plot->graph(finalCurveIndex)->setPen(colors.at(frequencyIndex));
    ui->plot->graph(finalCurveIndex)->setLineStyle(QCPGraph::lsLine);
    ui->plot->graph(finalCurveIndex)->setScatterStyle(QCPScatterStyle::ssCross);
    ui->plot->graph(finalCurveIndex)->rescaleAxes(true);*/

    QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
    curva->setData(tools::toQVector(ejex), tools::toQVector(ejey));
    curva->setPen(colors.at(frequencyIndex));
    boxCurves.push_back(curva);

    //ui->plot->rescaleAxes(true);

    ui->plot->replot();
}

void BoundaryUnionViewer::drawBox2(QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, qint32 frequencyIndex){

    std::vector<double> ejex;
    std::vector<double> ejey;

    ejex.push_back(uno.x());
    ejex.push_back(dos.x());
    ejex.push_back(tres.x());
    ejex.push_back(cuatro.x());
    ejex.push_back(uno.x());

    ejey.push_back(uno.y());
    ejey.push_back(dos.y());
    ejey.push_back(tres.y());
    ejey.push_back(cuatro.y());
    ejey.push_back(uno.y());

    /*boxCurves2.push_back(ui->plot->addGraph());
    ui->plot->graph(finalCurveIndex)->setData(tools::toQVector(ejex), tools::toQVector(ejey));

    ui->plot->graph(finalCurveIndex)->setPen(colors.at(frequencyIndex));
    ui->plot->graph(finalCurveIndex)->setLineStyle(QCPGraph::lsLine);
    ui->plot->graph(finalCurveIndex)->setScatterStyle(QCPScatterStyle::ssCircle);*/

    QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
    curva->setData(tools::toQVector(ejex), tools::toQVector(ejey));
    curva->setPen(colors.at(frequencyIndex));
    boxCurves2.push_back(curva);

    ui->plot->rescaleAxes(true);

    ui->plot->replot();
}

void BoundaryUnionViewer::on_saveImage_clicked()
{
    bool noFallo = true;
    QString extension;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save file"),"",
                                                    tr((".png (*.png);;.pdf(*.pdf);; .jpg(*.jpg);; .bmp(*.bmp)")), &extension);
    if (!fileName.isEmpty()){
        if (extension.contains(".pdf", Qt::CaseInsensitive)){
            noFallo = ui->plot->savePdf(fileName, true);
        }else if (extension.contains(".png", Qt::CaseInsensitive)){
            noFallo = ui->plot->savePng(fileName);
        }else if (extension.contains(".jpg", Qt::CaseInsensitive)){
            noFallo = ui->plot->saveJpg(fileName);
        }else if (extension.contains(".bmp", Qt::CaseInsensitive)){
            noFallo = ui->plot->saveBmp(fileName);
        }else{
            noFallo = false;
        }

        if (!noFallo)
            errorMessage(tr("The image could not be saved"), tr("Boundary plot"));
    }
}
