
#include "Modelo/Herramientas/exception.h"
#include <QMessageBox>

#include "template_viewer.h"
#include "ui_template_viewer.h"

#include "GUI/error_message.h"
#include "GUI/plot_palette.h"

using namespace std;
//using namespace tools;

TemplateViewer::TemplateViewer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TemplateViewer)
{
    ui->setupUi(this);

    templatesVisible = false;
    contourVisible = true;
    colorsCreated = false;
    setWindowTitle(tr("Templates"));


    frequenciesBox = new QGroupBox(this);
    frequenciesBox->setObjectName("frequenciesBox");
    frequenciesBox->setGeometry(QRect(660, 0, 141, 461));
    frequenciesBox->setTitle(QApplication::translate("TemplateViewer", "Frequencies", 0));

    //Connected ONCE (every replot used to add a duplicated connection).
    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->plot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->yAxis2, SLOT(setRange(QCPRange)));
}

TemplateViewer::~TemplateViewer()
{
    clearDiagram();

    if (colorsCreated){
        delete colorByFrequency;
    }

    delete frequenciesBox;

    delete ui;
}

void TemplateViewer::clearDiagram(){

    if (!plotted){
        return;
    }

    ui->plot->clearFocus();
    ui->plot->clearGraphs();
    ui->plot->clearItems();
    //QCustomPlot owns the graphs: clearGraphs frees them.
    ui->plot->clearPlottables();
    templatesVisible = false;

    //Every frequency-box row is freed WHOLE through its container widget
    //(checkbox, slider and line edit are its children): the loose controls
    //used to be deleted while the containers piled up in the layout on
    //every replot. The pointer vectors leaked too.
    foreach (QCheckBox * che, *checkboxes) {
        delete che->parentWidget();
    }
    delete checkboxes;
    checkboxes = nullptr;
    delete epsilonEdits;
    epsilonEdits = nullptr;
    delete epsilonSliders;
    epsilonSliders = nullptr;

    delete contourGraphs;
    contourGraphs = nullptr;
    delete templateGraphs;
    templateGraphs = nullptr;

    delete colorsLayout;
    colorsLayout = nullptr;

    plotted = false;
}

void TemplateViewer::setDatos(Controlador * controller){

    if (colorsCreated){
        //delete, not clear(): the previous map leaked on every recompute.
        delete colorByFrequency;
    }

    colorByFrequency = new QMap <qreal, QColor> ();
    colorsCreated = true;

    setTemplates(controller->getTemplate());
    setContour(controller->getContorno());

    this->controller = controller;

    this->omega = controller->getOmega()->getValores();

    this->epsilon = controller->getEpsilon();

    for (qint32 i = 0; i < omega->size(); i++){
        colorByFrequency->insert(omega->at(i), randomColor(i));
    }
}

void TemplateViewer::setTemplates(QVector<QVector<std::complex<qreal> > *> *templatesButton){
    this->templatesButton = templatesButton;
}

void TemplateViewer::setContour(QVector<QVector<std::complex<qreal> > *> *contourButton){
    this->contourButton = contourButton;
}

