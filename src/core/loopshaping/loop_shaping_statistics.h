/**
 * @file
 * @brief What a loop-shaping run cost, as counted by the algorithm itself,
 * and what it can claim.
 *
 * The counts are those the algorithms keep for their own purposes, read
 * out at the end: the time of the search, the most nodes alive at once,
 * the nodes taken, the boxes classified and how the verdicts split, the
 * nominal stability verdicts and profiles, the nodes by depth and how
 * often each design frequency left a box ambiguous.
 *
 * The certificate, kept by MC2 alone, is the bookkeeping of what the
 * search removed. A branch and bound proves optimality only if every
 * region it removes is covered by a proof or by a better design, so each
 * removal without a proof is counted with the smallest gain it could have
 * held: the discards read off the boundary columns, the boxes the nominal
 * criterion rejected on its grid, and the residue dropped with no
 * certified point. The removals with a proof, by the Routh table, the zero
 * exclusion, the exact sector verdict or the contraction of the gain, are
 * counted and enter no bound. lowerBound is the smallest gain an unproven
 * removal or a live box could hold, lowerBoundStrict counts the grid's
 * rejections too, and the returned gain is the upper bound; infinity means
 * nothing was left unproven. A certificate is finished when the search
 * reached its end, and one that is not has no bounds. The rest counts what
 * the exact reading costs: the refusals of the funnel, the improvements of
 * the best design, the gain searches with their rounds and ladder steps,
 * and the vertices they gave up on.
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
        bool kept = false;
        bool finished = false;
        bool exactPoints = false;
        double lowerBound = std::numeric_limits<double>::infinity();
        double lowerBoundStrict = std::numeric_limits<double>::infinity();
        std::size_t residueNodes = 0;
        std::size_t epsilonResolved = 0;
        std::size_t unprovenDiscards = 0;
        std::size_t gridBackedPrunes = 0;
        std::size_t familyPrunes = 0;
        std::size_t nominalBoxPrunes = 0;
        std::size_t contractedBoxes = 0;
        std::size_t emptiedBySpecifications = 0;
        std::size_t emptiedByStability = 0;
        std::size_t emptiedByZeroExclusion = 0;
        std::size_t nominalAxisPrunes = 0;
        std::size_t gridPrunesKept = 0;
        std::size_t provenInfeasible = 0;
        std::size_t columnsOverruled = 0;
        std::size_t certifiedCuts = 0;
        std::size_t sectorVerdicts = 0;
        std::size_t certifications = 0;
        std::size_t refusedByRouth = 0;
        std::size_t refusedByNominalRouth = 0;
        std::size_t refusedByNominalStability = 0;
        std::size_t refusedBySpecifications = 0;
        std::size_t refusedByRoots = 0;
        std::size_t incumbentUpdates = 0;
        std::size_t kernelPasses = 0;
        std::size_t gainSearches = 0;
        std::size_t exchangeRounds = 0;
        std::size_t ladderSteps = 0;
        std::size_t laddersExhausted = 0;
        std::size_t roundLimitsReached = 0;
        std::size_t largestWorkingSet = 0;
    };
    Certificate certificate;
};

}

#endif
