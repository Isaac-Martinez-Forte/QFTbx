#include "src/core/loopshaping/algorithm_name.h"

#include <vector>

namespace qftbx {

const std::vector<LoopShapingAlgorithm> & everyAlgorithm()
{
    static const std::vector<LoopShapingAlgorithm> all{nt, nk, mr, mc1, mc_thesis, mc2, mc3};
    return all;
}

const char * algorithmName(LoopShapingAlgorithm algorithm)
{
    switch (algorithm) {
    case nt: return "nt";
    case nk: return "nk";
    case mr: return "mr";
    case mc1: return "mc1";
    case mc_thesis: return "mc_thesis";
    case mc2: return "mc2";
    case mc3: return "mc3";
    }
    return "unknown";
}

std::optional<LoopShapingAlgorithm> algorithmFromName(const std::string & name)
{
    for (const LoopShapingAlgorithm algorithm : everyAlgorithm()) {
        if (name == algorithmName(algorithm)) {
            return algorithm;
        }
    }
    return std::nullopt;
}

} // namespace qftbx
