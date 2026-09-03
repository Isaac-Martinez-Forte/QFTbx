
#include "src/core/exception.h"
#include <QMessageBox>

#include "qt_containers.h"
#include "template_viewer.h"
#include "ui_template_viewer.h"

#include "src/gui/error_message.h"
#include "src/gui/plot_palette.h"

using namespace std;
//using namespace tools;

TemplateViewer::TemplateViewer(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::TemplateViewer>())
{
    ui->setupUi(this);

    templatesVisible = false;
    contourVisible = true;
    setWindowTitle(tr("Templates"));


    frequenciesBox = new QGroupBox(this);
    frequenciesBox->setObjectName("frequenciesBox");
    frequenciesBox->setGeometry(QRect(660, 0, 141, 461));
    frequenciesBox->setTitle(QApplication::translate("TemplateViewer", "Frequencies", 0));

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

    //Qt's own mechanism, and the only reason there is a delete here: every
    //frequency-box row is freed WHOLE through its container widget, whose
    //children the checkbox, the slider and the line edit are. The loose
    //controls used to be deleted while the containers piled up in the
    //layout on every replot.
    for (QCheckBox * che : checkboxes) {
        delete che->parentWidget();
    }
    checkboxes.clear();
    epsilonEdits.clear();
    epsilonSliders.clear();

    contourGraphs.clear();
    templateGraphs.clear();

    //Also Qt's: a widget holds exactly one layout, so rebuilding the
    //frequency box means destroying the one it has.
    delete colorsLayout;
    colorsLayout = nullptr;

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

    this->omega = omega;

    this->epsilon = epsilon;

    for (qint32 i = 0; i < omega->size(); i++){
        colorByFrequency.insert(omega->at(i), tools::randomColor(i));
    }
}

void TemplateViewer::setContourRecomputer(ContourRecomputer recompute){
    this->recompute = std::move(recompute);
}

void TemplateViewer::refreshContour(const qftbx::CloudSet & contour,
                                    std::vector<double> * omega,
                                    std::vector<double> * epsilon){
    setContour(contour);

    this->omega = omega;

    //The previous epsilon belongs to the project, which deleted it when it
    //accepted the new one; touching it here would be a use-after-free.
    this->epsilon = epsilon;

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

    colorsLayout = new QVBoxLayout (frequenciesBox);

    plotted = true;
    qint32 i = 0;
    qint32 counter = 0;

    if (m_templates.empty())
        return;
    templateGraphs.reserve(static_cast<qint32>(m_templates.size()));

    if (!m_contour.empty()){
        contourGraphs.reserve(static_cast<qint32>(m_contour.size()));
        for (const qftbx::ComplexCloud & vector : m_contour) {

            std::vector<double> fas;
            fas.reserve(static_cast<qint32>(vector.size()));
            std::vector<double> gan;
            gan.reserve(static_cast<qint32>(vector.size()));

            for (const std::complex <qreal> & complejo : vector) {

                if (plot){
                    qreal phase = arg(complejo)* 180 / M_PI;
                    if (phase >= 0){
                        phase -= 360;
                    }
                    fas.push_back(phase);
                    qreal mag = 20*log10(abs(complejo));
                    gan.push_back(mag);
                }else {
                    qreal phase = complejo.real();
                    fas.push_back(phase);
                    gan.push_back(complejo.imag());
                }
            }

            plotLine(i,contourGraphs,fas, gan, true, true,counter);

            i++;
            counter++;
        }

    }

    counter = 0;

    for (const qftbx::ComplexCloud & vector : m_templates) {

        std::vector<double> fas;
        fas.reserve(static_cast<qint32>(vector.size()));
        std::vector<double> gan;
        gan.reserve(static_cast<qint32>(vector.size()));

        for (const std::complex <qreal> & complejo : vector) {

            if (plot){
                qreal phase = arg(complejo)* 180 / M_PI;
                if (phase >= 0){
                    phase -= 360;
                }
                fas.push_back(phase);
                gan.push_back(20*log10(abs(complejo)));
            }else{
                qreal phase = complejo.real();
                fas.push_back(phase);
                gan.push_back(complejo.imag());
            }

        }

        plotLine(i,templateGraphs, fas, gan, false, false, counter);
        i++;
        counter++;
    }

    //No setLayout here: the layout above was built with the frequency box
    //as its parent, which already installs it.

    ui->plot->xAxis2->setVisible(true);
    ui->plot->xAxis2->setTickLabels(false);
    ui->plot->yAxis2->setVisible(true);
    ui->plot->yAxis2->setTickLabels(false);

    //ui->plot->legend->setVisible(true);
    //ui->plot->legend->setBrush(QColor(255, 255, 255, 150));

    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    ui->plot->replot();


}

