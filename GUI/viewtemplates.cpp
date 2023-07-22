

#include "viewtemplates.h"
#include "ui_viewtemplates.h"

using namespace std;
//using namespace tools;

ViewTemplates::ViewTemplates(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ViewTemplates)
{
    ui->setupUi(this);

    verTemplate = false;
    verContorno = true;
    color = false;
    setWindowTitle("Templates");


    cajaFrecuencias = new QGroupBox(this);
    cajaFrecuencias->setObjectName("cajaFrecuencias");
    cajaFrecuencias->setGeometry(QRect(660, 0, 141, 461));
    cajaFrecuencias->setTitle(QApplication::translate("ViewTemplates", "Frecuencias", 0));
}

ViewTemplates::~ViewTemplates()
{
    if (ejecutada){
        graContorno->clear();
        graTemplates->clear();
        ui->diagrama->clearFocus();
        ui->diagrama->clearGraphs();
        ui->diagrama->clearItems();
        ui->diagrama->clearPlottables();
        verTemplate = false;

        checkbox->clear();
        delete layoutColores;

        lineas->clear();
        sliders->clear();
    }

    if (color){
        colores->clear();
    }

    delete cajaFrecuencias;

    delete ui;
}

void ViewTemplates::setDatos(Controlador * controlador){

    if (color){
        colores->clear();
    }

    colores = new QMap <qreal, QColor> ();
    color = true;

    setTemplates(controlador->getTemplate());
    setContorno(controlador->getContorno());

    this->controlador = controlador;

    this->omega = controlador->getOmega()->getValores();

    this->epsilon = controlador->getEpsilon();

    for (qint32 i = 0; i < omega->size(); i++){
        colores->insert(omega->at(i), ramdonColor(i));
    }
}

void ViewTemplates::setTemplates(QVector<QVector<std::complex<qreal> > *> *templates){
    this->templates = templates;
}

void ViewTemplates::setContorno(QVector<QVector<std::complex<qreal> > *> *contorno){
    this->contorno = contorno;
}

