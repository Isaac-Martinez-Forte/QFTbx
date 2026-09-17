
#include "src/core/common/exception.h"
#include "src/core/math/constants.h"
#include "src/gui/common/plot_export.h"
#include "src/gui/common/field_mark.h"
#include "src/gui/common/number_text.h"
#include <QMessageBox>

#include <algorithm>

#include "src/gui/common/qt_containers.h"
#include "src/gui/templates/template_viewer.h"
#include "ui_template_viewer.h"

#include "src/gui/application/error_message.h"
#include "src/gui/common/plot_setup.h"

using namespace std;

namespace qftbx {

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
    connect(legend, &FrequencyLegend::rowToggled, this, &TemplateViewer::applyCheckboxes);

    //Connected ONCE: a connection per replot duplicates the handler.
    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->plot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->yAxis2, SLOT(setRange(QCPRange)));
}

TemplateViewer::~TemplateViewer()
{
    //The frequency box is parented to this widget, so Qt frees it, and the
    //colour map is a member: there is nothing left to free by hand.
    clearDiagram();
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

    legend->clear();
    epsilonEdits.clear();
    epsilonSliders.clear();
    gapLabels.clear();
    stateLabels.clear();

    contourGraphs.clear();
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

    //COPIES: the viewer outlives the project's vectors across a load. A
    //project may hold templates and no epsilon - a file that carries the
    //clouds and not the tolerance they were walked with - and asking for
    //the diagram must not be a way of reading a null.
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

//The frequencies where no contour closed and the whole template stands in
//for it: marked next to the epsilon, never as a dialog, since trying an
//epsilon and looking is how the contour is tuned.
void TemplateViewer::showContourState(){
    if (!report || stateLabels.empty()){
        return;
    }
    const std::vector<qftbx::TemplateEngine::ContourReport> reports = report();
    for (std::size_t i = 0; i < stateLabels.size(); ++i){
        const bool whole = i < reports.size() && reports[i].wholeCloud;
        stateLabels[i]->setText(whole ? tr("no contour: whole template shown") : QString());
        stateLabels[i]->setVisible(whole);
        stateLabels[i]->setToolTip(whole ? tr("No contour closed at this epsilon, so the whole template stands in for it here. A larger epsilon, or a denser sweep, closes it.") : QString());
        markAs(stateLabels[i], "notice", whole ? QStringLiteral("closed") : QString());
    }
}

//The epsilon each template asks for, next to the epsilon it has: the least
//that keeps the cloud connected, and how big that gap is against the
//template, so that the user sees at once where the epsilon is too small to
//close or too large to follow the shape, and where the sweep itself is too
//coarse to say (a gap of a fifth of the template is a sweep to densify,
//not an epsilon to tune).
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
        contourGraphs.reserve(static_cast<qint32>(m_contour.size()));
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

            plotLine(i,contourGraphs,phases, magnitudes, true, true,counter);

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

        plotLine(i,templateGraphs, phases, magnitudes, false, false, counter);
        i++;
        counter++;
    }

    //No setLayout here: the layout above was built with the frequency box
    //as its parent, which already installs it.



    ui->plot->replot();


}

void TemplateViewer::plotLine(qint32 pos, QVector <QCPGraph *> & graphs,
                              const std::vector<double> & phases, const std::vector<double> & magnitudes,
                              bool isContour, bool visible, qint32 counter){
    graphs.push_back(ui->plot->addGraph());
    ui->plot->graph(pos)->setData(qftbx::toQVector(phases), qftbx::toQVector(magnitudes));

    if (isContour){
        ui->plot->graph(pos)->setScatterStyle(QCPScatterStyle::ssNone);
        ui->plot->graph(pos)->setLineStyle(QCPGraph::lsLine);
    }else{
        ui->plot->graph(pos)->setScatterStyle(QCPScatterStyle::ssCross);
        ui->plot->graph(pos)->setLineStyle(QCPGraph::lsNone);
    }
    QColor color;

    color = colorByFrequency.value(m_omega.at(counter));
    if (visible){
        addFrequencyRow(color, pos);
    }

    ui->plot->graph(pos)->setPen(QPen(color, kCurveWidth));
    ui->plot->graph(pos)->setVisible(visible);

    if (pos == 0){
        ui->plot->graph(pos)->rescaleAxes();
        return;
    }
    ui->plot->graph(pos)->rescaleAxes(true);

}

