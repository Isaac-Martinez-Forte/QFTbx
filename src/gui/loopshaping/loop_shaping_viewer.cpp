#include "src/gui/common/qt_containers.h"
#include "src/core/math/constants.h"
#include "src/gui/common/plot_export.h"
#include "src/gui/common/number_text.h"
#include "src/gui/loopshaping/loop_shaping_viewer.h"
#include "ui_loop_shaping_viewer.h"

#include "src/gui/application/error_message.h"
#include "src/gui/common/plot_palette.h"


namespace qftbx {

LoopShapingViewer::LoopShapingViewer(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::LoopShapingViewer>())
{
    ui->setupUi(this);
    setWindowTitle(tr("Loop Shaping"));

    ui->numeratorEdit->setReadOnly(true);
    ui->denominatorEdit->setReadOnly(true);
    ui->gainEdit->setReadOnly(true);

    legend = new FrequencyLegend(this);
    legend->setGeometry(QRect(1060, 0, 120, 581));
    connect(legend, &FrequencyLegend::rowToggled, this, &LoopShapingViewer::applyCheckboxes);

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

    legend->clear();

    curves.clear();


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

//The controller checked against the specifications themselves, as the run
//left it: one line saying whether it satisfies them and by how much it
//misses when it does not, with the frequency and the specification of the
//worst case, and the whole table as the tooltip. The boundaries the search
//worked against are a discretisation; this is the answer to the question
//the user actually asked.
void LoopShapingViewer::showCheck(){

    const std::optional<qftbx::SpecificationCheck> & check = loopShapingData->check();

    if (!check.has_value() || check->entries.empty()) {
        ui->checkLabel->setText(tr("Not checked against the specifications (no templates to check over)."));
        ui->checkLabel->setToolTip(QString());
        ui->checkLabel->setStyleSheet(QString());
        return;
    }

    const qftbx::SpecificationExcess * worst = nullptr;
    for (const qftbx::SpecificationExcess & e : check->entries) {
        if (worst == nullptr || e.excessDb > worst->excessDb) {
            worst = &e;
        }
    }

    const QString name = specificationTitle(worst->type);

    if (check->satisfied()) {
        ui->checkLabel->setText(tr("Satisfies every specification over the template: tightest at w = %1 rad/s, %2, %3 dB of margin.")
                                .arg(qftbx::numberText(worst->omega), name, qftbx::numberText(-worst->excessDb)));
        ui->checkLabel->setStyleSheet("color: #1a7f37;");
    } else {
        ui->checkLabel->setText(tr("EXCEEDS a specification over the template: w = %1 rad/s, %2, by %3 dB.")
                                .arg(qftbx::numberText(worst->omega), name, qftbx::numberText(worst->excessDb)));
        ui->checkLabel->setStyleSheet("color: #b42318; font-weight: bold;");
    }

    QString table;
    for (const qftbx::SpecificationExcess & e : check->entries) {
        table += tr("w = %1: %2 = %3 dB, bound %4 dB, excess %5 dB\n")
                 .arg(qftbx::numberText(e.omega), specificationTitle(e.type),
                      qftbx::numberText(e.valueDb), qftbx::numberText(e.boundDb),
                      qftbx::numberText(e.excessDb));
    }
    ui->checkLabel->setToolTip(table.trimmed());
}

QString LoopShapingViewer::specificationTitle(qftbx::SpecificationType type){
    switch (type) {
    case qftbx::SpecificationType::TrackingLower:
    case qftbx::SpecificationType::TrackingUpper:     return tr("tracking");
    case qftbx::SpecificationType::Stability:         return tr("stability");
    case qftbx::SpecificationType::SensorNoise:       return tr("sensor noise");
    case qftbx::SpecificationType::OutputDisturbance: return tr("output disturbance");
    case qftbx::SpecificationType::InputDisturbance:  return tr("input disturbance");
    case qftbx::SpecificationType::ControlEffort:     return tr("control effort");
    }
    return QString();
}

void LoopShapingViewer::showDiagram(){

    QString numerator = "", denominator = "";

    qint32 i = 0;
    for (i = 0; i < static_cast<qint32>(loopShapingData->controller()->numerator().size()); i++){
        numerator += qftbx::numberText(loopShapingData->controller()->numerator()[i].nominal()) + " ";
    }
    for (i = 0; i < static_cast<qint32>(loopShapingData->controller()->denominator().size()); i++){
        denominator += qftbx::numberText(loopShapingData->controller()->denominator()[i].nominal()) + " ";
    }

    ui->numeratorEdit->setText(numerator);
    ui->denominatorEdit->setText(denominator);
    ui->gainEdit->setText(qftbx::numberText(loopShapingData->controller()->gain().nominal()));
    showCheck();

    const LtiSystem::SystemType type = loopShapingData->controller()->type();

    if (type == LtiSystem::SystemType::PolynomialForm){
        ui->systemTypeImage->setPixmap(QPixmap(":/figures/copol.png"));
    } else if (type == LtiSystem::SystemType::ZeroPoleGain){
        ui->systemTypeImage->setPixmap(QPixmap(":/figures/kgan.png"));
    }else {
        ui->systemTypeImage->setPixmap(QPixmap(":/figures/knogan.png"));
    }


    clearDiagram();



    plotted = true;

    qint32 curveIndex = 0;

    //Sweep the boundaries.

    QVector <QColor> rowColors;

    qint32 frequencyIndex = 0;
    for (const qftbx::Trace & bound : unionTraces) {
        QColor color = randomColor(frequencyIndex);
        frequencyIndex++;
        rowColors.push_back(color);

        std::vector<double> phases;
        std::vector<double> magnitudes;

        for (const qftbx::NicholsPoint & p : bound) {
            phases.push_back(p.phase);
            magnitudes.push_back(p.magnitude);
        }

        QCPCurve *curve = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
        curve->setData(qftbx::toQVector(phases), qftbx::toQVector(magnitudes));
        curve->setPen(color);
        curves.push_back(curve);

        addFrequencyRow(color, curveIndex);

        curveIndex++;
    }

    ui->plot->rescaleAxes();

    //Draw the open-loop curve.

    std::vector<double> frequencies;

    /*if(linSpace){
        frequencies = qftbx::linspace(loopShapingData->range().min, loopShapingData->range().max, loopShapingData->pointCount());

    } else {
        frequencies = qftbx::logspace(loopShapingData->range().min, loopShapingData->range().max, loopShapingData->pointCount());
    }*/

    //FIXED ON PURPOSE, for now (decision taken 2026-09-03: leave it, write
    //down why). The dialog asks for a range and a point count, and nothing
    //reads them but the persistence. Of the three reasons that stood in the
    //way of honouring them, one is now gone and two remain.
    //
    //SETTLED: the units. Both dialogs ask for rad/s now and say so on the
    //label, and each converts with log10 where qftbx::logspace wants an
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
    frequencies = qftbx::logspace(-5, 5, 10000);

    //The open-loop curve, cut into segments wherever the phase wraps: by
    //value, so a replot does not abandon them (they all used to be).
    QVector<std::vector<double> > phaseSegments;
    QVector<std::vector<double> > magnitudeSegments;

    std::vector<double> currentPhases;
    std::vector<double> currentMagnitudes;


    //The first sample only seeds the phase comparison, so it is taken
    //inside the loop: it used to be computed before it as well, which put
    //the first frequency in the curve TWICE, and read frequencies.at(0)
    //without knowing there was one.
    qreal previousPhase = 0.0;
    bool firstSample = true;

    for (qreal a : frequencies) {
        std::complex <qreal> c = plant->evaluate(a) * loopShapingData->controller()->evaluate(a);

        qreal phase = arg(c) *180 / qftbx::math::kPi;
        qreal magnitude = 20*log10(abs(c));
        if (phase > 0)
            phase -= 360;

        if (firstSample || abs(phase - previousPhase) < 100) {
            currentPhases.push_back(phase);
            currentMagnitudes.push_back(magnitude);
        } else {

            phaseSegments.push_back(std::move(currentPhases));
            magnitudeSegments.push_back(std::move(currentMagnitudes));

            currentPhases = std::vector<double> ();
            currentMagnitudes = std::vector<double> ();

            currentPhases.push_back(phase);
            currentMagnitudes.push_back(magnitude);
        }

        previousPhase = phase;
        firstSample = false;
    }

    phaseSegments.push_back(std::move(currentPhases));
    magnitudeSegments.push_back(std::move(currentMagnitudes));


    for (qint32 i = 0; i < phaseSegments.size(); i++){
        QCPCurve *curve = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
        curve->setData(qftbx::toQVector(phaseSegments.at(i)), qftbx::toQVector(magnitudeSegments.at(i)));
        curve->setPen((QColor) Qt::black);
        curves.push_back(curve);
    }


    //Draw the marker for each design frequency.
    for (qint32 i = 0; i < static_cast<std::int32_t>(omega->size()); i++){

        std::vector<double> phases;
        std::vector<double> magnitudes;

        std::complex <qreal> c = loopShapingData->controller()->evaluate(omega->at(i)) * plant->evaluate(omega->at(i));
        magnitudes.push_back(20*log10(abs(c)));
        qreal phase = arg(c) *180 / qftbx::math::kPi;
        if (phase > 0)
            phase -= 360;
        phases.push_back(phase);

        QCPGraph * marker = ui->plot->addGraph();
        marker->setData(qftbx::toQVector(phases), qftbx::toQVector(magnitudes));

        marker->setPen(rowColors.at(i));
        marker->setScatterStyle(QCPScatterStyle::ssCircle);
        marker->setLineStyle(QCPGraph::lsNone);
    }

    ui->plot->xAxis2->setVisible(true);
    ui->plot->xAxis2->setTickLabels(false);
    ui->plot->yAxis2->setVisible(true);
    ui->plot->yAxis2->setTickLabels(false);

    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    ui->plot->replot();
}

void LoopShapingViewer::applyCheckboxes(){
    for (qint32 i = 0; i < legend->rowCount(); i++){
        if (!legend->isRowChecked(i)){
            curves.at(i)->setVisible(false);
        }else {
            curves.at(i)->setVisible(true);
        }
    }
    ui->plot->replot();
}

void LoopShapingViewer::addFrequencyRow(QColor color, qint32 pos){
    legend->addRow(numberText(omega->at(pos)), color);
}

void LoopShapingViewer::on_saveImage_clicked()
{
    qftbx::exportPlot(this, *ui->plot, tr("Loop-shaping plot"));
}

} // namespace qftbx
