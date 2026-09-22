/**
 * @file
 * @brief What a loop-shaping run cost, as counted by the algorithm itself.
 *
 * The counts are what the algorithms already keep for their own purposes,
 * read out at the end: they cost nothing and change no result. The wall time
 * of the search; the most nodes the live list held at once, which is what a
 * run costs in memory and what the node budget is sized from; the nodes
 * taken from the head; the boxes classified against the boundaries and how
 * the verdicts split, the ambiguous ones being the boxes that had to be
 * bisected again; the nominal stability verdicts asked and the profiles they
 * needed; the nodes by depth of the tree and how often each design frequency
 * left a box ambiguous.
 */

#ifndef QFTBX_LOOPSHAPING_STATISTICS_H
#define QFTBX_LOOPSHAPING_STATISTICS_H

#include <cstddef>
#include <vector>

namespace qftbx {

struct LoopShapingStatistics
{
    double milliseconds = 0.0;
    std::size_t peakLiveNodes = 0;
    std::size_t nodesProcessed = 0;
    std::size_t boxesClassified = 0;
    std::size_t boxesFeasible = 0;
    std::size_t boxesInfeasible = 0;
    std::size_t boxesAmbiguous = 0;
    std::size_t stabilityVerdicts = 0;
    std::size_t stabilityProfiles = 0;

    struct DepthRow {
        std::size_t nodes = 0;
        std::size_t feasible = 0;
        std::size_t infeasible = 0;
        std::size_t ambiguous = 0;
    };
    std::vector<DepthRow> byDepth;
    std::vector<std::size_t> ambiguousByFrequency;
};

}

#endif