void ViewTemplates::pintarGrafico(bool diagrama){



    this->diagrama = diagrama;

    if (ejecutada){
        graContorno->clear();
        graTemplates->clear();
        ui->diagrama->clearFocus();
        ui->diagrama->clearGraphs();
        ui->diagrama->clearItems();
        ui->diagrama->clearPlottables();
        verTemplate = false;
        for (qint32 i = 0; i < omega->size(); i++) {
            delete checkbox->at(i);
            delete lineas->at(i);
            delete sliders->at(i);
        }

        checkbox->clear();
        delete layoutColores;

        lineas->clear();
        sliders->clear();
    }

    graContorno = new QVector <QCPGraph *> ();
    graTemplates = new QVector <QCPGraph *> ();

    layoutColores = new QVBoxLayout (cajaFrecuencias);

    checkbox = new QVector <QCheckBox *> ();

    lineas = new QVector <QLineEdit *> ();
    sliders = new QVector <QSlider *> ();

    ejecutada = true;
    qint32 i = 0;
    qint32 contador = 0;

    if (templates == NULL)
        return;
    graTemplates->reserve(templates->size());

    if (contorno != NULL){
        graContorno->reserve(contorno->size());
        foreach (const QVector<std::complex<qreal> > * vector, *contorno) {

            QVector <qreal> * fas = new QVector <qreal>();
            fas->reserve(vector->size());
            QVector <qreal> * gan = new QVector <qreal> ();
            gan->reserve(vector->size());

            foreach (const std::complex <qreal> complejo, *vector) {

                if (diagrama){
                    qreal fase = arg(complejo)* 180 / M_PI;
                    if (fase >= 0){
                        fase -= 360;
                    }
                    fas->append(fase);
                    qreal mag = 20*log10(abs(complejo));
                    gan->append(mag);
                }else {
                    qreal fase = complejo.real();
                    fas->append(fase);
                    gan->append(complejo.imag());
                }
            }

            pintarLinea(i,graContorno,fas, gan, true, true,contador);



            fas->clear();
            gan->clear();
            i++;
            contador++;
        }

    }

    contador = 0;

    foreach (const QVector<std::complex<qreal> > * vector, *templates) {

        QVector <qreal> * fas = new QVector <qreal>();
        fas->reserve(vector->size());
        QVector <qreal> * gan = new QVector <qreal> ();
        gan->reserve(vector->size());

        foreach (const std::complex <qreal> complejo, *vector) {

            if (diagrama){
                qreal fase = arg(complejo)* 180 / M_PI;
                if (fase >= 0){
                    fase -= 360;
                }
                fas->append(fase);
                gan->append(20*log10(abs(complejo)));
            }else{
                qreal fase = complejo.real();
                fas->append(fase);
                gan->append(complejo.imag());
            }

        }

        pintarLinea(i,graTemplates, fas, gan, false, false, contador);
        fas->clear();
        gan->clear();
        i++;
        contador++;
    }

    cajaFrecuencias->setLayout(layoutColores);

    ui->diagrama->xAxis2->setVisible(true);
    ui->diagrama->xAxis2->setTickLabels(false);
    ui->diagrama->yAxis2->setVisible(true);
    ui->diagrama->yAxis2->setTickLabels(false);

    //ui->diagrama->legend->setVisible(true);
    //ui->diagrama->legend->setBrush(QColor(255, 255, 255, 150));

    connect(ui->diagrama->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->diagrama->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->diagrama->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->diagrama->yAxis2, SLOT(setRange(QCPRange)));

    ui->diagrama->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    ui->diagrama->replot();


    /////////////////////////////////////////////

    /*Natura_Interval_extension * conversion = new Natura_Interval_extension ();

    cinterval <qreal> caja = conversion->get_box(controlador->getPlanta(),omega->at(0));

    QPointF uno (caja.re.inf, caja.im.inf);
    QPointF dos (caja.re.inf, caja.im.sup);
    QPointF tres (caja.re.sup, caja.im.inf);
    QPointF cuatro (caja.re.sup, caja.im.sup);

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
    ui->diagrama->graph(i)->setData(ejex, ejey);

    ui->diagrama->graph(i)->setLineStyle(QCPGraph::lsLine);
    ui->diagrama->graph(i)->setScatterStyle(QCPScatterStyle::ssCross);

     ui->diagrama->replot();*/

    ////////////////////////////////////////////

}

void ViewTemplates::pintarLinea(qint32 pos, QVector <QCPGraph *> * guardar, QVector <qreal> * fas, QVector <qreal> * gan, bool tipo,
                                bool visible, qint32 contador){
    guardar->append(ui->diagrama->addGraph());
    ui->diagrama->graph(pos)->setData(*fas, *gan);

    if (tipo){
        ui->diagrama->graph(pos)->setScatterStyle(QCPScatterStyle::ssNone);
        ui->diagrama->graph(pos)->setLineStyle(QCPGraph::lsLine);
    }else{
        ui->diagrama->graph(pos)->setScatterStyle(QCPScatterStyle::ssCross);
        ui->diagrama->graph(pos)->setLineStyle(QCPGraph::lsNone);
    }
    QColor color;

    if (visible){
        color = colores->value(omega->at(contador));
        pintarCuadro(color, pos);
    }else{
        color = colores->value(omega->at(contador));
    }

    ui->diagrama->graph(pos)->setPen(color);
    ui->diagrama->graph(pos)->setVisible(visible);

    if (pos == 0){
        ui->diagrama->graph(pos)->rescaleAxes();
        return;
    }
    ui->diagrama->graph(pos)->rescaleAxes(true);

}

