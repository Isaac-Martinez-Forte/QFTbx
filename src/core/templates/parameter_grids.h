#ifndef QFTBX_PARAMETER_GRIDS_H
#define QFTBX_PARAMETER_GRIDS_H

#include <map>
#include <vector>

#include <QString>

namespace qftbx {

/**
 * @brief The sweep grid of every uncertain parameter, keyed by NAME.
 *
 * Held BY VALUE, which is the whole point: this used to travel as
 * `QHash<QString, QVector<double> *> *` - a pointer to a map of pointers - and
 * nobody could tell from a signature who was supposed to free it. The engine
 * only stored the pointer, the dialog owned the map, the facade passed it
 * through, and every test had to remember to walk it and delete each vector.
 * Two leaks of this refactor came from exactly that, and a third from the
 * question being unanswerable on a throw path.
 *
 * Keyed by name and not by pointer identity because a clone() or a project
 * reload gives the same parameter a new address (that was a bug too).
 *
 * std::map rather than QHash so that iteration order is deterministic: the
 * sweep is expected to be bit-reproducible, and an unordered container is one
 * more thing that could quietly stop being so.
 */
using ParameterGrids = std::map<QString, std::vector<double>>;

} // namespace qftbx

#endif // QFTBX_PARAMETER_GRIDS_H
