/**
 * @file
 * @brief Draws the clouds as points and the contours as curves.
 *
 * A value set is a set, its order that of the parameter sweep, so the
 * cloud is crosses with no line through them. A contour is a parametric
 * curve that keeps the order the walk found the border in; a graph would
 * sort by phase and comb the cloud. A cloud that epsilon does not hold
 * together is drawn as one curve per component, from the starts the
 * contour report gives; on the Nichols plane phases are folded into
 * (-360, 0). The side panel and the plot share a splitter so a long legend
 * row can be read by dragging. Contour state and proposals are marked
 * beside each epsilon field, never as a dialog, since trying an epsilon
 * and looking is how a contour is tuned.
 */

#include "src/core/common/exception.h"
#include "src/core/math/constants.h"
#include "src/gui/common/plot_export.h"
#include "src/gui/common/field_mark.h"
#include "src/gui/common/number_text.h"
#include <QMessageBox>
#include <QSplitter>

#include <algorithm>

#include "src/gui/common/qt_containers.h"
#include "src/gui/templates/template_viewer.h"
#include "ui_template_viewer.h"

#include "src/gui/application/error_message.h"
#include "src/gui/common/plot_setup.h"

using namespace std;

namespace qftbx {

namespace {

constexpr int kMinimumPlot = 320;

}

TemplateViewer::TemplateViewer(QWidget *parent) :
    QWidget(parent),
    ui(std::make_unique<Ui::TemplateViewer>())
{
    ui->setupUi(this);
    qftbx::setUpPlot(*ui->plot, tr("phase (degrees)"), tr("magnitude (dB)"));

    templatesVisible = false;
    contourVisible = true;
    setWindowTitle(tr("Templates"));

    legend = new FrequencyLegend(ui->legendHolder);
    ui->legendHolder->layout()->addWidget(legend);

    narrowSideColumn(ui->sideLayout);

    ui->legendHolder->setMaximumWidth(QWIDGETSIZE_MAX);

    QSplitter * splitter = new QSplitter(Qt::Horizontal, this);
    ui->sidePanel->setMinimumWidth(kSideColumn);
    ui->plot->setMinimumWidth(kMinimumPlot);
    splitter->addWidget(ui->sidePanel);
    splitter->addWidget(ui->plot);
    splitter->setChildrenCollapsible(false);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({kSideColumn, 3 * kSideColumn});
    ui->outerLayout->addWidget(splitter);

    connect(legend, &FrequencyLegend::rowToggled, this, &TemplateViewer::applyCheckboxes);

    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->plot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->yAxis2, SLOT(setRange(QCPRange)));
}

TemplateViewer::~TemplateViewer()
{
    clearDiagram();
}

void TemplateViewer::clearDiagram(){

    if (!plotted){
        return;
    }

    ui->plot->clearFocus();
    ui->plot->clearGraphs();
    ui->plot->clearItems();
    ui->plot->clearPlottables();
    templatesVisible = false;

    legend->clear();
    epsilonEdits.clear();
    gapLabels.clear();
    stateLabels.clear();

    contourCurves.clear();
    templateGraphs.clear();

    plotted = false;
}

void TemplateViewer::clear(){

    clearDiagram();
    m_templates.clear();
    m_contour.clear();
    m_omega.clear();
    m_epsilon.clear();
    ui->plot->replot();
}

void TemplateViewer::setData(const qftbx::CloudSet & templates,
                              const qftbx::CloudSet & contour,
                              std::vector<double> * omega,
                              std::vector<double> * epsilon){

    colorByFrequency.clear();

    setTemplates(templates);
    setContour(contour);

    m_omega = omega != nullptr ? *omega : std::vector<double>();
    m_epsilon = epsilon != nullptr ? *epsilon : std::vector<double>();

    for (qint32 i = 0; i < static_cast<std::int32_t>(m_omega.size()); i++){
        colorByFrequency.insert(m_omega.at(i), qftbx::frequencyColour(i, static_cast<int>(m_omega.size())));
    }
}

void TemplateViewer::setContourRecomputer(ContourRecomputer recompute){
    this->recompute = std::move(recompute);
}

void TemplateViewer::setEpsilonProposer(EpsilonProposer propose){
    this->propose = std::move(propose);
}

void TemplateViewer::setContourReporter(ContourReporter report){
    this->report = std::move(report);
}

