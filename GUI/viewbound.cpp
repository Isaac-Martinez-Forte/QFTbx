#include "viewbound.h"
#include "ui_viewbound.h"

#include "GUI/menerror.h"
#include "GUI/plot_palette.h"

using namespace tools;

ViewBound::ViewBound(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ViewBound)
{
    ui->setupUi(this);
    setWindowTitle("Boundaries");
    ejecutado = false;

    cajaFrecuencias = new QGroupBox(this);
    cajaFrecuencias->setObjectName("cajaFrecuencias");
    cajaFrecuencias->setGeometry(QRect(660, 0, 141, 461));

    //Ejes secundarios espejados: conectados UNA vez (cada repintado anadia
    //una conexion duplicada).
    connect(ui->diagrama->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->diagrama->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->diagrama->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->diagrama->yAxis2, SLOT(setRange(QCPRange)));
}

ViewBound::~ViewBound()
{
    clearDiagram();

    delete ui;
}

void ViewBound::clearDiagram(){

    if (!ejecutado){
        return;
    }

    ui->diagrama->clearFocus();
    ui->diagrama->clearGraphs();
    ui->diagrama->clearItems();
    //QCustomPlot es dueno de las curvas: clearPlottables las libera.
    ui->diagrama->clearPlottables();

    //Solo se liberan los CONTENEDORES de punteros (antes se fugaban) y las
    //filas de la caja de frecuencias enteras: borrar solo el checkbox dejaba
    //su widget contenedor acumulandose en el layout en cada repintado.
    foreach (QVector <QCPCurve * > * gra, *graficos) {
        delete gra;
    }
    delete graficos;
    graficos = nullptr;

    foreach (QCheckBox * che, *checkbox) {
        delete che->parentWidget();
    }
    delete checkbox;
    checkbox = nullptr;

    delete layoutColores;
    layoutColores = nullptr;

    ejecutado = false;
}

void ViewBound::setDatos(BoundaryData *datos, QVector <qreal> * omega){

    boundaries = datos;
    this->omega = omega;
}

void ViewBound::mostrarDiagrama(){

    qint32 k = 0;

    clearDiagram();

    layoutColores = new QVBoxLayout (cajaFrecuencias);
    checkbox = new QVector <QCheckBox *> ();
    ejecutado = true;

    graficos = new QVector <QVector <QCPCurve * > * > ();

    QVector <QMap <QString, QVector <QVector <QPointF> * > * > * > * boundaries = this->boundaries->boundaries();

    //Recorre las frecuencias de diseño.
    for (qint32 i = 0; i < boundaries->size(); i++) {

        QVector <QCPCurve * > * gra = new QVector <QCPCurve *> ();

        QColor color = ramdonColor(i);

        crearCuadro(color, i);

        QMap <QString, QVector <QVector <QPointF> * > * > * mapa = boundaries->at(i);
        foreach (QVector <QVector <QPointF> * > * b, *mapa) {
            foreach (QVector <QPointF> * bound, *b) {

                QVector <qreal> * ejex = new QVector <qreal> ();
                QVector <qreal> * ejey = new QVector <qreal> ();

                foreach (QPointF p, *bound) {
                   ejex->append(p.x());
                   ejey->append(p.y());
                }

                /*gra->append(ui->diagrama->addGraph());
                ui->diagrama->graph(k)->setData(*ejex, *ejey);
                ui->diagrama->graph(k)->setPen(color);
                ui->diagrama->graph(k)->setLineStyle(QCPGraph::lsNone);
                ui->diagrama->graph(k)->setScatterStyle(QCPScatterStyle::ssCircle);*/

                QCPCurve *curva = new QCPCurve(ui->diagrama->xAxis, ui->diagrama->yAxis);
                curva->setData(*ejex, *ejey);
                curva->setPen(color);
                gra->append(curva);

                delete ejex;
                delete ejey;
                k++;
            }
        }

        graficos->append(gra);
    }

    ui->diagrama->xAxis2->setVisible(true);
    ui->diagrama->xAxis2->setTickLabels(false);
    ui->diagrama->yAxis2->setVisible(true);
    ui->diagrama->yAxis2->setTickLabels(false);

    ui->diagrama->axisRect()->setupFullAxesBox();
    ui->diagrama->rescaleAxes();

    ui->diagrama->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    ui->diagrama->replot();

}

