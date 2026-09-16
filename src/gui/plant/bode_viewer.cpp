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

    //Both canvases, once: the magnitude carries the label the caller chooses
    //(the plant's or the controller's), which drawBode() sets.
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

    //Replottable: without this every call piled new curves up.
    ui->magnitudePlot->clearPlottables();
    ui->phasePlot->clearPlottables();

    //By value, so the sweep is a plain local whether the frequencies are
    //the project's or this viewer's own.
    std::vector<double> frequencies;

    //The sweep starts where the DESIGN starts, not at a fixed exponent: on
    //the linear path a fixed -1 asks for negative frequencies, on an axis
    //that is logarithmic below.
    if (omega->type() == Omega::LinSpace){
        frequencies = linspace(omega->start(), omega->end(), 100);
    }else if (omega->type() == Omega::LogSpace){
        //start()/end() are in rad/s, like every other frequency in the
        //toolbox, and qftbx::logspace takes exponents - so the conversion
        //happens here, the same way the frequencies form does it when it
        //builds the set. Holding the exponents instead makes the unit a
        //secret shared between two files.
        if (omega->start() > 0.0 && omega->end() > 0.0){
            frequencies = logspace(std::log10(omega->start()),
                                   std::log10(omega->end()), 100);
        } else {
            //An older set, or one that cannot be re-derived: its own values
            //are always there.
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

        //Degrees: a Bode phase plot is read in degrees, and arg() answers
        //radians (the axis was labelled in Spanish and scaled in radians).
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

    //QCPCurve attaches itself to the plot, which owns it from then on.
    QCPCurve *curve = new QCPCurve(magnitudePlot->xAxis, magnitudePlot->yAxis);
    curve->setData(qftbx::toQVector(frequencies), qftbx::toQVector(yAxis_values));

    magnitudePlot->yAxis->setLabel(yAxisName);

    magnitudePlot->xAxis->setScaleType(QCPAxis::ScaleType::stLogarithmic);

    //Both axes span the EXTREMES of the data: first to last frames the
    //curve only when it is monotonic and the frequencies are sorted, and a
    //manual set need not be.
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
        for (const QString & known : {"pdf", "svg", "png"}) {
            if (selected.contains("." + known, Qt::CaseInsensitive)) {
                fileName += "." + known;
                break;
            }
        }
    }

    //Two canvases, two files: the suffix goes on the file NAME, since
    //prefixing the full path produces something that is not a path.
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

} // namespace qftbx
