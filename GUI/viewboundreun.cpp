#include "viewboundreun.h"
#include "ui_viewboundreun.h"


using namespace tools;

ViewBoundReun::ViewBoundReun(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ViewBoundReun)
{
    ui->setupUi(this);
    setWindowTitle("Boundaries Reunidos");
    ejecutado = false;


    cajaFrecuencias = new QGroupBox(this);
    cajaFrecuencias->setObjectName("cajaFrecuencias");
    cajaFrecuencias->setGeometry(QRect(10, 120, 120, 451));
    graficos2 = NULL;
    graficos3 = NULL;
    //cajaFrecuencias->setTitle(QApplication::translate("GrafTemp", "Frecuencias", 0));
}

ViewBoundReun::~ViewBoundReun()
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
        col->clear();
    }

    if (graficos2 != NULL){
        graficos2->clear();
    }

    if (graficos3 != NULL){
        graficos3->clear();
    }

    delete ui;
}


void ViewBoundReun::setDatos(QVector<QVector<QPointF> *> *boun, QVector<qreal> *omega){
    this->boun = boun;
    this->omega = omega;
    hash = false;
}

void ViewBoundReun::setDatos(QVector<QVector<QPointF> *> *boun, QVector<qreal> *omega, qint32 unBoundarie){
    this->boun = boun;
    this->omega = omega;
    hash = false;
    this->unBoundarie = unBoundarie;
}

void ViewBoundReun::setDatos (QVector< QVector< QVector<QPointF> * > * > * boun, QVector<qreal> *omega) {
    bounHash = boun;
    this->omega = omega;
    hash = true;
}

void ViewBoundReun::setDatos (QVector< QVector< QVector<QPointF> * > * > * boun, QVector<qreal> *omega, QVector<QPointF> * b) {
    bounHash = boun;
    this->omega = omega;
    hash = true;
    this->b = b;
}

void ViewBoundReun::mostrar_diagrama(){

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
        col->clear();
    }

    layoutColores = new QVBoxLayout (cajaFrecuencias);
    checkbox = new QVector <QCheckBox *> ();
    graficos = new QVector <QCPCurve * > ();
    col = new QVector <QColor> ();


    ejecutado = true;

    qint32 k = 0;

    //Recorre las frecuencias de diseño.

    qint32 contador = 0;

    if (unBoundarie < 0) {

        if (!hash){
            foreach (QVector <QPointF> * bound, *boun) {
                QColor color = ramdonColor(contador);
                contador++;
                col->append(color);

                QVector <qreal> * ejex = new QVector <qreal> ();
                QVector <qreal> * ejey = new QVector <qreal> ();

                foreach (QPointF p, *bound) {
                    ejex->append(p.x());
                    ejey->append(p.y());
                }

                QCPCurve *curva = new QCPCurve(ui->diagrama->xAxis, ui->diagrama->yAxis);
                curva->setData(*ejex, *ejey);
                curva->setPen(color);
                graficos->append(curva);
                pintarCuadro(color, k);

                ejex->clear();
                ejey->clear();

                k++;
            }
        } else {

            if (this->b != nullptr){
                bounHash->removeLast();
            }

            foreach (QVector <QVector <QPointF> * > * bound, *bounHash) {
                QColor color = ramdonColor(contador);
                contador++;
                col->append(color);

                QVector <qreal> * ejex = new QVector <qreal> ();
                QVector <qreal> * ejey = new QVector <qreal> ();

                foreach (QVector <QPointF> * b, *bound) {
                    foreach (QPointF p, *b) {
                        ejex->append(p.x());
                        ejey->append(p.y());
                    }
                }

                QCPCurve *curva = new QCPCurve(ui->diagrama->xAxis, ui->diagrama->yAxis);
                curva->setData(*ejex, *ejey);
                curva->setPen(color);
                graficos->append(curva);

                pintarCuadro(color, k);

                ejex->clear();
                ejey->clear();

                k++;
            }
            if (this->b != nullptr){
                QColor color = ramdonColor(contador);
                contador++;
                col->append(color);

                QVector <qreal> * ejex = new QVector <qreal> ();
                QVector <qreal> * ejey = new QVector <qreal> ();

                foreach (QPointF p, *this->b) {
                    ejex->append(p.x());
                    ejey->append(p.y());
                }

                QCPCurve *curva = new QCPCurve(ui->diagrama->xAxis, ui->diagrama->yAxis);
                curva->setData(*ejex, *ejey);
                curva->setPen(color);
                graficos->append(curva);

                pintarCuadro(color, k);

                ejex->clear();
                ejey->clear();

                k++;
            }

        }
    } else {
        QColor color = ramdonColor(contador);
        contador++;
        col->append(color);

        QVector <qreal> * ejex = new QVector <qreal> ();
        QVector <qreal> * ejey = new QVector <qreal> ();

        foreach (QPointF p, *boun->at(unBoundarie)) {
            ejex->append(p.x());
            ejey->append(p.y());
        }

        QCPCurve *curva = new QCPCurve(ui->diagrama->xAxis, ui->diagrama->yAxis);
        curva->setData(*ejex, *ejey);
        curva->setPen(color);
        graficos->append(curva);
        pintarCuadro(color, k);

        ejex->clear();
        ejey->clear();

        k++;
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

    finalk = k++;

    ui->diagrama->replot();
}

