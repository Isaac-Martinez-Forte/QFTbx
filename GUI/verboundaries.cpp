#include "verboundaries.h"
#include "ui_verboundaries.h"


using namespace tools;
using namespace cxsc;

verBoundaries::verBoundaries(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::verBoundaries)
{
    ui->setupUi(this);
    setWindowTitle("Boundaries Reunidos");
    ejecutado = false;


    cajaFrecuencias = new QGroupBox(this);
    cajaFrecuencias->setObjectName("cajaFrecuencias");
    cajaFrecuencias->setGeometry(QRect(660, 0, 141, 461));
    //cajaFrecuencias->setTitle(QApplication::translate("GrafTemp", "Frecuencias", 0));
}

verBoundaries::~verBoundaries()
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


void verBoundaries::setDatos(std::shared_ptr<DatosBound> boundariesNichols, std::shared_ptr<DatosBound> boundariesNyquist, QVector<qreal> *omega,
                             std::shared_ptr<Sistema> planta, std::shared_ptr<Sistema> controlador, bool nichols, bool nyquist){
    this->boundariesNichols = boundariesNichols;
    this->boundariesNyquist = boundariesNyquist;
    this->planta = planta;
    this->controlador = controlador;
    this->omega = omega;
    this->nichols = nichols;
    this->nyquist = nyquist;
}

void verBoundaries::mostrar_diagrama(){


    bool mostrarNichols = nichols;
    bool mostrarNyquist = nyquist;


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
    qint32 contador = 0;

    QVector <QColor > * colores = new QVector <QColor> ();

    //Recorre las frecuencias de diseño.

    qint32 c = 0;
    foreach (QVector <QPointF> * boundNichols, *boundariesNichols->getBoundariesReun()) {

        QVector <QPointF> * boundNyquist = boundariesNyquist->getBoundariesReun()->at(contador);


        QColor color = ramdonColor(c);
        c++;
        QColor color2 = ramdonColor(c);
        c++;

        colores->append(color);
        colores->append(color2);

        QVector <qreal> * ejex = new QVector <qreal> ();
        QVector <qreal> * ejey = new QVector <qreal> ();

        QVector <qreal> * ejex1 = new QVector <qreal> ();
        QVector <qreal> * ejey1 = new QVector <qreal> ();

        qint32 contador2 = 0;

        foreach (QPointF pNichols, *boundNichols) {
            QPointF pNyquist = boundNyquist->at(contador2);
            ejex->append(pNichols.x());
            ejey->append(pNichols.y());

            ejex1->append(pNyquist.x());
            ejey1->append(pNyquist.y());


            contador2++;
        }

        if (mostrarNichols){
            QCPCurve *curva = new QCPCurve(ui->diagrama->xAxis, ui->diagrama->yAxis);
            curva->setData(*ejex, *ejey);
            curva->setPen(color);
            pintarCuadro(color, contador);
            graficos->append(curva);
            k++;
        }

        if(mostrarNyquist && mostrarNichols){
            QCPCurve *curva2 = new QCPCurve(ui->diagrama->xAxis, ui->diagrama->yAxis);
            curva2->setData(*ejex1, *ejey1);
            curva2->setPen(color2);
            pintarCuadro(color2, contador);
            graficos->append(curva2);
            k++;
        }

        ejex->clear();
        ejey->clear();
        ejex1->clear();
        ejey1->clear();

        contador++;
    }

    /*ui->diagrama->xAxis2->setVisible(true);
    ui->diagrama->xAxis2->setTickLabels(false);
    ui->diagrama->yAxis2->setVisible(true);
    ui->diagrama->yAxis2->setTickLabels(false);

    connect(ui->diagrama->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->diagrama->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->diagrama->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->diagrama->yAxis2, SLOT(setRange(QCPRange)));*/


    ui->diagrama->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui->diagrama->axisRect()->setupFullAxesBox();
    ui->diagrama->rescaleAxes();

    finalk = k;

    /*Natura_Interval_extension * conversion = new Natura_Interval_extension();

    contador = 0;

    foreach (qreal o, *omega) {

        cinterval <qreal> box = conversion->get_box(controlador,o, planta->getPunto(o), false);


        interval <qreal> b = abs(box);

        if (b.inf == 0){
            b.inf = 0.01;
        }

        interval<qreal> g = 20.0 * log10(b);


        interval<qreal> theta = arg(box) * (180 / M_PI);

        if (mostrarNichols){
            dibujar_cuadro(QPointF(theta.inf, g.inf), QPointF(theta.inf, g.sup),
                           QPointF(theta.sup, g.inf), QPointF(theta.sup, g.sup), colores->at(contador));
        }

        contador++;


        if (mostrarNyquist){
            dibujar_cuadro(QPointF(box.re.inf, box.im.inf), QPointF(box.re.inf, box.im.sup),
                           QPointF(box.re.sup, box.im.inf), QPointF(box.re.sup, box.im.sup), colores->at(contador));
        }

        contador++;
    }*/


    ui->diagrama->replot();
}

void verBoundaries::revisarCheckBox(){
    for (qint32 i = 0; i < checkbox->size(); i++){
        if (checkbox->at(i)->checkState() == 0){
            graficos->at(i)->setVisible(false);
        }else {
            graficos->at(i)->setVisible(true);
        }
    }
    ui->diagrama->replot();
}

void verBoundaries::pintarCuadro(QColor color, qint32 pos){

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


void verBoundaries::dibujar_cuadro(QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, QColor color){

    QVector <qreal> ejex;
    QVector <qreal> ejey;

    ejex.append(uno.x());
    ejex.append(dos.x());
    ejex.append(tres.x());
    ejex.append(cuatro.x());

    ejey.append(uno.y());
    ejey.append(dos.y());
    ejey.append(tres.y());
    ejey.append(cuatro.y());


    ui->diagrama->addGraph();
    ui->diagrama->graph(finalk)->setData(ejex, ejey);

    ui->diagrama->graph(finalk)->setPen(color);
    ui->diagrama->graph(finalk)->setLineStyle(QCPGraph::lsLine);
    ui->diagrama->graph(finalk)->setScatterStyle(QCPScatterStyle::ssCircle);

    finalk++;

    ui->diagrama->replot();
}

void verBoundaries::on_guardar_clicked()
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