void ViewTemplates::pintarCuadro(QColor color, qint32 pos){

    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QCheckBox *check;
    QSlider *slider;
    QLineEdit *linea;

    widget = new QWidget(cajaFrecuencias);
    widget->setObjectName(QString::fromUtf8("widget"));
    widget->setGeometry(QRect(60, 70, 177, 58));

    QMetaObject::connectSlotsByName(widget);

    verticalLayout = new QVBoxLayout(widget);
    verticalLayout->setSpacing(6);
    verticalLayout->setContentsMargins(11, 11, 11, 11);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setSpacing(6);
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));

    check = new QCheckBox(widget);
    check->setObjectName(QString::fromUtf8("check"));
    check->setText(QString::number(omega->at(pos)));
    check->setStyleSheet("color : " + color.name());

    checkbox->append(check);
    check->setCheckState(Qt::Checked);
    horizontalLayout->addWidget(check);


    slider = new QSlider(widget);
    slider->setObjectName(QString::fromUtf8("slider"));
    slider->setOrientation(Qt::Horizontal);
    slider->setMaximum(epsilon->at(pos) * 10000);
    slider->setValue(epsilon->at(pos) * 1000);

    sliders->append(slider);
    horizontalLayout->addWidget(slider);


    verticalLayout->addLayout(horizontalLayout);

    linea = new QLineEdit(widget);
    linea->setObjectName(QString::fromUtf8("linea"));
    linea->setText(QString::number(epsilon->at(pos)));

    lineas->append(linea);
    verticalLayout->addWidget(linea);


    layoutColores->addWidget(widget);

    connect(slider, SIGNAL (sliderMoved (int)), this, SLOT (moverSliders ()));
    connect(check, SIGNAL (clicked()), this, SLOT (revisarCheckBox()));
}

void ViewTemplates::on_guardar_clicked()
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
            menerror("No se ha podido guardar la imagen", "Grafico Template");
    }
    delete extension;
}

void ViewTemplates::on_templates_clicked()
{
    verTemplate = !verTemplate;

    if (verTemplate)
        ui->templates->setText("Ocultar\nTemplates");
    else
        ui->templates->setText("Ver\nTemplates");

    foreach (QCPGraph * var, *graTemplates) {
        var->setVisible(verTemplate);
    }
    ui->diagrama->replot();
}

void ViewTemplates::on_contorno_clicked()
{
    verContorno = !verContorno;

    if (verContorno)
        ui->contorno->setText("Ocultar\nContorno");
    else
        ui->contorno->setText("Ver\nContorno");

    foreach (QCPGraph * var, *graContorno) {
        var->setVisible(verContorno);
    }
    ui->diagrama->replot();
}

void ViewTemplates::on_eContorno_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Guardar Fichero"));

    QFile fichero (fileName);
    QTextStream out (&fichero);

    if (!fichero.open(QIODevice::WriteOnly)){
        menerror("Gráfico Template", "No se pueden exportar los datos al fichero seleccionado");
    }

    //TODO como guardar numeros complejos...***

}


void ViewTemplates::moverSliders(){
    for (qint32 i = 0; i < sliders->size(); i++){
        lineas->at(i)->setText(QString::number(sliders->at(i)->value() / 1000));
    }
}

void ViewTemplates::revisarCheckBox(){
    for (qint32 i = 0; i < checkbox->size(); i++){
        if (checkbox->at(i)->checkState() == 0){
            graContorno->at(i)->setVisible(false);
        }else {
            graContorno->at(i)->setVisible(true);
        }
    }
    ui->diagrama->replot();
}

void ViewTemplates::on_recalcular_clicked()
{
    QVector <qreal> * epsilon = new QVector <qreal> ();

    for (qint32 i = 0; i < lineas->size(); i++) {
        qreal pos = lineas->at(i)->text().toDouble();
        sliders->at(i)->setValue(pos * 1000);
        epsilon->append(pos);
    }

    setContorno(controlador->recalcularContorno(epsilon));
    omega = controlador->getOmega()->getValores();
    this->epsilon->clear();
    this->epsilon = controlador->getEpsilon();
    pintarGrafico(diagrama);
}


void ViewTemplates::on_eTemplate_clicked()
{
 // TODO falta implementar la exportación numérica.
}

