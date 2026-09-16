#ifndef QFTBX_GUI_PLOT_SETUP_H
#define QFTBX_GUI_PLOT_SETUP_H

#include <QColor>
#include <QPalette>
#include <QString>

class QCustomPlot;

namespace qftbx {

/**
 * @brief What every plot of the toolbox is set up with, in one place.
 *
 * Things to keep in mind:
 * - It is called ONCE per plot, when the viewer is built, and not from the
 *   drawing routine. A plot configured while drawing has no interactions
 *   until it has data, and loses them on any redraw that takes another path.
 * - The wheel zooms ONE axis when the cursor is over that axis, and both
 *   over the canvas. Zooming both together is QCustomPlot's default and it
 *   is rarely what a Nichols chart wants: a phase span cannot be examined
 *   without flattening the decibels.
 * - A double click frames the data again. Without it a zoom that has gone
 *   too far has no way back.
 */
void setUpPlot(QCustomPlot & plot, const QString & xLabel, const QString & yLabel);

/**
 * @brief Colours a plot from the palette of the interface: the ground, the
 * four axes, their labels and the grid.
 *
 * So that a diagram belongs to the theme around it instead of being a white
 * rectangle in the middle of a dark window. What it does NOT touch is the
 * curves: those carry the frequency they belong to (frequencyColour) and
 * mean the same in either theme.
 *
 * Called by setUpPlot, and again by the window when the theme changes, for
 * the plots that already exist. The figures a user EXPORTS are not affected:
 * qftbx::savePlot draws them on white with black axes whatever the screen
 * wears, because that is what a paper wants.
 */
void applyPlotPalette(QCustomPlot & plot, const QPalette & palette);

/**
 * @brief The colour of a curve that is the i-th of n over an ORDERED
 * magnitude, which is what a design frequency is.
 *
 * The viridis map of Smith and van der Walt: perceptually uniform, readable
 * in greyscale and safe for the usual colour blindness. It says something
 * besides telling curves apart - which way the frequency grows - and it does
 * not run out: a palette of named colours repeats itself once the
 * frequencies outnumber it, which is what happened here from the fifteenth
 * on, and twenty-three design frequencies are an ordinary problem.
 */
QColor frequencyColour(int index, int count);

/// The pen width of a drawn curve: one pixel is a hairline in an exported
/// figure, where the resolution is not the screen's.
inline constexpr double kCurveWidth = 1.6;

} // namespace qftbx

#endif // QFTBX_GUI_PLOT_SETUP_H