void TemplateViewer::addFrequencyRow(QColor color, qint32 pos){
    const FrequencyLegend::Row row = legend->addRow(numberText(m_omega.at(pos)), color);

    //The epsilon of this frequency, when the project has one: a project can
    //hold the clouds and not the tolerance they were walked with, and a
    //row that asks a vector for an element it does not have takes the
    //toolbox down with a message about vector ranges.
    const bool known = pos < static_cast<qint32>(m_epsilon.size());
    const double epsilon = known ? m_epsilon.at(pos) : 0.0;

    //A slider for coarse moves and a field for the exact value, BESIDE the
    //frequency and not under it: a legend is a column of its own and every
    //line it takes is a line the diagram does not have.
    QSlider * slider = new QSlider(row.widget);
    slider->setObjectName(QString::fromUtf8("slider"));
    slider->setToolTip(tr("The epsilon of this frequency, by hand. Recompute walks the contours "
                          "again with it."));
    slider->setOrientation(Qt::Horizontal);
    slider->setMaximum(epsilon * 10000);
    slider->setValue(epsilon * 1000);
    epsilonSliders.push_back(slider);
    row.layout->addWidget(slider);

    QLineEdit * field = new QLineEdit(row.widget);
    field->setObjectName(QString::fromUtf8("field"));
    field->setToolTip(tr("The same epsilon, exactly: the diameter of the hull the contour of "
                         "this template is walked with."));
    field->setText(known ? numberText(epsilon) : QString());
    epsilonEdits.push_back(field);
    row.layout->addWidget(field);

    //What this template asks for, filled in by showProposals(): under the
    //line, and hidden until it has something to say.
    QLabel * gap = new QLabel(row.widget);
    gap->setObjectName(QString::fromUtf8("gap"));
    gap->setVisible(false);
    gapLabels.push_back(gap);
    row.column->addWidget(gap);

    //Whether the whole template stands in for this contour, by showContourState().
    QLabel * state = new QLabel(row.widget);
    state->setObjectName(QString::fromUtf8("contourState"));
    state->setVisible(false);
    stateLabels.push_back(state);
    row.column->addWidget(state);

    connect(slider, SIGNAL (sliderMoved (int)), this, SLOT (syncSliders ()));
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

    for (QCPGraph * parameter : contourGraphs) {
        parameter->setVisible(contourVisible);
    }
    ui->plot->replot();
}

void TemplateViewer::syncSliders(){
    for (qint32 i = 0; i < epsilonSliders.size(); i++){
        epsilonEdits.at(i)->setText(qftbx::numberText(epsilonSliders.at(i)->value() / 1000.0));
    }
}

void TemplateViewer::applyCheckboxes(){
    for (qint32 i = 0; i < legend->rowCount(); i++){
        if (!legend->isRowChecked(i)){
            contourGraphs.at(i)->setVisible(false);
        }else {
            contourGraphs.at(i)->setVisible(true);
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
        epsilonSliders.at(i)->setMaximum(std::max(epsilonSliders.at(i)->maximum(), static_cast<int>(value * 10000)));
        epsilonSliders.at(i)->setValue(static_cast<int>(value * 1000));
    }
    on_recomputeButton_clicked();
}

void TemplateViewer::on_recomputeButton_clicked()
{
    //Nothing plotted yet: the epsilon controls do not exist, and their
    //vectors are only created by plotDiagram (reading them here would be
    //reading uninitialised pointers).
    if (!plotted){
        return;
    }

    std::vector<double> epsilon;
    epsilon.reserve(epsilonEdits.size());

    for (qint32 i = 0; i < epsilonEdits.size(); i++) {
        qreal pos = epsilonEdits.at(i)->text().toDouble();
        epsilonSliders.at(i)->setValue(pos * 1000);
        epsilon.push_back(pos);
    }

    //The viewer draws; the computation belongs to whoever installed the
    //handler, which answers with refreshContour().
    if (!recompute){
        return;
    }

    recompute(std::move(epsilon));
}

} // namespace qftbx
