/**
 * @file
 * @brief Draws the loop, the boundaries and the verdict of the design.
 *
 * The digits combo offers two to eight significant figures and the whole
 * double; it sets the global that everything drawn afterwards reads and
 * redraws the controller at those digits while the file keeps the rest.
 * The verdict is the controller checked against the specifications over
 * the templates, naming the worst frequency and specification, with the
 * whole table as tooltip; a design read from a file brings the verdict
 * without the table. Each boundary piece is a curve of its own. The
 * open-loop curve is drawn over a fixed dense logarithmic sweep, not the
 * range the form asks for, with phases folded into (-360, 0] and the curve
 * cut wherever the phase jumps by more than 100 degrees.
 */

#include "src/gui/common/qt_containers.h"
#include "src/core/math/constants.h"
#include "src/gui/common/plot_export.h"
#include "src/gui/common/number_text.h"
#include "src/gui/loopshaping/loop_shaping_viewer.h"
#include "ui_loop_shaping_viewer.h"

#include "src/gui/application/error_message.h"
#include <QComboBox>

#include "src/gui/common/field_mark.h"
#include "src/gui/common/plot_setup.h"
#include "src/gui/common/trace_segments.h"
#include "src/core/system/system_formula.h"

#include <cmath>

namespace qftbx {

LoopShapingViewer::LoopShapingViewer(QWidget *parent) :
    QWidget(parent),
    ui(std::make_unique<Ui::LoopShapingViewer>())
{
    ui->setupUi(this);
    qftbx::setUpPlot(*ui->plot, tr("phase (degrees)"), tr("magnitude (dB)"));
    setWindowTitle(tr("Loop Shaping"));

    ui->numeratorEdit->setReadOnly(true);
    ui->denominatorEdit->setReadOnly(true);
    ui->gainEdit->setReadOnly(true);

    legend = new FrequencyLegend(ui->legendHolder);
    ui->legendHolder->layout()->addWidget(legend);
    connect(legend, &FrequencyLegend::rowToggled, this, &LoopShapingViewer::applyCheckboxes);

    narrowSideColumn(ui->sideLayout);

    fillDigitsCombo();

    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->plot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->yAxis2, SLOT(setRange(QCPRange)));
}

LoopShapingViewer::~LoopShapingViewer()
{
    clearDiagram();

}

void LoopShapingViewer::fillDigitsCombo()
{
    for (int digits : {2, 3, 4, 5, 6, 8}) {
        ui->digitsCombo->addItem(QString::number(digits), digits);
    }
    ui->digitsCombo->addItem(tr("all"), 17);

    const int current = ui->digitsCombo->findData(shownDigits());
    ui->digitsCombo->setCurrentIndex(current >= 0 ? current : 2);

    connect(ui->digitsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                const int digits = ui->digitsCombo->itemData(index).toInt();
                setShownDigits(digits);
                if (loopShapingData != nullptr) {
                    showDiagram();
                }
                emit digitsChanged(digits);
            });
}

void LoopShapingViewer::clearDiagram(){

    if (!plotted){
        return;
    }

    ui->plot->clearFocus();
    ui->plot->clearGraphs();
    ui->plot->clearItems();
    ui->plot->clearPlottables();

    legend->clear();

    curves.clear();
    boundaryCurves.clear();

    plotted = false;
}

void LoopShapingViewer::clear(){

    clearDiagram();
    unionTraces.clear();
    omega = nullptr;
    plant = nullptr;
    loopShapingData = nullptr;
    ui->plot->replot();
}

void LoopShapingViewer::setData(const qftbx::UnionTraces & unionTraces, std::vector<double> *omega, LoopShapingResult *loopShapingData,
                               LtiSystem* plant, bool linSpace){
    this->unionTraces = unionTraces;
    this->omega = omega;
    this->loopShapingData = loopShapingData;
    this->plant = plant;
    this->linSpace = linSpace;
}