void TemplateViewer::plotLine(qint32 pos, QVector <QCPGraph *> & saveImage,
                              const std::vector<double> & fas, const std::vector<double> & gan,
                              bool tipo, bool visible, qint32 counter){
    saveImage.push_back(ui->plot->addGraph());
    ui->plot->graph(pos)->setData(tools::toQVector(fas), tools::toQVector(gan));

    if (tipo){
        ui->plot->graph(pos)->setScatterStyle(QCPScatterStyle::ssNone);
        ui->plot->graph(pos)->setLineStyle(QCPGraph::lsLine);
    }else{
        ui->plot->graph(pos)->setScatterStyle(QCPScatterStyle::ssCross);
        ui->plot->graph(pos)->setLineStyle(QCPGraph::lsNone);
    }
    QColor color;

    if (visible){
        color = colorByFrequency.value(omega->at(counter));
        addFrequencyRow(color, pos);
    }else{
        color = colorByFrequency.value(omega->at(counter));
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

    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QCheckBox *check;
    QSlider *slider;
    QLineEdit *linea;

    widget = new QWidget(frequenciesBox);
    widget->setObjectName(QString::fromUtf8("widget"));
    widget->setGeometry(QRect(60, 70, 177, 58));

    QMetaObject::connectSlotsByName(widget);

    verticalLayout = new QVBoxLayout(widget);
    verticalLayout->setSpacing(6);
    verticalLayout->setContentsMargins(11, 11, 11, 11);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setSpacing(6);
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));

    check = new QCheckBox(widget);
    check->setObjectName(QString::fromUtf8("check"));
    check->setText(QString::number(omega->at(pos)));
    check->setStyleSheet("color : " + color.name());

    checkboxes.push_back(check);
    check->setCheckState(Qt::Checked);
    horizontalLayout->addWidget(check);


    slider = new QSlider(widget);
    slider->setObjectName(QString::fromUtf8("slider"));
    slider->setOrientation(Qt::Horizontal);
    slider->setMaximum(epsilon->at(pos) * 10000);
    slider->setValue(epsilon->at(pos) * 1000);

    epsilonSliders.push_back(slider);
    horizontalLayout->addWidget(slider);


    verticalLayout->addLayout(horizontalLayout);

    linea = new QLineEdit(widget);
    linea->setObjectName(QString::fromUtf8("linea"));
    linea->setText(QString::number(epsilon->at(pos)));

    epsilonEdits.push_back(linea);
    verticalLayout->addWidget(linea);


    colorsLayout->addWidget(widget);

    connect(slider, SIGNAL (sliderMoved (int)), this, SLOT (syncSliders ()));
    connect(check, SIGNAL (clicked()), this, SLOT (applyCheckboxes()));
}

void TemplateViewer::on_saveImage_clicked()
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
            tools::errorMessage(tr("The image could not be saved"), tr("Template plot"));
    }
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
        epsilonEdits.at(i)->setText(QString::number(epsilonSliders.at(i)->value() / 1000.0));
    }
}

void TemplateViewer::applyCheckboxes(){
    for (qint32 i = 0; i < checkboxes.size(); i++){
        if (checkboxes.at(i)->checkState() == 0){
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


