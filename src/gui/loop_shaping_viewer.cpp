#include "qt_containers.h"
#include "src/gui/plot_export.h"
#include "src/gui/number_text.h"
#include "loop_shaping_viewer.h"
#include "ui_loop_shaping_viewer.h"

#include "src/gui/error_message.h"
#include "src/gui/plot_palette.h"

using namespace tools;

LoopShapingViewer::LoopShapingViewer(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::LoopShapingViewer>())
{
    ui->setupUi(this);
    setWindowTitle(tr("Loop Shaping"));

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

    //Qt's own mechanism: destroying the row widget is how a widget leaves
    //a layout, and it takes its checkbox with it. The row widgets used to
    //pile up on every replot.
    for (QCheckBox * che : checkboxes) {
        delete che->parentWidget();
    }
    checkboxes.clear();

    curves.clear();

    //Also Qt's: a widget holds exactly one layout, so rebuilding the
    //frequency box means destroying the one it has.
    delete colorsLayout;
    colorsLayout = nullptr;

    plotted = false;
}


void LoopShapingViewer::setData(const qftbx::UnionTraces & unionTraces, std::vector<double> *omega, LoopShapingResult *loopShapingData,
                               LtiSystem* plant, bool linSpace){
    this->unionTraces = unionTraces;
    this->omega = omega;
    this->loopShapingData = loopShapingData;
    this->plant = plant;
    this->linSpace = linSpace;
}

