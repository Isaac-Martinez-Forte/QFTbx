/**
 * @file
 * @brief The sweep grid of every uncertain parameter, keyed by name.
 *
 * Keyed by name and not by object, so that a grid survives a copy of the
 * plant or a reload of the project.
 */

#ifndef QFTBX_PARAMETER_GRIDS_H
#define QFTBX_PARAMETER_GRIDS_H

#include <map>
#include <vector>

#include <string>

namespace qftbx {

/**
 * @brief The sweep grid of every uncertain parameter, keyed by NAME.
 *
 * Held by value, so that a signature says who owns it.
 *
 * Keyed by name and not by pointer identity because a clone() or a project
 * reload gives the same parameter a new address.
 *
 * std::map rather than QHash so that iteration order is deterministic: the
 * sweep is expected to be bit-reproducible, and an unordered container is one
 * more thing that could quietly stop being so.
 */
using ParameterGrids = std::map<std::string, std::vector<double>>;

}

#endif
