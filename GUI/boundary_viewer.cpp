#include "boundary_viewer.h"
#include "ui_boundary_viewer.h"

#include "GUI/error_message.h"
#include "GUI/plot_palette.h"

using namespace tools;

BoundaryViewer::BoundaryViewer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BoundaryViewer)
{
    ui->setupUi(this);
    setWindowTitle(tr("Boundaries"));
    plotted = false;

    frequenciesBox = new QGroupBox(this);
    frequenciesBox->setObjectName("frequenciesBox");
    frequenciesBox->setGeometry(QRect(660, 0, 141, 461));

    //Ejes secundarios espejados: conectados UNA vez (cada repintado anadia
    //una conexion duplicada).
    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->plot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->yAxis2, SLOT(setRange(QCPRange)));
}

BoundaryViewer::~BoundaryViewer()
{
    clearDiagram();

    delete ui;
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

    //Only the pointer CONTAINERS are freed here (they used to leak), plus
    //the whole frequency-box rows: deleting just the checkbox left its
    //container widget piling up in the layout on every replot.
    foreach (QVector <QCPCurve * > * gra, *curves) {
        delete gra;
    }
    delete curves;
    curves = nullptr;

    foreach (QCheckBox * che, *checkboxes) {
        delete che->parentWidget();
    }
    delete checkboxes;
    checkboxes = nullptr;

    delete colorsLayout;
    colorsLayout = nullptr;

    plotted = false;
}

void BoundaryViewer::setDatos(BoundaryData *datos, QVector <qreal> * omega){

    boundaryData = datos;
    this->omega = omega;
}

void BoundaryViewer::showDiagram(){

    qint32 k = 0;

    clearDiagram();

    colorsLayout = new QVBoxLayout (frequenciesBox);
    checkboxes = new QVector <QCheckBox *> ();
    plotted = true;

    curves = new QVector <QVector <QCPCurve * > * > ();

    QVector <QMap <QString, QVector <QVector <QPointF> * > * > * > * boundaryData = this->boundaryData->boundaries();

    //Sweep the design frequencies.
    for (qint32 i = 0; i < boundaryData->size(); i++) {

        QVector <QCPCurve * > * gra = new QVector <QCPCurve *> ();

        QColor color = randomColor(i);

        addFrequencyRow(color, i);

        QMap <QString, QVector <QVector <QPointF> * > * > * mapa = boundaryData->at(i);
        foreach (QVector <QVector <QPointF> * > * b, *mapa) {
            foreach (QVector <QPointF> * bound, *b) {

                QVector <qreal> * ejex = new QVector <qreal> ();
                QVector <qreal> * ejey = new QVector <qreal> ();

                foreach (QPointF p, *bound) {
                   ejex->append(p.x());
                   ejey->append(p.y());
                }

                /*gra->append(ui->plot->addGraph());
                ui->plot->graph(k)->setData(*ejex, *ejey);
                ui->plot->graph(k)->setPen(color);
                ui->plot->graph(k)->setLineStyle(QCPGraph::lsNone);
                ui->plot->graph(k)->setScatterStyle(QCPScatterStyle::ssCircle);*/

                QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
                curva->setData(*ejex, *ejey);
                curva->setPen(color);
                gra->append(curva);

                delete ejex;
                delete ejey;
                k++;
            }
        }

        curves->append(gra);
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
    checkboxes->append(checkBox);
    checkBox->setCheckState(Qt::Checked);

    connect(checkBox, SIGNAL (clicked()), this, SLOT (applyCheckboxes()));
}

void BoundaryViewer::applyCheckboxes(){
    for (qint32 i = 0; i < checkboxes->size(); i++){
        if (checkboxes->at(i)->checkState() == 0){

            for (qint32 j = 0; j < curves->at(i)->size(); j++){
                curves->at(i)->at(j)->setVisible(false);
            }
        }else {
            for (qint32 j = 0; j < curves->at(i)->size(); j++){
                curves->at(i)->at(j)->setVisible(true);
            }
        }
    }
    ui->plot->replot();
}

void BoundaryViewer::on_exportData_clicked()
{
   /* QString fileName = QFileDialog::getSaveFileName(this, tr("Save boundaryData"));

    QVector <QString> nombres;
    nombres << "Tracking" << "Stability" << "SensorNoise" << "OutputDisturbance" << "InputDisturbance" << "ControlEffort";

    QVector <QMap <QString, QVector <QVector <QPoint> * > *> * > * boundaryData = this->boundaryData->boundaries();

    qint32 puntosFas = this->boundaryData->phaseCount();
    qint32 puntosMag = this->boundaryData->magnitudeCount();

    qint32 k = 0;*/

    //TODO Queda por hacer.
 /*   foreach (QVector <QVector <QVector <QPoint> * > * >* b, *boundaryData) {
        for (qint32 i = 0; i < b->size(); i++){

            QVector <QVector <QPoint> * > * vectores = b->at(i);
            QFile fichero (fileName+"-" + nombres.at(k) + "-" + QString::number(i));
            QTextStream out (&fichero);

            if (!fichero.open(QIODevice::WriteOnly)){
                errorMessage(tr("The data cannot be exported to the chosen file"), tr("Boundary plot"));
                return;
            }

            for (qint32 j = 0; j < vectores->size(); j = j+2){

                QVector <QPoint> * vector = vectores->at(j);

                for (qint32 k = 0; k < vector->size(); k++){

                    out << vector->at(k).x() - puntosFas << " " << vector->at(k).y() - puntosMag << endl;

                }
                out << endl;
            }
        }
        k++;
    }*/

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