void TemplateViewer::showContourState(){
    if (!report || stateLabels.empty()){
        return;
    }
    const std::vector<qftbx::TemplateEngine::ContourReport> reports = report();
    for (std::size_t i = 0; i < stateLabels.size(); ++i){
        const bool whole = i < reports.size() && reports[i].wholeCloud;
        const bool open = !whole && i < reports.size() && reports[i].relaxed;

        QString notice;
        QString explanation;

        if (whole) {
            notice = tr("no contour: whole template shown");
            explanation = tr("No contour closed at this epsilon, so the whole template stands "
                             "in for it here. A larger epsilon, or a denser sweep, closes it.");
        } else if (open) {
            notice = tr("open contour");
            explanation = tr("The epsilon-hull walk did not close at this epsilon and the "
                             "relaxed walk stood in for it: what is drawn covers the cloud but "
                             "is not the closed hull, so it ends where the walk ended. Propose "
                             "gives the epsilon that closes it.");
        }

        stateLabels[i]->setText(notice);
        stateLabels[i]->setVisible(!notice.isEmpty());
        stateLabels[i]->setToolTip(explanation);
        markAs(stateLabels[i], "notice", whole ? QStringLiteral("closed")
                                              : open ? QStringLiteral("coarse") : QString());
    }
}

void TemplateViewer::showProposals(){
    if (!propose || gapLabels.empty()){
        return;
    }
    m_proposals = propose();
    for (std::size_t i = 0; i < gapLabels.size() && i < m_proposals.size(); ++i){
        const qftbx::TemplateEngine::EpsilonProposal & p = m_proposals[i];
        const QString gap = QString::number(100.0 * p.coarseness(), 'f', 1);
        gapLabels[i]->setText(tr("needs %1 (gap %2%)").arg(shownText(p.epsilon), gap));
        gapLabels[i]->setVisible(true);
        gapLabels[i]->setToolTip(tr("The least epsilon at which this template's contour closes is %1 (it is connected from %2); the largest gap between its points is %3% of its size. Above a few per cent the sweep is coarse: more points per parameter, not a larger epsilon.")
                                 .arg(numberText(p.epsilon), numberText(p.connected), gap));
        markAs(gapLabels[i], "notice",
               p.coarseness() > 0.05 ? QStringLiteral("coarse") : QString());
    }
}

void TemplateViewer::refreshContour(const qftbx::CloudSet & contour,
                                    std::vector<double> * omega,
                                    std::vector<double> * epsilon){
    setContour(contour);

    m_omega = omega != nullptr ? *omega : std::vector<double>();
    m_epsilon = epsilon != nullptr ? *epsilon : std::vector<double>();

    plotDiagram(plot);
}

void TemplateViewer::setTemplates(const qftbx::CloudSet & templates){
    m_templates = templates;
}

void TemplateViewer::setContour(const qftbx::CloudSet & contour){
    m_contour = contour;
}

void TemplateViewer::plotDiagram(bool plot){

    this->plot = plot;

    clearDiagram();

    plotted = true;
    showProposals();
    showContourState();
    qint32 i = 0;
    qint32 counter = 0;

    if (m_templates.empty())
        return;
    templateGraphs.reserve(static_cast<qint32>(m_templates.size()));

    if (!m_contour.empty()){
        contourCurves.reserve(static_cast<qint32>(m_contour.size()));
        for (const qftbx::ComplexCloud & vector : m_contour) {

            std::vector<double> phases;
            phases.reserve(static_cast<qint32>(vector.size()));
            std::vector<double> magnitudes;
            magnitudes.reserve(static_cast<qint32>(vector.size()));

            for (const std::complex <qreal> & value : vector) {

                if (plot){
                    qreal phase = arg(value)* 180 / qftbx::math::kPi;
                    if (phase >= 0){
                        phase -= 360;
                    }
                    phases.push_back(phase);
                    qreal magnitude = 20*log10(abs(value));
                    magnitudes.push_back(magnitude);
                }else {
                    qreal phase = value.real();
                    phases.push_back(phase);
                    magnitudes.push_back(value.imag());
                }
            }

            plotContour(phases, magnitudes, counter);

            i++;
            counter++;
        }

    }

    counter = 0;

    for (const qftbx::ComplexCloud & vector : m_templates) {

        std::vector<double> phases;
        phases.reserve(static_cast<qint32>(vector.size()));
        std::vector<double> magnitudes;
        magnitudes.reserve(static_cast<qint32>(vector.size()));

        for (const std::complex <qreal> & value : vector) {

            if (plot){
                qreal phase = arg(value)* 180 / qftbx::math::kPi;
                if (phase >= 0){
                    phase -= 360;
                }
                phases.push_back(phase);
                magnitudes.push_back(20*log10(abs(value)));
            }else{
                qreal phase = value.real();
                phases.push_back(phase);
                magnitudes.push_back(value.imag());
            }

        }

        plotCloud(phases, magnitudes, counter);
        i++;
        counter++;
    }

    ui->plot->replot();

}

void TemplateViewer::plotCloud(const std::vector<double> & phases,
                               const std::vector<double> & magnitudes, qint32 frequency)
{
    QCPGraph * cloud = ui->plot->addGraph();
    cloud->setData(qftbx::toQVector(phases), qftbx::toQVector(magnitudes));
    cloud->setScatterStyle(QCPScatterStyle::ssCross);
    cloud->setLineStyle(QCPGraph::lsNone);

    const QColor color = colorByFrequency.value(m_omega.at(frequency));
    cloud->setPen(QPen(color, kCurveWidth));
    cloud->setVisible(templatesVisible);

    templateGraphs.push_back(cloud);
}

