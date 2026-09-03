#ifndef QFTBX_GUI_QT_CONTAINERS_H
#define QFTBX_GUI_QT_CONTAINERS_H

#include <vector>

#include <QVector>

namespace tools {

/**
 * @brief A std::vector as the QVector a Qt API asks for.
 *
 * The seam, in one function. The core and the persistence hold std
 * containers - phase 9.8 - and QCustomPlot's setData() takes QVector, so
 * somebody has to copy. Better here, named and visible at the call, than by
 * keeping a Qt container in the model to save a conversion nobody sees.
 */
template <class T>
QVector<T> toQVector(const std::vector<T> & values)
{
    return QVector<T>(values.begin(), values.end());
}

} // namespace tools

#endif // QFTBX_GUI_QT_CONTAINERS_H