void ViewBound::crearCuadro(QColor color, qint32 pos){

    QWidget *widget;
    QCheckBox *checkBox;

    widget = new QWidget(cajaFrecuencias);
    widget->setObjectName("widget");
    widget->setGeometry(QRect(10, 10, 111, 23));
    checkBox = new QCheckBox(widget);
    checkBox->setObjectName("checkBox");

    QMetaObject::connectSlotsByName(widget);


    checkBox->setText(QString::number(omega->at(pos)));

    checkBox->setStyleSheet("color : " + color.name());

    layoutColores->addWidget(widget);
    checkbox->append(checkBox);
    checkBox->setCheckState(Qt::Checked);

    connect(checkBox, SIGNAL (clicked()), this, SLOT (revisarCheckBox()));
}

void ViewBound::revisarCheckBox(){
    for (qint32 i = 0; i < checkbox->size(); i++){
        if (checkbox->at(i)->checkState() == 0){

            for (qint32 j = 0; j < graficos->at(i)->size(); j++){
                graficos->at(i)->at(j)->setVisible(false);
            }
        }else {
            for (qint32 j = 0; j < graficos->at(i)->size(); j++){
                graficos->at(i)->at(j)->setVisible(true);
            }
        }
    }
    ui->diagrama->replot();
}

void ViewBound::on_exportar_clicked()
{
   /* QString fileName = QFileDialog::getSaveFileName(this, tr("Guardar Boundaries"));

    QVector <QString> nombres;
    nombres << "Tracking" << "Stability" << "SensorNoise" << "OutputDisturbance" << "InputDisturbance" << "ControlEffort";

    QVector <QMap <QString, QVector <QVector <QPoint> * > *> * > * boundaries = this->boundaries->boundaries();

    qint32 puntosFas = this->boundaries->phaseCount();
    qint32 puntosMag = this->boundaries->magnitudeCount();

    qint32 k = 0;*/

    //TODO Queda por hacer.
 /*   foreach (QVector <QVector <QVector <QPoint> * > * >* b, *boundaries) {
        for (qint32 i = 0; i < b->size(); i++){

            QVector <QVector <QPoint> * > * vectores = b->at(i);
            QFile fichero (fileName+"-" + nombres.at(k) + "-" + QString::number(i));
            QTextStream out (&fichero);

            if (!fichero.open(QIODevice::WriteOnly)){
                menerror("No se pueden exportar los datos al fichero seleccionado", "Gráfico Boundaries");
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

void ViewBound::on_guardar_clicked()
{
    bool noFallo = true;
    QString extension;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Guardar Fichero"),"",
                                                    tr((".png (*.png);;.pdf(*.pdf);; .jpg(*.jpg);; .bmp(*.bmp)")), &extension);
    if (!fileName.isEmpty()){
        if (extension.contains(".pdf", Qt::CaseInsensitive)){
            noFallo = ui->diagrama->savePdf(fileName, true);
        }else if (extension.contains(".png", Qt::CaseInsensitive)){
            noFallo = ui->diagrama->savePng(fileName);
        }else if (extension.contains(".jpg", Qt::CaseInsensitive)){
            noFallo = ui->diagrama->saveJpg(fileName);
        }else if (extension.contains(".bmp", Qt::CaseInsensitive)){
            noFallo = ui->diagrama->saveBmp(fileName);
        }else{
            noFallo = false;
        }

        if (!noFallo)
            menerror("No se ha podido guardar la imagen", "Grafico Boundaries");
    }
}
