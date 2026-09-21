/**
 * @file
 * @brief Colours for series that have no order: one per algorithm, case or name.
 *
 * A design frequency is an ordered magnitude and takes its colour from the
 * frequency map of the plot setup; this palette is for the benchmark, where
 * the series are algorithms and nothing sits between two of them. Past the
 * eight named colours it walks the hue circle by the golden angle, with the
 * lightness alternating so that two hues close by eye differ in value,
 * rather than repeating and painting two series alike.
 */

#ifndef QFTBX_GUI_PLOT_PALETTE_H
#define QFTBX_GUI_PLOT_PALETTE_H

#include <QColor>
#include <Qt>

namespace qftbx {

/**
 * @brief Palette for series that are NOT ordered: one colour per algorithm,
 * per case, per name.
 *
 * A design frequency is not one of those - it is a magnitude with an order -
 * and its colour comes from qftbx::frequencyColour (plot_setup.h), which
 * follows that order and never runs out. This one is for the benchmark, where
 * the series are five algorithms and nothing sits between two of them.
 *
 * Past the named colours it walks the hue circle rather than repeating: a
 * palette that repeats paints two series alike, which is worse than a colour
 * nobody would have chosen.
 */
inline QColor seriesColour(qint32 i)
{
    static const QColor named[] = {
        Qt::red, Qt::darkCyan, Qt::darkGreen, Qt::darkMagenta,
        Qt::blue, Qt::darkRed, Qt::darkBlue, Qt::darkYellow
    };
    constexpr qint32 count = static_cast<qint32>(sizeof(named) / sizeof(named[0]));

    if (i < 0) {
        return named[0];
    }
    if (i < count) {
        return named[i];
    }

    const qint32 beyond = i - count;
    const int hue = static_cast<int>((beyond * 137) % 360);
    const int lightness = (beyond % 2 == 0) ? 110 : 160;
    return QColor::fromHsl(hue, 170, lightness);
}

}

#endif