void ViewBoundReun::revisarCheckBox(){
    for (qint32 i = 0; i < checkbox->size(); i++){
        if (checkbox->at(i)->checkState() == 0){
            graficos->at(i)->setVisible(false);
            if (graficos2 != nullptr){
                graficos2->at(i)->setVisible(false);
            }
            if (graficos3 != nullptr){
                graficos3->at(i)->setVisible(false);
            }
        }else {
            graficos->at(i)->setVisible(true);
            if (graficos2 != nullptr){
                graficos2->at(i)->setVisible(true);
            }
            if (graficos3 != nullptr){
                graficos3->at(i)->setVisible(true);
            }
        }
    }
    ui->diagrama->replot();
}

void ViewBoundReun::pintarCuadro(QColor color, qint32 pos){

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


void ViewBoundReun::dibujar_cuadro(QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, qint32 contador){

    if (graficos2 == NULL){
        graficos2 = new QVector <QCPCurve * > ();
    }

    QVector <qreal> ejex;
    QVector <qreal> ejey;

    ejex.append(uno.x());
    ejex.append(dos.x());
    ejex.append(tres.x());
    ejex.append(cuatro.x());
    ejex.append(uno.x());

    ejey.append(uno.y());
    ejey.append(dos.y());
    ejey.append(tres.y());
    ejey.append(cuatro.y());
    ejey.append(uno.y());

    /*graficos2->append(ui->diagrama->addGraph());
    ui->diagrama->graph(finalk)->setData(ejex, ejey);

    ui->diagrama->graph(finalk)->setPen(col->at(contador));
    ui->diagrama->graph(finalk)->setLineStyle(QCPGraph::lsLine);
    ui->diagrama->graph(finalk)->setScatterStyle(QCPScatterStyle::ssCross);
    ui->diagrama->graph(finalk)->rescaleAxes(true);*/

    QCPCurve *curva = new QCPCurve(ui->diagrama->xAxis, ui->diagrama->yAxis);
    curva->setData(ejex, ejey);
    curva->setPen(col->at(contador));
    graficos2->append(curva);

    //ui->diagrama->rescaleAxes(true);

    ui->diagrama->replot();
}

void ViewBoundReun::dibujar_cuadro2(QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, qint32 contador){

    if (graficos3 == NULL){
        graficos3 = new QVector <QCPCurve * > ();
    }
    QVector <qreal> ejex;
    QVector <qreal> ejey;

    ejex.append(uno.x());
    ejex.append(dos.x());
    ejex.append(tres.x());
    ejex.append(cuatro.x());
    ejex.append(uno.x());

    ejey.append(uno.y());
    ejey.append(dos.y());
    ejey.append(tres.y());
    ejey.append(cuatro.y());
    ejey.append(uno.y());

    /*graficos3->append(ui->diagrama->addGraph());
    ui->diagrama->graph(finalk)->setData(ejex, ejey);

    ui->diagrama->graph(finalk)->setPen(col->at(contador));
    ui->diagrama->graph(finalk)->setLineStyle(QCPGraph::lsLine);
    ui->diagrama->graph(finalk)->setScatterStyle(QCPScatterStyle::ssCircle);*/

    QCPCurve *curva = new QCPCurve(ui->diagrama->xAxis, ui->diagrama->yAxis);
    curva->setData(ejex, ejey);
    curva->setPen(col->at(contador));
    graficos3->append(curva);

    ui->diagrama->rescaleAxes(true);

    ui->diagrama->replot();
}

void ViewBoundReun::on_guardar_clicked()
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
