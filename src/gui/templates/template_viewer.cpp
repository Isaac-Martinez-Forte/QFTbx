
#include "src/core/common/exception.h"
#include "src/core/math/constants.h"
#include "src/gui/common/plot_export.h"
#include "src/gui/common/number_text.h"
#include <QMessageBox>

#include "src/gui/common/qt_containers.h"
#include "src/gui/templates/template_viewer.h"
#include "ui_template_viewer.h"

#include "src/gui/application/error_message.h"
#include "src/gui/common/plot_palette.h"

using namespace std;

namespace qftbx {

TemplateViewer::TemplateViewer(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::TemplateViewer>())
{
    ui->setupUi(this);

    templatesVisible = false;
    contourVisible = true;
    setWindowTitle(tr("Templates"));


    legend = new FrequencyLegend(this);
    legend->setGeometry(QRect(660, 0, 141, 461));
    connect(legend, &FrequencyLegend::rowToggled, this, &TemplateViewer::applyCheckboxes);

    //Connected ONCE (every replot used to add a duplicated connection).
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

    contourGraphs.clear();
    templateGraphs.clear();


    plotted = false;
}

void TemplateViewer::setData(const qftbx::CloudSet & templates,
                              const qftbx::CloudSet & contour,
                              std::vector<double> * omega,
                              std::vector<double> * epsilon){

    //The map used to be replaced with a new/delete pair, and leaked on
    //every recompute when the delete was forgotten.
    colorByFrequency.clear();

    setTemplates(templates);
    setContour(contour);

    //Copies: the viewer used to alias the project's vectors, and it outlives
    //them across a load.
    m_omega = *omega;
    m_epsilon = *epsilon;

    for (qint32 i = 0; i < static_cast<std::int32_t>(m_omega.size()); i++){
        colorByFrequency.insert(m_omega.at(i), qftbx::randomColor(i));
    }
}

void TemplateViewer::setContourRecomputer(ContourRecomputer recompute){
    this->recompute = std::move(recompute);
}

void TemplateViewer::refreshContour(const qftbx::CloudSet & contour,
                                    std::vector<double> * omega,
                                    std::vector<double> * epsilon){
    setContour(contour);

    m_omega = *omega;
    m_epsilon = *epsilon;

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

    ui->plot->xAxis2->setVisible(true);
    ui->plot->xAxis2->setTickLabels(false);
    ui->plot->yAxis2->setVisible(true);
    ui->plot->yAxis2->setTickLabels(false);

    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

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

    ui->plot->graph(pos)->setPen(color);
    ui->plot->graph(pos)->setVisible(visible);

    if (pos == 0){
        ui->plot->graph(pos)->rescaleAxes();
        return;
    }
    ui->plot->graph(pos)->rescaleAxes(true);

}

void TemplateViewer::addFrequencyRow(QColor color, qint32 pos){
    const FrequencyLegend::Row row = legend->addRow(numberText(m_omega.at(pos)), color);

    //The epsilon of this frequency: a slider for coarse moves and a field
    //for the exact value, both in the legend's row.
    QSlider * slider = new QSlider(row.widget);
    slider->setObjectName(QString::fromUtf8("slider"));
    slider->setOrientation(Qt::Horizontal);
    slider->setMaximum(m_epsilon.at(pos) * 10000);
    slider->setValue(m_epsilon.at(pos) * 1000);
    epsilonSliders.push_back(slider);
    row.layout->addWidget(slider);

    QLineEdit * field = new QLineEdit(row.widget);
    field->setObjectName(QString::fromUtf8("field"));
    field->setText(numberText(m_epsilon.at(pos)));
    epsilonEdits.push_back(field);
    row.layout->addWidget(field);

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
