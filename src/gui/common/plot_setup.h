/**
 * @file
 * @brief What every plot of the toolbox shares: interaction, palette, curve colours and widths.
 *
 * The setup is applied once when a viewer is built: the wheel zooms one
 * axis over that axis and both over the canvas, and a double click frames
 * the data again. The palette colours ground, axes and grid from the
 * interface's theme and is reapplied when the theme changes; the curves are
 * not touched, because a curve's colour is its frequency. Frequency colours
 * follow the viridis map of Smith and van der Walt, perceptually uniform
 * and readable in greyscale, which shows which way the frequency grows and
 * never runs out. The loop transmission is red and heavier than the
 * boundaries, being the answer the design was for, and the column of
 * controls beside a diagram is capped in width.
 */

#ifndef QFTBX_GUI_PLOT_SETUP_H
#define QFTBX_GUI_PLOT_SETUP_H

#include <QColor>
#include <QLayout>
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

/**
 * @brief The colour and the width of the loop transmission drawn over the
 * boundaries.
 *
 * Every other curve on that chart is something the loop has to respect;
 * this one is the ANSWER, the thing the whole design was for. In black it
 * read as one more line among the boundaries, so it is red and a little
 * heavier: on a chart of a dozen curves the eye has to find it without
 * looking for it.
 */
inline const QColor kLoopColour = QColor(0xe1, 0x06, 0x00);
inline constexpr double kLoopWidth = 2.4;

/**
 * @brief Caps the width of the column of controls beside a diagram.
 *
 * The four viewers put their buttons and their legend in a column to the
 * left of the chart, and that column took whatever its widest button asked
 * for - half a card, on a phase with a slider per frequency - while the
 * chart, which already spends a fair part of its width on the labels of its
 * axes, lived on what was left. Every widget of the layout given is capped,
 * including those of the layouts inside it.
 */
void narrowSideColumn(QLayout * side);

/// How wide that column is worth being.
inline constexpr int kSideColumn = 210;

}

#endif
