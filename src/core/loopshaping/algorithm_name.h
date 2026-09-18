#ifndef QFTBX_ALGORITHM_NAME_H
#define QFTBX_ALGORITHM_NAME_H

// The short name of each loop-shaping algorithm and the way back from it.
// The names are what a benchmark plan, a command line or a file writes when
// it has to say WHICH search ran, so they belong with the algorithms and not
// with any one of the things that spell them.

#include <optional>
#include <string>
#include <vector>

#include "src/core/loopshaping/loop_shaping_types.h"

namespace qftbx {

/// "nt", "nk", "mr", "mc1", "mc_thesis", "mc2", "mc3".
const char * algorithmName(LoopShapingAlgorithm algorithm);

/// The algorithm of that name, or nothing when no algorithm has it.
std::optional<LoopShapingAlgorithm> algorithmFromName(const std::string & name);

/// Every algorithm there is, in the order the names above list them.
const std::vector<LoopShapingAlgorithm> & everyAlgorithm();

} // namespace qftbx

#endif // QFTBX_ALGORITHM_NAME_H
