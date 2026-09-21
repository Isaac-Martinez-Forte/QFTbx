/**
 * @file
 * @brief Plot setup, palette application and the viridis interpolation.
 *
 * The map is held as eleven control points every tenth of the range and
 * interpolated linearly, which stays within a couple of levels of the exact
 * map without a table of 256 entries; the walk stops at 92 % of it because
 * the top is a light yellow that disappears on white. The chart is closed
 * by a frame of four axes with the mirrored two unlabelled, and the axis
 * under the cursor is chosen on every mouse move for both drag and zoom.
 */

#include "src/gui/common/plot_setup.h"

#include <algorithm>
#include <cmath>

#include "qcustomplot.h"

namespace qftbx {

namespace {

constexpr int kStops = 11;
constexpr int kViridis[kStops][3] = {
    { 68,   1,  84}, { 72,  40, 120}, { 62,  73, 137}, { 49, 104, 142},
    { 38, 130, 142}, { 31, 158, 137}, { 53, 183, 121}, {109, 205,  89},
    {180, 222,  44}, {253, 231,  37}, {253, 231,  37}
};

}

QColor frequencyColour(int index, int count)
{
    if (count <= 1) {
        return QColor(kViridis[4][0], kViridis[4][1], kViridis[4][2]);
    }

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

void applyPlotPalette(QCustomPlot & plot, const QPalette & palette)
{
    const QColor ground = palette.color(QPalette::Base);
    const QColor ink = palette.color(QPalette::Text);
    const QColor grid = palette.color(QPalette::Mid);

    plot.setBackground(QBrush(ground));
    plot.axisRect()->setBackground(QBrush(ground));

    for (QCPAxis * axis : {plot.xAxis, plot.yAxis, plot.xAxis2, plot.yAxis2}) {
        axis->setBasePen(QPen(ink));
        axis->setTickPen(QPen(ink));
        axis->setSubTickPen(QPen(ink));
        axis->setTickLabelColor(ink);
        axis->setLabelColor(ink);
        axis->grid()->setPen(QPen(grid, 1, Qt::DotLine));
        axis->grid()->setSubGridPen(QPen(grid, 1, Qt::DotLine));
    }

    plot.replot();
}

void setUpPlot(QCustomPlot & plot, const QString & xLabel, const QString & yLabel)
{
    plot.xAxis->setLabel(xLabel);
    plot.yAxis->setLabel(yLabel);

    plot.axisRect()->setupFullAxesBox();
    plot.xAxis2->setTickLabels(false);
    plot.yAxis2->setTickLabels(false);

    plot.xAxis->grid()->setSubGridVisible(true);
    plot.yAxis->grid()->setSubGridVisible(true);

    plot.setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    plot.setSelectionRectMode(QCP::srmNone);

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

    applyPlotPalette(plot, plot.palette());

    QObject::connect(&plot, &QCustomPlot::mouseDoubleClick, &plot, [&plot](QMouseEvent *) {
        plot.rescaleAxes();
        plot.replot();
    });
}

void narrowSideColumn(QLayout * side)
{
    if (side == nullptr) {
        return;
    }

    for (int i = 0; i < side->count(); ++i) {
        QLayoutItem * item = side->itemAt(i);

        if (QWidget * widget = item->widget()) {
            widget->setMaximumWidth(kSideColumn);
        } else if (QLayout * inside = item->layout()) {
            narrowSideColumn(inside);
        }
    }
}

}
