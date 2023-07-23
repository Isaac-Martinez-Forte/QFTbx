#include "viewloopshaping.h"
#include "ui_viewloopshaping.h"

using namespace tools;

ViewLoopShaping::ViewLoopShaping(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ViewLoopShaping)
{
    ui->setupUi(this);
    setWindowTitle("Loop Shaping");
    ejecutado = false;

    ui->nume->setReadOnly(true);
    ui->deno->setReadOnly(true);
    ui->k->setReadOnly(true);

    cajaFrecuencias = new QGroupBox(this);
    cajaFrecuencias->setObjectName("cajaFrecuencias");
    cajaFrecuencias->setGeometry(QRect(1060, 0, 120, 581));
    //cajaFrecuencias->setTitle(QApplication::translate("GrafTemp", "Frecuencias", 0));
}

ViewLoopShaping::~ViewLoopShaping()
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


void ViewLoopShaping::setDatos(QVector<QVector<QPointF> *> *boun, QVector<qreal> *omega, DatosLoopShaping *datos,
                               std::shared_ptr<Sistema> planta, bool linSpace){
    this->boun = boun;
    this->omega = omega;
    this->datos = datos;
    this->planta = planta;
    this->linSpace = linSpace;
}

void ViewLoopShaping::mostrar_diagrama(){

    QString numerador = "", denominador = "";

    qint32 i = 0;
    for (i = 0; i < datos->getControlador()->getNumerador()->size(); i++){
        numerador += QString::number(datos->getControlador()->getNumerador()->at(i)->getNominal()) + " ";
    }
    for (i = 0; i < datos->getControlador()->getDenominador()->size(); i++){
        denominador += QString::number(datos->getControlador()->getDenominador()->at(i)->getNominal()) + " ";
    }

    ui->nume->setText(numerador);
    ui->deno->setText(denominador);
    ui->k->setText(QString::number(datos->getControlador()->getK()->getNominal()));

    Sistema::tipo_planta tipo = datos->getControlador()->getClass();

    if (tipo == Sistema::cof_polinomios){
        QPixmap imagen (":/imagenes/copol.png");
        ui->tipoSistema->setPixmap(imagen);
    } else if (tipo == Sistema::k_ganancia){
        QPixmap imagen (":/imagenes/kgan.png");
        ui->tipoSistema->setPixmap(imagen);
    }else {
        QPixmap imagen (":/imagenes/knogan.png");
        ui->tipoSistema->setPixmap(imagen);
    }



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
    graficos = new QVector <QCPCurve * > ();


    ejecutado = true;

    qint32 k = 0;

    //Recorre los boundaries.

    QVector <QColor> colores;

    qint32 contador = 0;
    foreach (QVector <QPointF> * bound, *boun) {
        QColor color = ramdonColor(contador);
        contador++;
        colores.append(color);

        QVector <qreal> * ejex = new QVector <qreal> ();
        QVector <qreal> * ejey = new QVector <qreal> ();

        foreach (QPointF p, *bound) {
            ejex->append(p.x());
            ejey->append(p.y());
        }

        /*graficos->append(ui->diagrama->addGraph());
        ui->diagrama->graph(k)->setData(*ejex, *ejey);*/

        QCPCurve *curva = new QCPCurve(ui->diagrama->xAxis, ui->diagrama->yAxis);
        curva->setData(*ejex, *ejey);
        curva->setPen(color);
        graficos->append(curva);

        ejex->clear();
        ejey->clear();

        /*ui->diagrama->graph(k)->setPen(color);
        ui->diagrama->graph(k)->setLineStyle(QCPGraph::lsNone);
        ui->diagrama->graph(k)->setScatterStyle(QCPScatterStyle::ssCircle);*/
        pintarCuadro(color, k);

        k++;
    }

    ui->diagrama->rescaleAxes();

    //Dibujamos el lazo cerrado

    QVector <qreal> * frecuencias;

    /*if(linSpace){
        frecuencias = tools::linspace(datos->getRango().x(), datos->getRango().y(), datos->getNPuntos());

    } else {
        frecuencias = tools::logspace(datos->getRango().x(), datos->getRango().y(), datos->getNPuntos());
    }*/

    frecuencias = tools::logspace(-5, 5, 10000);

    QVector<QVector <qreal> *> * ejex = new QVector<QVector <qreal> *> ();
    QVector<QVector <qreal> *> * ejey = new QVector<QVector <qreal> *> ();

    QVector <qreal> * ejexActual = new QVector <qreal> ();
    QVector <qreal> * ejeyActual = new QVector <qreal> ();


    std::complex <qreal> c = planta->getPunto(frecuencias->at(0)) * datos->getControlador()->getPunto(frecuencias->at(0));

    qreal fas = arg(c) *180 / M_PI;
    qreal mag = 20*log10(abs(c));
    if (fas > 0)
        fas -= 360;

     ejexActual->append(fas);
     ejeyActual->append(mag);

     qreal puntoXAnterior = fas;


    foreach (qreal a, *frecuencias) {
        std::complex <qreal> c = planta->getPunto(a) * datos->getControlador()->getPunto(a);

        qreal fas = arg(c) *180 / M_PI;
        qreal mag = 20*log10(abs(c));
        if (fas > 0)
            fas -= 360;

        if (abs(fas - puntoXAnterior) < 100) {
            ejexActual->append(fas);
            ejeyActual->append(mag);
        } else {

            /*if (puntoXAnterior < -100){
                ejexActual->append(0);
                ejeyActual->append(puntoYAnterior);
            } else {
                ejexActual->append(-360);
                ejeyActual->append(puntoYAnterior);
            }*/

            ejex->append(ejexActual);
            ejey->append(ejeyActual);

            ejexActual = new QVector <qreal> ();
            ejeyActual = new QVector <qreal> ();

            /*if (fas > -100){
                ejexActual->append(0);
                ejeyActual->append(mag);
            } else {
                ejexActual->append(-360);
                ejeyActual->append(mag);
            }*/

            ejexActual->append(fas);
            ejeyActual->append(mag);
        }

        puntoXAnterior = fas;
    }

    ejex->append(ejexActual);
    ejey->append(ejeyActual);


    /*QCPGraph * gra = ui->diagrama->addGraph();
    gra->setData(*ejex, *ejey);

    gra->setPen(ramdonColor(contador));
    gra->setScatterStyle(QCPScatterStyle::ssCircle);
    gra->setLineStyle(QCPGraph::lsNone);*/

    for (qint32 i = 0; i < ejex->size(); i++){
        QCPCurve *curva = new QCPCurve(ui->diagrama->xAxis, ui->diagrama->yAxis);
        curva->setData(*ejex->at(i), *ejey->at(i));
        curva->setPen((QColor) Qt::black);
        graficos->append(curva);
    }


    ejex->clear();
    ejey->clear();


    //Dibujamos las cruces para cada frecuencia
    for (qint32 i = 0; i < omega->size(); i++){

        QVector <qreal> ejex;
        QVector <qreal> ejey;

        std::complex <qreal> c = datos->getControlador()->getPunto(omega->at(i)) * planta->getPunto(omega->at(i));
        ejey.append(20*log10(abs(c)));
        qreal fas = arg(c) *180 / M_PI;
        if (fas > 0)
            fas -= 360;
        ejex.append(fas);

        QCPGraph * gra = ui->diagrama->addGraph();
        gra->setData(ejex, ejey);

        gra->setPen(colores.at(i));
        gra->setScatterStyle(QCPScatterStyle::ssCircle);
        gra->setLineStyle(QCPGraph::lsNone);
    }

    ui->diagrama->xAxis2->setVisible(true);
    ui->diagrama->xAxis2->setTickLabels(false);
    ui->diagrama->yAxis2->setVisible(true);
    ui->diagrama->yAxis2->setTickLabels(false);

    connect(ui->diagrama->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->diagrama->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->diagrama->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->diagrama->yAxis2, SLOT(setRange(QCPRange)));

    ui->diagrama->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    ui->diagrama->replot();
}

void ViewLoopShaping::revisarCheckBox(){
    for (qint32 i = 0; i < checkbox->size(); i++){
        if (checkbox->at(i)->checkState() == 0){
            graficos->at(i)->setVisible(false);
        }else {
            graficos->at(i)->setVisible(true);
        }
    }
    ui->diagrama->replot();
}

void ViewLoopShaping::pintarCuadro(QColor color, qint32 pos){

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

void ViewLoopShaping::on_guardar_clicked()
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