void TemplateViewer::plotContour(const std::vector<double> & phases,
                                 const std::vector<double> & magnitudes, qint32 frequency)
{
    const QColor color = colorByFrequency.value(m_omega.at(frequency));

    std::vector<std::size_t> starts{0};
    if (report) {
        const std::vector<qftbx::TemplateEngine::ContourReport> reports = report();
        if (static_cast<std::size_t>(frequency) < reports.size()
                && reports[frequency].componentStarts.size() > 1) {
            starts = reports[frequency].componentStarts;
        }
    }

    QVector<QCPCurve *> pieces;

    for (std::size_t piece = 0; piece < starts.size(); ++piece) {
        const std::size_t from = starts[piece];
        const std::size_t to = piece + 1 < starts.size() ? starts[piece + 1] : phases.size();

        if (from >= to || to > phases.size()) {
            continue;
        }

        QCPCurve * contour = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
        contour->setData(qftbx::toQVector(std::vector<double>(phases.begin() + std::ptrdiff_t(from),
                                                              phases.begin() + std::ptrdiff_t(to))),
                         qftbx::toQVector(std::vector<double>(magnitudes.begin() + std::ptrdiff_t(from),
                                                              magnitudes.begin() + std::ptrdiff_t(to))));
        contour->setPen(QPen(color, kCurveWidth));
        contour->setVisible(contourVisible);

        pieces.push_back(contour);
    }

    contourCurves.push_back(pieces);

    addFrequencyRow(color, frequency);

    if (frequency == 0) {
        ui->plot->rescaleAxes();
    } else {
        ui->plot->rescaleAxes(true);
    }
}

void TemplateViewer::addFrequencyRow(QColor color, qint32 pos){
    const FrequencyLegend::Row row = legend->addRow(numberText(m_omega.at(pos)), color);

    const bool known = pos < static_cast<qint32>(m_epsilon.size());
    const double epsilon = known ? m_epsilon.at(pos) : 0.0;

    QLineEdit * field = new QLineEdit(row.widget);
    field->setObjectName(QString::fromUtf8("field"));
    field->setToolTip(tr("The epsilon of this frequency: the diameter of the hull the contour "
                         "of this template is walked with. Recompute walks the contours again "
                         "with it."));
    field->setText(known ? numberText(epsilon) : QString());
    epsilonEdits.push_back(field);
    row.layout->addWidget(field);

    QLabel * gap = new QLabel(row.widget);
    gap->setObjectName(QString::fromUtf8("gap"));
    gap->setVisible(false);
    gapLabels.push_back(gap);
    row.column->addWidget(gap);

    QLabel * state = new QLabel(row.widget);
    state->setObjectName(QString::fromUtf8("contourState"));
    state->setVisible(false);
    stateLabels.push_back(state);
    row.column->addWidget(state);
}

void TemplateViewer::on_saveImage_clicked()
{
    qftbx::exportPlot(this, *ui->plot, tr("Template plot"));
}

void TemplateViewer::on_templatesButton_clicked()
{
    templatesVisible = !templatesVisible;

    if (templatesVisible)
        ui->templatesButton->setText(tr("Hide\ntemplates"));
    else
        ui->templatesButton->setText(tr("Show\ntemplates"));

    for (QCPGraph * parameter : templateGraphs) {
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

    for (const QVector<QCPCurve *> & pieces : contourCurves) {
        for (QCPCurve * contour : pieces) {
            contour->setVisible(contourVisible);
        }
    }
    ui->plot->replot();
}

void TemplateViewer::applyCheckboxes(){
    for (qint32 i = 0; i < legend->rowCount(); i++){
        const bool shown = legend->isRowChecked(i);

        if (i < contourCurves.size()) {
            for (QCPCurve * contour : contourCurves.at(i)) {
                contour->setVisible(shown && contourVisible);
            }
        }
        if (i < templateGraphs.size()) {
            templateGraphs.at(i)->setVisible(shown && templatesVisible);
        }
    }
    ui->plot->replot();
}

void TemplateViewer::on_proposeButton_clicked()
{
    if (!plotted || !propose){
        return;
    }
    m_proposals = propose();
    for (qint32 i = 0; i < epsilonEdits.size() && i < static_cast<qint32>(m_proposals.size()); i++) {
        const double value = m_proposals[static_cast<std::size_t>(i)].epsilon;
        epsilonEdits.at(i)->setText(numberText(value));
    }
}

void TemplateViewer::on_recomputeButton_clicked()
{
    if (!plotted){
        return;
    }

    std::vector<double> epsilon;
    epsilon.reserve(epsilonEdits.size());

    for (qint32 i = 0; i < epsilonEdits.size(); i++) {
        qreal pos = epsilonEdits.at(i)->text().toDouble();
        epsilon.push_back(pos);
    }

    if (!recompute){
        return;
    }

    recompute(std::move(epsilon));
}

}
