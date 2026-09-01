#include "loop_shaping_viewer.h"
#include "ui_loop_shaping_viewer.h"

#include "GUI/error_message.h"
#include "GUI/plot_palette.h"

using namespace tools;

LoopShapingViewer::LoopShapingViewer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoopShapingViewer)
{
    ui->setupUi(this);
    setWindowTitle(tr("Loop Shaping"));
    plotted = false;

    ui->numeratorEdit->setReadOnly(true);
    ui->denominatorEdit->setReadOnly(true);
    ui->gainEdit->setReadOnly(true);

    frequenciesBox = new QGroupBox(this);
    frequenciesBox->setObjectName("frequenciesBox");
    frequenciesBox->setGeometry(QRect(1060, 0, 120, 581));

    //Connected ONCE (every replot used to add a duplicated connection).
    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->plot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->yAxis2, SLOT(setRange(QCPRange)));
}

LoopShapingViewer::~LoopShapingViewer()
{
    clearDiagram();

    delete ui;
}

void LoopShapingViewer::clearDiagram(){

    if (!plotted){
        return;
    }

    ui->plot->clearFocus();
    ui->plot->clearGraphs();
    ui->plot->clearItems();
    //QCustomPlot owns the curves: clearPlottables frees them.
    ui->plot->clearPlottables();

    //Whole frequency-box rows and the pointer containers: the row widgets
    //used to pile up and the vectors leaked.
    foreach (QCheckBox * che, *checkboxes) {
        delete che->parentWidget();
    }
    delete checkboxes;
    checkboxes = nullptr;

    delete curves;
    curves = nullptr;

    delete colorsLayout;
    colorsLayout = nullptr;

    plotted = false;
}


void LoopShapingViewer::setDatos(const qftbx::UnionTraces & unionTraces, QVector<qreal> *omega, LoopShapingResult *loopShapingData,
                               LtiSystem* plant, bool linSpace){
    this->unionTraces = unionTraces;
    this->omega = omega;
    this->loopShapingData = loopShapingData;
    this->plant = plant;
    this->linSpace = linSpace;
}

