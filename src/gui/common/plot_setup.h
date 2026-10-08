/**
 * @file
 * @brief What every plot of the toolbox shares: interaction, palette,
 * curve colours and widths.
 *
 * setUpPlot is called once per plot, when the viewer is built, and never
 * from the drawing routine: a plot set up while drawing has no
 * interactions until it has data and loses them on a redraw that takes
 * another path. The wheel zooms one axis over that axis and both over the
 * canvas, so that a phase span can be examined without flattening the
 * decibels, and a double click frames the data again. The palette colours
 * ground, axes and grid from the interface's theme; setUpPlot applies it,
 * and the window again when the theme changes. The curves are not
 * touched, because a curve's colour is its frequency, and neither are
 * exported figures, which savePlot draws on white with black axes.
 *
 * Frequency colours follow the viridis map of Smith and van der Walt,
 * perceptually uniform and readable in greyscale, which shows which way
 * the frequency grows and never runs out. Curves are wider than a pixel,
 * a hairline in an exported figure. The loop transmission is red and
 * heavier than the boundaries, being the answer the design was for, and
 * narrowSideColumn caps every widget of the column of controls beside a
 * diagram, those of nested layouts included.
 */

#ifndef QFTBX_GUI_PLOT_SETUP_H
#define QFTBX_GUI_PLOT_SETUP_H

#include <QColor>
#include <QLayout>
#include <QPalette>
#include <QString>

class QCustomPlot;

namespace qftbx {

void setUpPlot(QCustomPlot & plot, const QString & xLabel, const QString & yLabel);

void applyPlotPalette(QCustomPlot & plot, const QPalette & palette);

QColor frequencyColour(int index, int count);

inline constexpr double kCurveWidth = 1.6;

inline const QColor kLoopColour = QColor(0xe1, 0x06, 0x00);
inline constexpr double kLoopWidth = 2.4;

void narrowSideColumn(QLayout * side);

inline constexpr int kSideColumn = 210;

}

#endif
