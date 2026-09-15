#include "src/gui/common/plot_setup.h"

#include <algorithm>
#include <cmath>

#include "qcustomplot.h"

namespace qftbx {

namespace {

//The viridis control points, every 10 % of the range. Interpolated linearly
//between them, which is within a couple of levels of the exact map and needs
//no table of 256 entries.
constexpr int kStops = 11;
constexpr int kViridis[kStops][3] = {
    { 68,   1,  84}, { 72,  40, 120}, { 62,  73, 137}, { 49, 104, 142},
    { 38, 130, 142}, { 31, 158, 137}, { 53, 183, 121}, {109, 205,  89},
    {180, 222,  44}, {253, 231,  37}, {253, 231,  37}
};

} // namespace

QColor frequencyColour(int index, int count)
{
    if (count <= 1) {
        return QColor(kViridis[4][0], kViridis[4][1], kViridis[4][2]);
    }

    //The top of the map is a light yellow that disappears on white, so the
    //walk stops at 92 % of it.
    const double t = 0.92 * std::clamp(static_cast<double>(index) / (count - 1), 0.0, 1.0);
    const double scaled = t * (kStops - 2);
    const int stop = static_cast<int>(scaled);
    const double fraction = scaled - stop;

    const auto mix = [&](int channel) {
        const double a = kViridis[stop][channel];
        const double b = kViridis[stop + 1][channel];
        return static_cast<int>(std::lround(a + (b - a) * fraction));
    };

    return QColor(mix(0), mix(1), mix(2));
}

void setUpPlot(QCustomPlot & plot, const QString & xLabel, const QString & yLabel)
{
    plot.xAxis->setLabel(xLabel);
    plot.yAxis->setLabel(yLabel);

    //The frame of four axes, with the two mirrored ones unlabelled: it is
    //what closes the chart, and the viewers connect their ranges.
    plot.axisRect()->setupFullAxesBox();
    plot.xAxis2->setTickLabels(false);
    plot.yAxis2->setTickLabels(false);

    plot.xAxis->grid()->setSubGridVisible(true);
    plot.yAxis->grid()->setSubGridVisible(true);

    plot.setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    plot.setSelectionRectMode(QCP::srmNone);

    //The wheel over an axis zooms that axis alone; over the canvas, both.
    plot.axisRect()->setRangeZoomAxes(plot.xAxis, plot.yAxis);
    QObject::connect(&plot, &QCustomPlot::mouseMove, &plot, [&plot](QMouseEvent * event) {
        const bool overX = plot.xAxis->selectTest(event->pos(), false) >= 0;
        const bool overY = plot.yAxis->selectTest(event->pos(), false) >= 0;
        if (overX && !overY) {
            plot.axisRect()->setRangeZoomAxes(plot.xAxis, nullptr);
            plot.axisRect()->setRangeDragAxes(plot.xAxis, nullptr);
        } else if (overY && !overX) {
            plot.axisRect()->setRangeZoomAxes(nullptr, plot.yAxis);
            plot.axisRect()->setRangeDragAxes(nullptr, plot.yAxis);
        } else {
            plot.axisRect()->setRangeZoomAxes(plot.xAxis, plot.yAxis);
            plot.axisRect()->setRangeDragAxes(plot.xAxis, plot.yAxis);
        }
    });

    //A double click frames the data again.
    QObject::connect(&plot, &QCustomPlot::mouseDoubleClick, &plot, [&plot](QMouseEvent *) {
        plot.rescaleAxes();
        plot.replot();
    });
}

} // namespace qftbx
