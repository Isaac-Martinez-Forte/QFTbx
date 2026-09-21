/**
 * @file
 * @brief Evaluates the plant over a sweep and draws both Bode canvases.
 *
 * The sweep is a hundred points over the design set's range, linear or
 * logarithmic as the set was generated, starting where the design starts;
 * the logarithmic ends are in rad/s and converted to exponents here, since
 * the generator takes exponents. A manual set, one from a file, or a
 * logarithmic one with a non-positive end is drawn at its own values.
 * Phase is in degrees. Both axes span the extremes of the data rather than
 * its first and last points, since a manual set need not be sorted. Export
 * writes two files, the suffix on the file name and not the path.
 */

#include <cmath>
#include "src/core/math/constants.h"
#include "src/gui/common/plot_export.h"
#include "src/gui/common/plot_setup.h"
#include <vector>
#include <algorithm>

#include "src/gui/common/qt_containers.h"
#include "src/gui/plant/bode_viewer.h"
#include "ui_bode_viewer.h"

#include "src/gui/application/error_message.h"

#include <QFileInfo>

using namespace std;

namespace qftbx {

BodeViewer::BodeViewer(QWidget *parent) :
    QWidget(parent),
    ui(std::make_unique<Ui::BodeViewer>())
{
    ui->setupUi(this);
    setWindowTitle(tr("Bode diagram"));

    qftbx::setUpPlot(*ui->magnitudePlot, tr("frequency (rad/s)"), tr("magnitude (dB)"));
    qftbx::setUpPlot(*ui->phasePlot, tr("frequency (rad/s)"), tr("phase (degrees)"));
}

BodeViewer::~BodeViewer() = default;

void BodeViewer::clear(){

    ui->magnitudePlot->clearPlottables();
    ui->phasePlot->clearPlottables();
    ui->magnitudePlot->replot();
    ui->phasePlot->replot();
}

void BodeViewer::drawBode(LtiSystem *plant, Omega *omega){

    ui->magnitudePlot->clearPlottables();
    ui->phasePlot->clearPlottables();

    std::vector<double> frequencies;

    if (omega->type() == Omega::LinSpace){
        frequencies = linspace(omega->start(), omega->end(), 100);
    }else if (omega->type() == Omega::LogSpace){
        if (omega->start() > 0.0 && omega->end() > 0.0){
            frequencies = logspace(std::log10(omega->start()),
                                   std::log10(omega->end()), 100);
        } else {
            frequencies = *omega->values();
        }
    }else {
        frequencies = *omega->values();
    }

    std::vector<double> magnitude;
    std::vector<double> phase;
    magnitude.reserve(frequencies.size());
    phase.reserve(frequencies.size());

    for (const std::complex<qreal> &comp : plant->evaluate(frequencies)){
        magnitude.push_back(20*log10(abs(comp)));

        phase.push_back(arg(comp) * 180.0 / qftbx::math::kPi);
    }

    drawAxis(tr("Magnitude (dB)"), magnitude, frequencies, ui->magnitudePlot);
    drawAxis(tr("Phase (deg)"), phase, frequencies, ui->phasePlot);
    this->setWindowTitle(tr("Bode diagram"));
    ui->magnitudePlot->replot();
    ui->phasePlot->replot();
}

void BodeViewer::drawAxis(QString yAxisName, const std::vector<double> & yAxis_values,
                          const std::vector<double> & frequencies, QCustomPlot * magnitudePlot){

    QCPCurve *curve = new QCPCurve(magnitudePlot->xAxis, magnitudePlot->yAxis);
    curve->setData(qftbx::toQVector(frequencies), qftbx::toQVector(yAxis_values));

    magnitudePlot->yAxis->setLabel(yAxisName);

    magnitudePlot->xAxis->setScaleType(QCPAxis::ScaleType::stLogarithmic);

    const auto frequencyEnds = std::minmax_element(frequencies.begin(), frequencies.end());
    const auto valueEnds = std::minmax_element(yAxis_values.begin(), yAxis_values.end());

    magnitudePlot->xAxis->setRange(*frequencyEnds.first, *frequencyEnds.second);
    magnitudePlot->yAxis->setRange(*valueEnds.first, *valueEnds.second);
}

void BodeViewer::on_saveImage_clicked()
{
    QString selected;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save figure"), QString(),
                                                    qftbx::exportFilter(), &selected);
    if (fileName.isEmpty()){
        return;
    }

    if (QFileInfo(fileName).suffix().isEmpty()) {
        for (const QString known : {"pdf", "svg", "png"}) {
            if (selected.contains("." + known, Qt::CaseInsensitive)) {
                fileName += "." + known;
                break;
            }
        }
    }

    const QFileInfo info(fileName);
    const auto named = [&info](const QString & part) {
        return info.dir().filePath(info.completeBaseName() + "-" + part + "." + info.suffix());
    };

    const auto write = [](QCustomPlot & plot, const QString & name) {
        qftbx::ExportRequest request;
        request.fileName = name;
        request.size = plot.size() * 2;
        request.profile = qftbx::ExportProfile::ForPublishing;
        return qftbx::savePlot(plot, request);
    };

    if (!write(*ui->magnitudePlot, named("mag")) || !write(*ui->phasePlot, named("phase"))){
        errorMessage(tr("The figure could not be saved"), tr("Bode diagram"));
    }
}

}