void LoopShapingViewer::showDiagram(){

    QString numerador = "", denominador = "";

    qint32 i = 0;
    for (i = 0; i < static_cast<qint32>(loopShapingData->controller()->numerator().size()); i++){
        numerador += QString::number(loopShapingData->controller()->numerator()[i].nominal()) + " ";
    }
    for (i = 0; i < static_cast<qint32>(loopShapingData->controller()->denominator().size()); i++){
        denominador += QString::number(loopShapingData->controller()->denominator()[i].nominal()) + " ";
    }

    ui->numeratorEdit->setText(numerador);
    ui->denominatorEdit->setText(denominador);
    ui->gainEdit->setText(QString::number(loopShapingData->controller()->gain().nominal()));

    LtiSystem::SystemType tipo = loopShapingData->controller()->type();

    if (tipo == LtiSystem::SystemType::PolynomialForm){
        QPixmap imagen (":/figures/copol.png");
        ui->systemTypeImage->setPixmap(imagen);
    } else if (tipo == LtiSystem::SystemType::ZeroPoleGain){
        QPixmap imagen (":/figures/kgan.png");
        ui->systemTypeImage->setPixmap(imagen);
    }else {
        QPixmap imagen (":/figures/knogan.png");
        ui->systemTypeImage->setPixmap(imagen);
    }



    clearDiagram();

    colorsLayout = new QVBoxLayout (frequenciesBox);
    checkboxes = new QVector <QCheckBox *> ();
    curves = new QVector <QCPCurve * > ();


    plotted = true;

    qint32 gainEdit = 0;

    //Sweep the boundaries.

    QVector <QColor> rowColors;

    qint32 contador = 0;
    for (const qftbx::Trace & bound : unionTraces) {
        QColor color = randomColor(contador);
        contador++;
        rowColors.append(color);

        QVector <qreal> * ejex = new QVector <qreal> ();
        QVector <qreal> * ejey = new QVector <qreal> ();

        for (const QPointF & p : bound) {
            ejex->append(p.x());
            ejey->append(p.y());
        }

        /*curves->append(ui->plot->addGraph());
        ui->plot->graph(gainEdit)->setData(*ejex, *ejey);*/

        QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
        curva->setData(*ejex, *ejey);
        curva->setPen(color);
        curves->append(curva);

        delete ejex;
        delete ejey;

        /*ui->plot->graph(gainEdit)->setPen(color);
        ui->plot->graph(gainEdit)->setLineStyle(QCPGraph::lsNone);
        ui->plot->graph(gainEdit)->setScatterStyle(QCPScatterStyle::ssCircle);*/
        addFrequencyRow(color, gainEdit);

        gainEdit++;
    }

    ui->plot->rescaleAxes();

    //Draw the open-loop curve.

    QVector <qreal> frequencies;

    /*if(linSpace){
        frequencies = tools::linspace(loopShapingData->range().min, loopShapingData->range().max, loopShapingData->pointCount());

    } else {
        frequencies = tools::logspace(loopShapingData->range().min, loopShapingData->range().max, loopShapingData->pointCount());
    }*/

    frequencies = tools::logspace(-5, 5, 10000);

    QVector<QVector <qreal> *> * ejex = new QVector<QVector <qreal> *> ();
    QVector<QVector <qreal> *> * ejey = new QVector<QVector <qreal> *> ();

    QVector <qreal> * ejexActual = new QVector <qreal> ();
    QVector <qreal> * ejeyActual = new QVector <qreal> ();


    std::complex <qreal> c = plant->evaluate(frequencies.at(0)) * loopShapingData->controller()->evaluate(frequencies.at(0));

    qreal fas = arg(c) *180 / M_PI;
    qreal mag = 20*log10(abs(c));
    if (fas > 0)
        fas -= 360;

     ejexActual->append(fas);
     ejeyActual->append(mag);

     qreal previousPhase = fas;


    foreach (qreal a, frequencies) {
        std::complex <qreal> c = plant->evaluate(a) * loopShapingData->controller()->evaluate(a);

        qreal fas = arg(c) *180 / M_PI;
        qreal mag = 20*log10(abs(c));
        if (fas > 0)
            fas -= 360;

        if (abs(fas - previousPhase) < 100) {
            ejexActual->append(fas);
            ejeyActual->append(mag);
        } else {

            /*if (previousPhase < -100){
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

        previousPhase = fas;
    }

    ejex->append(ejexActual);
    ejey->append(ejeyActual);


    /*QCPGraph * gra = ui->plot->addGraph();
    gra->setData(*ejex, *ejey);

    gra->setPen(randomColor(contador));
    gra->setScatterStyle(QCPScatterStyle::ssCircle);
    gra->setLineStyle(QCPGraph::lsNone);*/

    for (qint32 i = 0; i < ejex->size(); i++){
        QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
        curva->setData(*ejex->at(i), *ejey->at(i));
        curva->setPen((QColor) Qt::black);
        curves->append(curva);
    }


    //The loop segments are already copied into the curves: the vectors are
    //freed (they all used to be abandoned on every replot), as is the sweep
    //frequency vector.
    foreach (QVector <qreal> * segment, *ejex) {
        delete segment;
    }
    delete ejex;
    foreach (QVector <qreal> * segment, *ejey) {
        delete segment;
    }
    delete ejey;


    //Draw the marker for each design frequency.
    for (qint32 i = 0; i < omega->size(); i++){

        QVector <qreal> ejex;
        QVector <qreal> ejey;

        std::complex <qreal> c = loopShapingData->controller()->evaluate(omega->at(i)) * plant->evaluate(omega->at(i));
        ejey.append(20*log10(abs(c)));
        qreal fas = arg(c) *180 / M_PI;
        if (fas > 0)
            fas -= 360;
        ejex.append(fas);

        QCPGraph * gra = ui->plot->addGraph();
        gra->setData(ejex, ejey);

        gra->setPen(rowColors.at(i));
        gra->setScatterStyle(QCPScatterStyle::ssCircle);
        gra->setLineStyle(QCPGraph::lsNone);
    }

    ui->plot->xAxis2->setVisible(true);
    ui->plot->xAxis2->setTickLabels(false);
    ui->plot->yAxis2->setVisible(true);
    ui->plot->yAxis2->setTickLabels(false);

    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    ui->plot->replot();
}

void LoopShapingViewer::applyCheckboxes(){
    for (qint32 i = 0; i < checkboxes->size(); i++){
        if (checkboxes->at(i)->checkState() == 0){
            curves->at(i)->setVisible(false);
        }else {
            curves->at(i)->setVisible(true);
        }
    }
    ui->plot->replot();
}

void LoopShapingViewer::addFrequencyRow(QColor color, qint32 pos){

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

void LoopShapingViewer::on_saveImage_clicked()
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
            errorMessage(tr("The image could not be saved"), tr("Loop-shaping plot"));
    }
}
