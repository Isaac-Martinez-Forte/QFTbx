/**
 * @file
 * @brief The one conversion from a standard vector to the Qt vector a plot
 * asks for.
 *
 * The core and the persistence hold standard containers and the plotting
 * library takes Qt ones, so somebody has to copy. It is done here, named
 * and visible at the call, rather than by keeping a Qt container in a model
 * to save a conversion nobody sees.
 */

#ifndef QFTBX_GUI_QT_CONTAINERS_H
#define QFTBX_GUI_QT_CONTAINERS_H

#include <vector>

#include <QVector>

namespace qftbx {

template <class T>
QVector<T> toQVector(const std::vector<T> & values)
{
    return QVector<T>(values.begin(), values.end());
}

}

#endif
