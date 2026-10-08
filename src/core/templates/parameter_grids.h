/**
 * @file
 * @brief The sweep grid of every uncertain parameter, keyed by name.
 *
 * Keyed by name and not by object, so that a grid survives a copy of the
 * plant or a reload of the project, and held by value. An ordered map, so
 * that the iteration order is deterministic: the sweep is expected to be
 * reproducible to the bit, and an unordered container would not keep it.
 */

#ifndef QFTBX_PARAMETER_GRIDS_H
#define QFTBX_PARAMETER_GRIDS_H

#include <map>
#include <vector>

#include <string>

namespace qftbx {

using ParameterGrids = std::map<std::string, std::vector<double>>;

}

#endif
