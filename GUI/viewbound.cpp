#include "viewbound.h"
#include "ui_viewbound.h"

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
   // cajaFrecuencias->setTitle(QApplication::translate("GrafTemp", "Frecuencias", 0));
}

ViewBound::~ViewBound()
{
    if (ejecutado){
        graficos->clear();
        ui->diagrama->clearFocus();
        ui->diagrama->clearGraphs();
        ui->diagrama->clearItems();
        ui->diagrama->clearPlottables();
        foreach (QCheckBox * che, *checkbox) {
            delete che;
        }

        checkbox->clear();
        delete layoutColores;
    }

    delete ui;
}

void ViewBound::setDatos(std::shared_ptr<DatosBound> datos, QVector <qreal> * omega){

    boundaries = datos;
    this->omega = omega;
}

void ViewBound::mostrarDiagrama(){

    qint32 k = 0;

    if (ejecutado){
        graficos->clear();
        ui->diagrama->clearFocus();
        ui->diagrama->clearGraphs();
        ui->diagrama->clearItems();
        ui->diagrama->clearPlottables();
        foreach (QCheckBox * che, *checkbox) {
            delete che;
        }

        checkbox->clear();
        delete layoutColores;
    }

    layoutColores = new QVBoxLayout (cajaFrecuencias);
    checkbox = new QVector <QCheckBox *> ();
    ejecutado = true;

    graficos = new QVector <QVector <QCPCurve * > * > ();

    QVector <QMap <QString, QVector <QVector <QPointF> * > * > * > * boundaries = this->boundaries->getBoundaries();

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

                ejex->clear();
                ejey->clear();
                k++;
            }
        }

        graficos->append(gra);
    }

    ui->diagrama->xAxis2->setVisible(true);
    ui->diagrama->xAxis2->setTickLabels(false);
    ui->diagrama->yAxis2->setVisible(true);
    ui->diagrama->yAxis2->setTickLabels(false);

    connect(ui->diagrama->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->diagrama->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->diagrama->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->diagrama->yAxis2, SLOT(setRange(QCPRange)));
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
    nombres << "Seguimiento" << "Estabilidad" << "Ruido" << "RPS" << "RPE" << "EC";

    QVector <QMap <QString, QVector <QVector <QPoint> * > *> * > * boundaries = this->boundaries->getBoundaries();

    qint32 puntosFas = this->boundaries->getTamFas();
    qint32 puntosMag = this->boundaries->getTamMag();

    qint32 k = 0;*/

    //TODO Queda por hacer.
 /*   foreach (QVector <QVector <QVector <QPoint> * > * >* b, *boundaries) {
        for (qint32 i = 0; i < b->size(); i++){

            QVector <QVector <QPoint> * > * vectores = b->at(i);
            QFile fichero (fileName+"-" + nombres.at(k) + "-" + QString::number(i));
            QTextStream out (&fichero);

            if (!fichero.open(QIODevice::WriteOnly)){
                menerror("Gráfico Boundaries", "No se pueden exportar los datos al fichero seleccionado");
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
    QString * extension = new QString();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Guardar Fichero"),"",
                                                    tr((".png (*.png);;.pdf(*.pdf);; .jpg(*.jpg);; .bmp(*.bmp)")), extension);
    if (!fileName.isEmpty()){
        if (extension->contains(".pdf", Qt::CaseInsensitive)){
            noFallo = ui->diagrama->savePdf(fileName, true);
        }else if (extension->contains(".png", Qt::CaseInsensitive)){
            noFallo = ui->diagrama->savePng(fileName);
        }else if (extension->contains(".jpg", Qt::CaseInsensitive)){
            noFallo = ui->diagrama->saveJpg(fileName);
        }else if (extension->contains(".bmp", Qt::CaseInsensitive)){
            noFallo = ui->diagrama->saveBmp(fileName);
        }else{
            noFallo = false;
        }

        if (!noFallo)
            menerror("No se ha podido guardar la imagen", "Grafico Boundaries");
    }
    delete extension;
}
