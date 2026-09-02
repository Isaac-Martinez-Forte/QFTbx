#include "loop_boundaries_viewer.h"
#include "ui_loop_boundaries_viewer.h"

#include "GUI/error_message.h"
#include "GUI/plot_palette.h"


using namespace tools;
using namespace cxsc;

LoopBoundariesViewer::LoopBoundariesViewer(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::LoopBoundariesViewer>())
{
    ui->setupUi(this);
    setWindowTitle(tr("Boundary union"));
    plotted = false;


    frequenciesBox = new QGroupBox(this);
    frequenciesBox->setObjectName("frequenciesBox");
    frequenciesBox->setGeometry(QRect(660, 0, 141, 461));
    //frequenciesBox->setTitle(QApplication::translate("GrafTemp", "Frequencies", 0));
}

LoopBoundariesViewer::~LoopBoundariesViewer()
{
    clearDiagram();

}

void LoopBoundariesViewer::clearDiagram(){

    if (!plotted){
        return;
    }

    ui->plot->clearFocus();
    ui->plot->clearGraphs();
    ui->plot->clearItems();
    //QCustomPlot owns the curves: clearPlottables frees them.
    ui->plot->clearPlottables();

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


void LoopBoundariesViewer::setDatos(const BoundaryData *nicholsData, const BoundaryData *nyquistData, QVector<qreal> *omega,
                             LtiSystem *plant, LtiSystem *controller, bool nichols, bool nyquist){
    this->nicholsData = nicholsData;
    this->nyquistData = nyquistData;
    this->plant = plant;
    this->controller = controller;
    this->omega = omega;
    this->nichols = nichols;
    this->nyquist = nyquist;
}

void LoopBoundariesViewer::showDiagram(){


    bool mostrarNichols = nichols;
    bool mostrarNyquist = nyquist;


    clearDiagram();

    colorsLayout = new QVBoxLayout (frequenciesBox);
    checkboxes = new QVector <QCheckBox *> ();
    curves = new QVector <QCPCurve * > ();

    plotted = true;

    qint32 k = 0;
    qint32 contador = 0;

    QVector <QColor > * rowColors = new QVector <QColor> ();

    //Sweep the design frequencies.

    qint32 c = 0;
    for (const qftbx::Trace & boundNichols : nicholsData->unionBoundaries()) {

        const qftbx::Trace & boundNyquist =
                nyquistData->unionBoundaries().at(static_cast<std::size_t>(contador));


        QColor color = randomColor(c);
        c++;
        QColor color2 = randomColor(c);
        c++;

        rowColors->append(color);
        rowColors->append(color2);

        QVector <qreal> * ejex = new QVector <qreal> ();
        QVector <qreal> * ejey = new QVector <qreal> ();

        QVector <qreal> * ejex1 = new QVector <qreal> ();
        QVector <qreal> * ejey1 = new QVector <qreal> ();

        qint32 contador2 = 0;

        for (const QPointF & pNichols : boundNichols) {
            QPointF pNyquist = boundNyquist.at(static_cast<std::size_t>(contador2));
            ejex->append(pNichols.x());
            ejey->append(pNichols.y());

            ejex1->append(pNyquist.x());
            ejey1->append(pNyquist.y());


            contador2++;
        }

        if (mostrarNichols){
            QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
            curva->setData(*ejex, *ejey);
            curva->setPen(color);
            addFrequencyRow(color, contador);
            curves->append(curva);
            k++;
        }

        //The Nyquist-only mode drew nothing: the curve also required
        //mostrarNichols.
        if(mostrarNyquist){
            QCPCurve *curva2 = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
            curva2->setData(*ejex1, *ejey1);
            curva2->setPen(color2);
            addFrequencyRow(color2, contador);
            curves->append(curva2);
            k++;
        }

        delete ejex;
        delete ejey;
        delete ejex1;
        delete ejey1;

        contador++;
    }

    /*ui->plot->xAxis2->setVisible(true);
    ui->plot->xAxis2->setTickLabels(false);
    ui->plot->yAxis2->setVisible(true);
    ui->plot->yAxis2->setTickLabels(false);

    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->plot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->yAxis2, SLOT(setRange(QCPRange)));*/


    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui->plot->axisRect()->setupFullAxesBox();
    ui->plot->rescaleAxes();

    finalCurveIndex = k;

    delete rowColors;

    /*NaturalIntervalExtension * conversion = new NaturalIntervalExtension();

    contador = 0;

    foreach (qreal o, *omega) {

        cinterval <qreal> box = conversion->nicholsBox(controller,o, plant->evaluate(o), false);


        interval <qreal> b = abs(box);

        if (b.inf == 0){
            b.inf = 0.01;
        }

        interval<qreal> g = 20.0 * log10(b);


        interval<qreal> theta = arg(box) * (180 / M_PI);

        if (mostrarNichols){
            drawBox(QPointF(theta.inf, g.inf), QPointF(theta.inf, g.sup),
                           QPointF(theta.sup, g.inf), QPointF(theta.sup, g.sup), rowColors->at(contador));
        }

        contador++;


        if (mostrarNyquist){
            drawBox(QPointF(box.re.inf, box.im.inf), QPointF(box.re.inf, box.im.sup),
                           QPointF(box.re.sup, box.im.inf), QPointF(box.re.sup, box.im.sup), rowColors->at(contador));
        }

        contador++;
    }*/


    ui->plot->replot();
}

void LoopBoundariesViewer::applyCheckboxes(){
    for (qint32 i = 0; i < checkboxes->size(); i++){
        if (checkboxes->at(i)->checkState() == 0){
            curves->at(i)->setVisible(false);
        }else {
            curves->at(i)->setVisible(true);
        }
    }
    ui->plot->replot();
}

void LoopBoundariesViewer::addFrequencyRow(QColor color, qint32 pos){

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


void LoopBoundariesViewer::drawBox(QPointF uno, QPointF dos, QPointF tres, QPointF cuatro, QColor color){

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
    ui->plot->graph(finalCurveIndex)->setData(ejex, ejey);

    ui->plot->graph(finalCurveIndex)->setPen(color);
    ui->plot->graph(finalCurveIndex)->setLineStyle(QCPGraph::lsLine);
    ui->plot->graph(finalCurveIndex)->setScatterStyle(QCPScatterStyle::ssCircle);

    finalCurveIndex++;

    ui->plot->replot();
}

void LoopBoundariesViewer::on_saveImage_clicked()
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