void LoopShapingViewer::showDiagram(){

    QString numerator = "", denominator = "";

    qint32 i = 0;
    for (i = 0; i < static_cast<qint32>(loopShapingData->controller()->numerator().size()); i++){
        numerator += tools::numberText(loopShapingData->controller()->numerator()[i].nominal()) + " ";
    }
    for (i = 0; i < static_cast<qint32>(loopShapingData->controller()->denominator().size()); i++){
        denominator += tools::numberText(loopShapingData->controller()->denominator()[i].nominal()) + " ";
    }

    ui->numeratorEdit->setText(numerator);
    ui->denominatorEdit->setText(denominator);
    ui->gainEdit->setText(tools::numberText(loopShapingData->controller()->gain().nominal()));

    const LtiSystem::SystemType type = loopShapingData->controller()->type();

    if (type == LtiSystem::SystemType::PolynomialForm){
        ui->systemTypeImage->setPixmap(QPixmap(":/figures/copol.png"));
    } else if (type == LtiSystem::SystemType::ZeroPoleGain){
        ui->systemTypeImage->setPixmap(QPixmap(":/figures/kgan.png"));
    }else {
        ui->systemTypeImage->setPixmap(QPixmap(":/figures/knogan.png"));
    }



    clearDiagram();

    colorsLayout = new QVBoxLayout (frequenciesBox);


    plotted = true;

    qint32 curveIndex = 0;

    //Sweep the boundaries.

    QVector <QColor> rowColors;

    qint32 frequencyIndex = 0;
    for (const qftbx::Trace & bound : unionTraces) {
        QColor color = randomColor(frequencyIndex);
        frequencyIndex++;
        rowColors.push_back(color);

        std::vector<double> ejex;
        std::vector<double> ejey;

        for (const qftbx::NicholsPoint & p : bound) {
            ejex.push_back(p.phase);
            ejey.push_back(p.magnitude);
        }

        QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
        curva->setData(tools::toQVector(ejex), tools::toQVector(ejey));
        curva->setPen(color);
        curves.push_back(curva);

        addFrequencyRow(color, curveIndex);

        curveIndex++;
    }

    ui->plot->rescaleAxes();

    //Draw the open-loop curve.

    std::vector<double> frequencies;

    /*if(linSpace){
        frequencies = tools::linspace(loopShapingData->range().min, loopShapingData->range().max, loopShapingData->pointCount());

    } else {
        frequencies = tools::logspace(loopShapingData->range().min, loopShapingData->range().max, loopShapingData->pointCount());
    }*/

    //FIXED ON PURPOSE, for now (decision taken 2026-09-03: leave it, write
    //down why). The dialog asks for a range and a point count, and nothing
    //reads them but the persistence. Of the three reasons that stood in the
    //way of honouring them, one is now gone and two remain.
    //
    //SETTLED: the units. Both dialogs ask for rad/s now and say so on the
    //label, and each converts with log10 where tools::logspace wants an
    //exponent. Before, this dialog's defaults were written as values while
    //the frequencies dialog read its field as an exponent, so the same "0.01"
    //meant two different frequencies and no label admitted it. Reviving the
    //code above therefore needs a std::log10 on both ends, exactly like
    //bode_viewer does.
    //
    //STILL IN THE WAY: the disabled code predates Range becoming a struct, so
    //it asks a QPointF for .min. And the segmentation below detects the phase
    //wrap by |delta| > 100 degrees, which PRESUMES a dense sweep: a small
    //user count would break the curve into spurious pieces.
    //
    //And that last one is why the point count should probably never be
    //honoured as typed. The answer is not a number of points but a
    //tolerance, and it is already solved next door for the computation:
    //NominalStabilityChecker derives its range from the design frequencies
    //(kDecadesBeyond) and refines until the phase step falls under
    //kMaxPhaseStepDegrees. Doing the same here would also make the drawing
    //and the check look at the same place, which they need not do today.
    frequencies = tools::logspace(-5, 5, 10000);

    //The open-loop curve, cut into segments wherever the phase wraps: by
    //value, so a replot does not abandon them (they all used to be).
    QVector<std::vector<double> > ejex;
    QVector<std::vector<double> > ejey;

    std::vector<double> ejexActual;
    std::vector<double> ejeyActual;


    //The first sample only seeds the phase comparison, so it is taken
    //inside the loop: it used to be computed before it as well, which put
    //the first frequency in the curve TWICE, and read frequencies.at(0)
    //without knowing there was one.
    qreal previousPhase = 0.0;
    bool firstSample = true;

    for (qreal a : frequencies) {
        std::complex <qreal> c = plant->evaluate(a) * loopShapingData->controller()->evaluate(a);

        qreal fas = arg(c) *180 / M_PI;
        qreal mag = 20*log10(abs(c));
        if (fas > 0)
            fas -= 360;

        if (firstSample || abs(fas - previousPhase) < 100) {
            ejexActual.push_back(fas);
            ejeyActual.push_back(mag);
        } else {

            ejex.push_back(std::move(ejexActual));
            ejey.push_back(std::move(ejeyActual));

            ejexActual = std::vector<double> ();
            ejeyActual = std::vector<double> ();

            ejexActual.push_back(fas);
            ejeyActual.push_back(mag);
        }

        previousPhase = fas;
        firstSample = false;
    }

    ejex.push_back(std::move(ejexActual));
    ejey.push_back(std::move(ejeyActual));


    for (qint32 i = 0; i < ejex.size(); i++){
        QCPCurve *curva = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
        curva->setData(tools::toQVector(ejex.at(i)), tools::toQVector(ejey.at(i)));
        curva->setPen((QColor) Qt::black);
        curves.push_back(curva);
    }



    //Draw the marker for each design frequency.
    for (qint32 i = 0; i < static_cast<std::int32_t>(omega->size()); i++){

        std::vector<double> ejex;
        std::vector<double> ejey;

        std::complex <qreal> c = loopShapingData->controller()->evaluate(omega->at(i)) * plant->evaluate(omega->at(i));
        ejey.push_back(20*log10(abs(c)));
        qreal fas = arg(c) *180 / M_PI;
        if (fas > 0)
            fas -= 360;
        ejex.push_back(fas);

        QCPGraph * gra = ui->plot->addGraph();
        gra->setData(tools::toQVector(ejex), tools::toQVector(ejey));

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
    for (qint32 i = 0; i < checkboxes.size(); i++){
        if (checkboxes.at(i)->checkState() == Qt::Unchecked){
            curves.at(i)->setVisible(false);
        }else {
            curves.at(i)->setVisible(true);
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

    checkBox->setText(tools::numberText(omega->at(pos)));

    checkBox->setStyleSheet("color : " + color.name());

    colorsLayout->addWidget(widget);
    checkboxes.push_back(checkBox);
    checkBox->setCheckState(Qt::Checked);

    connect(checkBox, SIGNAL (clicked()), this, SLOT (applyCheckboxes()));
}

void LoopShapingViewer::on_saveImage_clicked()
{
    tools::exportPlot(this, *ui->plot, tr("Loop-shaping plot"));
}
