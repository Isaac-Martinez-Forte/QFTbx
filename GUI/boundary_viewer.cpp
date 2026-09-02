#include "boundary_viewer.h"
#include "ui_boundary_viewer.h"

#include "GUI/error_message.h"
#include "GUI/plot_palette.h"

using namespace tools;

BoundaryViewer::BoundaryViewer(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::BoundaryViewer>())
{
    ui->setupUi(this);
    setWindowTitle(tr("Boundaries"));
    plotted = false;

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
    foreach (QCheckBox * che, checkboxes) {
        delete che->parentWidget();
    }
    checkboxes.clear();

    //Also Qt's: a widget holds exactly one layout, so rebuilding the
    //frequency box means destroying the one it has.
    delete colorsLayout;
    colorsLayout = nullptr;

    plotted = false;
}

void BoundaryViewer::setDatos(const BoundaryData *datos, QVector <qreal> * omega){

    boundaryData = datos;
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

        QVector <QCPCurve *> gra;

        QColor color = randomColor(i);

        addFrequencyRow(color, i);

        const auto & mapa = boundarySet.at(static_cast<std::size_t>(i));
        for (const auto & entry : mapa) {
            const qftbx::TraceSet & b = entry.second;
            for (const qftbx::Trace & bound : b) {

                QVector <qreal> ejex;
                QVector <qreal> ejey;
                ejex.reserve(static_cast<qsizetype>(bound.size()));
                ejey.reserve(static_cast<qsizetype>(bound.size()));

                for (const QPointF & p : bound) {
                   ejex.append(p.x());
                   ejey.append(p.y());
                }

                /*gra->append(ui->plot->addGraph());
                ui->plot->graph(k)->setData(*ejex, *ejey);
                ui->plot->graph(k)->setPen(color);
                ui->plot->graph(k)->setLineStyle(QCPGraph::lsNone);
                ui->plot->graph(k)->setScatterStyle(QCPScatterStyle::ssCircle);*/

                QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
                curva->setData(ejex, ejey);
                curva->setPen(color);
                gra.append(curva);

                k++;
            }
        }

        curves.append(std::move(gra));
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

    QMetaObject::connectSlotsByName(widget);


    checkBox->setText(QString::number(omega->at(pos)));

    checkBox->setStyleSheet("color : " + color.name());

    colorsLayout->addWidget(widget);
    checkboxes.append(checkBox);
    checkBox->setCheckState(Qt::Checked);

    connect(checkBox, SIGNAL (clicked()), this, SLOT (applyCheckboxes()));
}

void BoundaryViewer::applyCheckboxes(){
    for (qint32 i = 0; i < checkboxes.size(); i++){
        if (checkboxes.at(i)->checkState() == 0){

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