void TemplateViewer::plotDiagram(bool plot){



    this->plot = plot;

    clearDiagram();

    contourGraphs = new QVector <QCPGraph *> ();
    templateGraphs = new QVector <QCPGraph *> ();

    colorsLayout = new QVBoxLayout (frequenciesBox);

    checkboxes = new QVector <QCheckBox *> ();

    epsilonEdits = new QVector <QLineEdit *> ();
    epsilonSliders = new QVector <QSlider *> ();

    plotted = true;
    qint32 i = 0;
    qint32 counter = 0;

    if (templatesButton == NULL)
        return;
    templateGraphs->reserve(templatesButton->size());

    if (contourButton != NULL){
        contourGraphs->reserve(contourButton->size());
        foreach (const QVector<std::complex<qreal> > * vector, *contourButton) {

            QVector <qreal> * fas = new QVector <qreal>();
            fas->reserve(vector->size());
            QVector <qreal> * gan = new QVector <qreal> ();
            gan->reserve(vector->size());

            foreach (const std::complex <qreal> complejo, *vector) {

                if (plot){
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

            plotLine(i,contourGraphs,fas, gan, true, true,counter);

            delete fas;
            delete gan;
            i++;
            counter++;
        }

    }

    counter = 0;

    foreach (const QVector<std::complex<qreal> > * vector, *templatesButton) {

        QVector <qreal> * fas = new QVector <qreal>();
        fas->reserve(vector->size());
        QVector <qreal> * gan = new QVector <qreal> ();
        gan->reserve(vector->size());

        foreach (const std::complex <qreal> complejo, *vector) {

            if (plot){
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

        plotLine(i,templateGraphs, fas, gan, false, false, counter);
        delete fas;
        delete gan;
        i++;
        counter++;
    }

    frequenciesBox->setLayout(colorsLayout);

    ui->plot->xAxis2->setVisible(true);
    ui->plot->xAxis2->setTickLabels(false);
    ui->plot->yAxis2->setVisible(true);
    ui->plot->yAxis2->setTickLabels(false);

    //ui->plot->legend->setVisible(true);
    //ui->plot->legend->setBrush(QColor(255, 255, 255, 150));

    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    ui->plot->replot();


    /////////////////////////////////////////////

    /*NaturalIntervalExtension * conversion = new NaturalIntervalExtension ();

    cinterval <qreal> caja = conversion->nicholsBox(controller->getPlanta(),omega->at(0));

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


    ui->plot->addGraph();
    ui->plot->graph(i)->setData(ejex, ejey);

    ui->plot->graph(i)->setLineStyle(QCPGraph::lsLine);
    ui->plot->graph(i)->setScatterStyle(QCPScatterStyle::ssCross);

     ui->plot->replot();*/

    ////////////////////////////////////////////

}

void TemplateViewer::plotLine(qint32 pos, QVector <QCPGraph *> * saveImage, QVector <qreal> * fas, QVector <qreal> * gan, bool tipo,
                                bool visible, qint32 counter){
    saveImage->append(ui->plot->addGraph());
    ui->plot->graph(pos)->setData(*fas, *gan);

    if (tipo){
        ui->plot->graph(pos)->setScatterStyle(QCPScatterStyle::ssNone);
        ui->plot->graph(pos)->setLineStyle(QCPGraph::lsLine);
    }else{
        ui->plot->graph(pos)->setScatterStyle(QCPScatterStyle::ssCross);
        ui->plot->graph(pos)->setLineStyle(QCPGraph::lsNone);
    }
    QColor colorsCreated;

    if (visible){
        colorsCreated = colorByFrequency->value(omega->at(counter));
        addFrequencyRow(colorsCreated, pos);
    }else{
        colorsCreated = colorByFrequency->value(omega->at(counter));
    }

    ui->plot->graph(pos)->setPen(colorsCreated);
    ui->plot->graph(pos)->setVisible(visible);

    if (pos == 0){
        ui->plot->graph(pos)->rescaleAxes();
        return;
    }
    ui->plot->graph(pos)->rescaleAxes(true);

}

void TemplateViewer::addFrequencyRow(QColor colorsCreated, qint32 pos){

    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QCheckBox *check;
    QSlider *slider;
    QLineEdit *linea;

    widget = new QWidget(frequenciesBox);
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
    check->setStyleSheet("colorsCreated : " + colorsCreated.name());

    checkboxes->append(check);
    check->setCheckState(Qt::Checked);
    horizontalLayout->addWidget(check);


    slider = new QSlider(widget);
    slider->setObjectName(QString::fromUtf8("slider"));
    slider->setOrientation(Qt::Horizontal);
    slider->setMaximum(epsilon->at(pos) * 10000);
    slider->setValue(epsilon->at(pos) * 1000);

    epsilonSliders->append(slider);
    horizontalLayout->addWidget(slider);


    verticalLayout->addLayout(horizontalLayout);

    linea = new QLineEdit(widget);
    linea->setObjectName(QString::fromUtf8("linea"));
    linea->setText(QString::number(epsilon->at(pos)));

    epsilonEdits->append(linea);
    verticalLayout->addWidget(linea);


    colorsLayout->addWidget(widget);

    connect(slider, SIGNAL (sliderMoved (int)), this, SLOT (syncSliders ()));
    connect(check, SIGNAL (clicked()), this, SLOT (applyCheckboxes()));
}

void TemplateViewer::on_saveImage_clicked()
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
            errorMessage(tr("The image could not be saved"), tr("Template plot"));
    }
}

void TemplateViewer::on_templatesButton_clicked()
{
    templatesVisible = !templatesVisible;

    if (templatesVisible)
        ui->templatesButton->setText(tr("Hide\ntemplates"));
    else
        ui->templatesButton->setText(tr("Show\ntemplates"));

    foreach (QCPGraph * parameter, *templateGraphs) {
        parameter->setVisible(templatesVisible);
    }
    ui->plot->replot();
}

void TemplateViewer::on_contourButton_clicked()
{
    contourVisible = !contourVisible;

    if (contourVisible)
        ui->contourButton->setText(tr("Hide\ncontour"));
    else
        ui->contourButton->setText(tr("Show\ncontour"));

    foreach (QCPGraph * parameter, *contourGraphs) {
        parameter->setVisible(contourVisible);
    }
    ui->plot->replot();
}

void TemplateViewer::on_exportContourButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save file"));

    QFile fichero (fileName);
    QTextStream out (&fichero);

    if (!fichero.open(QIODevice::WriteOnly)){
        errorMessage(tr("The data cannot be exported to the chosen file"), tr("Template plot"));
        return;
    }

    //TODO: how to store complex numbers...***

}


void TemplateViewer::syncSliders(){
    for (qint32 i = 0; i < epsilonSliders->size(); i++){
        epsilonEdits->at(i)->setText(QString::number(epsilonSliders->at(i)->value() / 1000.0));
    }
}

void TemplateViewer::applyCheckboxes(){
    for (qint32 i = 0; i < checkboxes->size(); i++){
        if (checkboxes->at(i)->checkState() == 0){
            contourGraphs->at(i)->setVisible(false);
        }else {
            contourGraphs->at(i)->setVisible(true);
        }
    }
    ui->plot->replot();
}

void TemplateViewer::on_recomputeButton_clicked()
{
    QVector <qreal> * epsilon = new QVector <qreal> ();

    for (qint32 i = 0; i < epsilonEdits->size(); i++) {
        qreal pos = epsilonEdits->at(i)->text().toDouble();
        epsilonSliders->at(i)->setValue(pos * 1000);
        epsilon->append(pos);
    }

    try {
        setContour(controller->recalcularContorno(epsilon));
    } catch (const qftbx::Exception & e) {
        QMessageBox::critical(this, tr("Template computation"), e.what());
        return;
    }
    omega = controller->getOmega()->getValores();
    //The previous epsilon is deleted by the DAO when accepting the new
    //one; touching it here would be a use-after-free.
    this->epsilon = controller->getEpsilon();
    plotDiagram(plot);
}


void TemplateViewer::on_exportTemplateButton_clicked()
{
 // TODO: numeric export not implemented.
}