void LoopShapingViewer::showCheck(){

    const std::optional<qftbx::SpecificationCheck> & check = loopShapingData->check();

    if (!check.has_value()) {
        ui->checkLabel->setText(tr("Not checked against the specifications (no templates to check over)."));
        ui->checkLabel->setToolTip(QString());
        markAs(ui->checkLabel, "verdict", QString());
        return;
    }

    if (check->entries.empty()) {
        ui->checkLabel->setToolTip(QString());
        if (!std::isfinite(check->worstExcessDb)) {
            ui->checkLabel->setText(tr("No specification was active at any design frequency."));
            markAs(ui->checkLabel, "verdict", QString());
        } else if (check->satisfied()) {
            ui->checkLabel->setText(tr("Satisfies every specification over the template, by %1 dB.")
                                    .arg(qftbx::shownText(-check->worstExcessDb)));
            markAs(ui->checkLabel, "verdict", QStringLiteral("met"));
        } else {
            ui->checkLabel->setText(tr("EXCEEDS a specification over the template by %1 dB.")
                                    .arg(qftbx::shownText(check->worstExcessDb)));
            markAs(ui->checkLabel, "verdict", QStringLiteral("exceeded"));
        }
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
                                .arg(qftbx::shownText(worst->omega), name, qftbx::shownText(-worst->excessDb)));
        markAs(ui->checkLabel, "verdict", QStringLiteral("met"));
    } else {
        ui->checkLabel->setText(tr("EXCEEDS a specification over the template: w = %1 rad/s, %2, by %3 dB.")
                                .arg(qftbx::shownText(worst->omega), name, qftbx::shownText(worst->excessDb)));
        markAs(ui->checkLabel, "verdict", QStringLiteral("exceeded"));
    }

    QString table;
    for (const qftbx::SpecificationExcess & e : check->entries) {
        table += tr("w = %1: %2 = %3 dB, bound %4 dB, excess %5 dB\n")
                 .arg(qftbx::shownText(e.omega), specificationTitle(e.type),
                      qftbx::shownText(e.valueDb), qftbx::shownText(e.boundDb),
                      qftbx::shownText(e.excessDb));
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

void LoopShapingViewer::showController(){

    if (loopShapingData == nullptr || loopShapingData->controller() == nullptr) {
        return;
    }

    QString numerator = "", denominator = "";

    qint32 i = 0;
    for (i = 0; i < static_cast<qint32>(loopShapingData->controller()->numerator().size()); i++){
        numerator += qftbx::shownText(loopShapingData->controller()->numerator()[i].nominal()) + " ";
    }
    for (i = 0; i < static_cast<qint32>(loopShapingData->controller()->denominator().size()); i++){
        denominator += qftbx::shownText(loopShapingData->controller()->denominator()[i].nominal()) + " ";
    }

    ui->numeratorEdit->setText(numerator.trimmed());
    ui->denominatorEdit->setText(denominator.trimmed());
    ui->gainEdit->setText(qftbx::shownText(loopShapingData->controller()->gain().nominal()));

    ui->controllerFormula->setFormula(formulaOf(*loopShapingData->controller(), shownDigits()));

    showCheck();
}

void LoopShapingViewer::showDiagram(){

    showController();

    clearDiagram();

    plotted = true;

    qint32 curveIndex = 0;

    QVector <QColor> rowColors;

    qint32 frequencyIndex = 0;
    for (const qftbx::Trace & bound : unionTraces) {
        QColor color = frequencyColour(frequencyIndex, static_cast<int>(unionTraces.size()));
        frequencyIndex++;
        rowColors.push_back(color);

        QVector<QCPCurve *> pieces;

        for (const qftbx::Trace & piece : qftbx::continuousSegments(bound)) {
            std::vector<double> phases;
            std::vector<double> magnitudes;

            for (const qftbx::NicholsPoint & p : piece) {
                phases.push_back(p.phase);
                magnitudes.push_back(p.magnitude);
            }

            QCPCurve * curve = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
            curve->setData(qftbx::toQVector(phases), qftbx::toQVector(magnitudes));
            curve->setPen(QPen(color, kCurveWidth));

            if (piece.size() == 1) {
                curve->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));
            }

            curves.push_back(curve);
            pieces.push_back(curve);
        }

        boundaryCurves.push_back(pieces);

        addFrequencyRow(color, curveIndex);

        curveIndex++;
    }

    ui->plot->rescaleAxes();

    std::vector<double> frequencies;

    frequencies = qftbx::logspace(-5, 5, 10000);

    QVector<std::vector<double> > phaseSegments;
    QVector<std::vector<double> > magnitudeSegments;

    std::vector<double> currentPhases;
    std::vector<double> currentMagnitudes;

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
        curve->setPen(QPen(kLoopColour, kLoopWidth));
        curves.push_back(curve);
    }

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

    ui->plot->replot();
}

void LoopShapingViewer::applyCheckboxes(){
    for (qint32 i = 0; i < legend->rowCount() && i < boundaryCurves.size(); i++){
        for (QCPCurve * piece : boundaryCurves.at(i)) {
            piece->setVisible(legend->isRowChecked(i));
        }
    }
    ui->plot->replot();
}

void LoopShapingViewer::addFrequencyRow(QColor color, qint32 pos){
    legend->addRow(shownText(omega->at(pos)), color);
}

void LoopShapingViewer::on_saveImage_clicked()
{
    qftbx::exportPlot(this, *ui->plot, tr("Loop-shaping plot"));
}

}
