/**
 * @file
 * @brief Draws the loop, the boundaries and the verdict of the design.
 *
 * The digits combo offers two to eight significant figures and the whole
 * double; it sets the global that everything drawn afterwards reads and
 * redraws the controller at those digits while the file keeps the rest.
 * The verdict is one line cut to the width of the column, with the whole
 * sentence as tooltip, so it never takes the legend's room; Details asks
 * for the report, which holds the rest, and the line goes away while the
 * report is in sight beside the diagram. Each boundary piece is a curve of
 * its own. The open-loop curve is drawn over a fixed dense logarithmic
 * sweep, not the range the form asks for, with phases folded into
 * (-360, 0] and the curve cut wherever the phase jumps by more than 100
 * degrees.
 */

#include "src/gui/common/qt_containers.h"
#include "src/core/math/constants.h"
#include "src/gui/common/plot_export.h"
#include "src/gui/common/number_text.h"
#include "src/gui/loopshaping/loop_shaping_viewer.h"
#include "ui_loop_shaping_viewer.h"

#include "src/gui/application/error_message.h"
#include "src/gui/loopshaping/loop_shaping_report.h"
#include <QComboBox>
#include <QEvent>
#include <QToolButton>

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

    ui->checkLabel->installEventFilter(this);
    connect(ui->detailsButton, &QToolButton::clicked, this, &LoopShapingViewer::detailsAsked);

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
    summary.clear();
    ui->checkLabel->clear();
    ui->checkLabel->setToolTip(QString());
    markAs(ui->checkLabel, "verdict", QString());
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

    summary = LoopShapingReport::summaryOf(check);
    ui->checkLabel->setToolTip("<p>" + withPowersOfTen(LoopShapingReport::verdictOf(check)) + "</p>");
    markAs(ui->checkLabel, "verdict", LoopShapingReport::markOf(check));
    elideSummary();
}

void LoopShapingViewer::setSummaryShown(bool shown){
    ui->checkLabel->setVisible(shown);
    ui->detailsButton->setVisible(shown);
}

void LoopShapingViewer::elideSummary(){
    ui->checkLabel->setText(ui->checkLabel->fontMetrics().elidedText(summary, Qt::ElideRight,
                                                                      ui->checkLabel->width()));
}

bool LoopShapingViewer::eventFilter(QObject * watched, QEvent * event){
    if (watched == ui->checkLabel && event->type() == QEvent::Resize) {
        elideSummary();
    }
    return QWidget::eventFilter(watched, event);
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
