/**
 * @file
 * @brief The names of the loop-shaping algorithms, and the list of them.
 *
 * One place where an algorithm and the short name the file, the benchmark
 * plan and the command line use for it are matched, in both directions:
 * "nt", "nk", "mr", "mc1", "mc_thesis", "mc2", "mc3". algorithmFromName()
 * returns nothing for a name no algorithm has, and everyAlgorithm() lists
 * them all in that order.
 */

#ifndef QFTBX_ALGORITHM_NAME_H
#define QFTBX_ALGORITHM_NAME_H

#include <optional>
#include <string>
#include <vector>

#include "src/core/loopshaping/loop_shaping_types.h"

namespace qftbx {

const char * algorithmName(LoopShapingAlgorithm algorithm);

std::optional<LoopShapingAlgorithm> algorithmFromName(const std::string & name);

const std::vector<LoopShapingAlgorithm> & everyAlgorithm();

}

#endif
