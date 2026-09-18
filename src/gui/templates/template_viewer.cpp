
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

//The diagram does not give away all its width: below this it stops being
//readable, and it is the panel beside it that has to give way.
constexpr int kMinimumPlot = 320;

} // namespace

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

    //The column of controls does not take half the card: the chart needs
    //the width more than the buttons do.
    narrowSideColumn(ui->sideLayout);

    //The frequencies are the exception: they are what the panel is opened
    //for, and a row that does not fit is a row that cannot be read.
    ui->legendHolder->setMaximumWidth(QWIDGETSIZE_MAX);

    //The two of them share the width and the separation belongs to the
    //user: the card growing widens the frequencies as well as the diagram,
    //and whoever needs to read a long row drags the handle across.
    QSplitter * splitter = new QSplitter(Qt::Horizontal, this);
    ui->sidePanel->setMinimumWidth(kSideColumn);
    ui->plot->setMinimumWidth(kMinimumPlot);
    splitter->addWidget(ui->sidePanel);
    splitter->addWidget(ui->plot);
    //Neither of the two is dragged out of sight.
    splitter->setChildrenCollapsible(false);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    //The card opens as it did before: the diagram takes the width, and the
    //panel is widened by whoever wants it wider.
    splitter->setSizes({kSideColumn, 3 * kSideColumn});
    ui->outerLayout->addWidget(splitter);

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

//What the contour of each frequency went through, marked next to its
//epsilon and never as a dialog, since trying an epsilon and looking is how
//a contour is tuned. Two things can be worth saying, and the worse one
//wins: that no walk closed and the whole template stands in, and that the
//faithful walk did not close so the relaxed one stood in - which is why
//that contour is drawn as an OPEN curve, with the piece the walk never
//went round missing. Neither is an error; both are answered by the epsilon
//the Propose button computes.
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

    //No setLayout here: the layout above was built with the frequency box
    //as its parent, which already installs it.



    ui->plot->replot();


}

//The cloud: crosses, and no line. A value set is a SET - the order its
//points come in is the order of the parameter sweep, which has nothing to
//do with where they sit on the plane - so any line through it is a lie.
//What the border of a cloud looks like is what its contour is for.
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

//The contour: a CURVE, which is the whole point. A QCPGraph is a function
//of its key and sorts its points by phase, so a closed contour came out as
//a comb of vertical strokes across the cloud - the border walked in phase
//order instead of in walk order. A QCPCurve is parametric: it keeps the
//order it is given, which is the order the walk found the border in.
void TemplateViewer::plotContour(const std::vector<double> & phases,
                                 const std::vector<double> & magnitudes, qint32 frequency)
{
    const QColor color = colorByFrequency.value(m_omega.at(frequency));

    //A cloud that epsilon does not hold together is walked once per
    //component, and the walks arrive concatenated in one vector: drawn as
    //one curve, a line crossed from the end of one component to the start
    //of the next. Where each begins is in the report.
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

    //One row of the legend per frequency, whatever it took to draw it.
    addFrequencyRow(color, frequency);

    if (frequency == 0) {
        ui->plot->rescaleAxes();
    } else {
        ui->plot->rescaleAxes(true);
    }
}

void TemplateViewer::addFrequencyRow(QColor color, qint32 pos){
    const FrequencyLegend::Row row = legend->addRow(numberText(m_omega.at(pos)), color);

    //The epsilon of this frequency, when the project has one: a project can
    //hold the clouds and not the tolerance they were walked with, and a
    //row that asks a vector for an element it does not have takes the
    //toolbox down with a message about vector ranges.
    const bool known = pos < static_cast<qint32>(m_epsilon.size());
    const double epsilon = known ? m_epsilon.at(pos) : 0.0;

    //The field for the value, BESIDE the frequency and not under it: a
    //legend is a column of its own and every line it takes is a line the
    //diagram does not have.
    QLineEdit * field = new QLineEdit(row.widget);
    field->setObjectName(QString::fromUtf8("field"));
    field->setToolTip(tr("The epsilon of this frequency: the diameter of the hull the contour "
                         "of this template is walked with. Recompute walks the contours again "
                         "with it."));
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
        //A frequency unticked takes its contour and its cloud with it.
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
