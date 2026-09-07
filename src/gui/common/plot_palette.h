#ifndef QFTBX_GUI_PLOT_PALETTE_H
#define QFTBX_GUI_PLOT_PALETTE_H

#include <QColor>
#include <Qt>

/**
 * @brief Series palette for the plots.
 *
 * Fixed while moving: index 3 used to fall through to the default colour,
 * and darkYellow appeared twice, so two series were painted alike.
 */
namespace qftbx {

inline QColor randomColor (qint32 i){

    switch (i){
    case 0: return Qt::red;
    case 1: return Qt::darkYellow;
    case 2: return Qt::green;
    case 3: return Qt::darkRed;
    case 4: return Qt::magenta;
    case 5: return Qt::darkGreen;
    case 6: return Qt::blue;
    case 7: return Qt::darkBlue;
    case 8: return Qt::darkCyan;
    case 9: return Qt::darkGray;
    case 10: return Qt::darkMagenta;
    case 11: return Qt::yellow;
    case 12: return Qt::gray;
    default: return Qt::cyan;
    }

}

} // namespace qftbx

#endif // QFTBX_GUI_PLOT_PALETTE_H
