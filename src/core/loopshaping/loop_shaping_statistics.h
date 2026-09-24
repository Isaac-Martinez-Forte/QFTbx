/**
 * @file
 * @brief What a loop-shaping run cost, as counted by the algorithm itself,
 * and what it can claim.
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
 *
 * The certificate is the bookkeeping of what the search threw away without
 * proof. A branch and bound proves optimality only if every region it
 * removes is covered by a proof or by a better design; the interval searches
 * remove boxes on the boundary columns of a phase grid, which is not the
 * specification, and prune boxes on the nominal stability of one point of an
 * interval enclosure. So each such removal is counted and the smallest gain
 * it could have held is kept: the unproven discards are the infeasible
 * verdicts and cuts read off the columns, the grid-backed prunes the boxes
 * the nominal criterion rejected whole, and the residue the boxes dropped
 * with no certified point at the size the search stops at, or with a corner
 * the nominal criterion refused. lowerBound is the smallest gain any of them
 * or any box still alive could hold, lowerBoundStrict counts the grid-backed
 * prunes too, and the returned gain is the upper bound; the optimum of the
 * problem posed lies between them, and infinity means nothing was left
 * unproven. Under the exact point reading the candidates asked of the
 * certification funnel, where each refusal fell, how often the best design
 * improved and how many passes over a value set the exact check made say
 * what the reading costs; under the exact gain the searches for the best
 * gain at fixed zeros and poles, the rounds in which a working set of
 * plants had to grow, the ladder steps a rounding cost and the largest
 * working set say what that costs.
 */

#ifndef QFTBX_LOOPSHAPING_STATISTICS_H
#define QFTBX_LOOPSHAPING_STATISTICS_H

#include <cstddef>
#include <limits>
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

    struct Certificate {
        bool exactPoints = false;
        double lowerBound = std::numeric_limits<double>::infinity();
        double lowerBoundStrict = std::numeric_limits<double>::infinity();
        std::size_t residueNodes = 0;
        std::size_t epsilonResolved = 0;
        std::size_t unprovenDiscards = 0;
        std::size_t gridBackedPrunes = 0;
        std::size_t certifications = 0;
        std::size_t refusedBySpecifications = 0;
        std::size_t refusedByNominalStability = 0;
        std::size_t refusedByRouth = 0;
        std::size_t refusedByRoots = 0;
        std::size_t incumbentUpdates = 0;
        std::size_t kernelPasses = 0;
        std::size_t gainSearches = 0;
        std::size_t exchangeRounds = 0;
        std::size_t ladderSteps = 0;
        std::size_t largestWorkingSet = 0;
    };
    Certificate certificate;
};

}

#endif
