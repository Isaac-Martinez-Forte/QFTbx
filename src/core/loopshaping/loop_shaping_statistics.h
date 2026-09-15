#ifndef QFTBX_LOOPSHAPING_STATISTICS_H
#define QFTBX_LOOPSHAPING_STATISTICS_H

#include <cstddef>
#include <vector>

namespace qftbx {

/**
 * @brief What a loop-shaping run cost, as counted by the algorithm itself.
 *
 * The counts are what the algorithms already keep for their own purposes,
 * read out at the end: they cost nothing to gather and change no result.
 * The peak of live nodes is what a run costs in memory and what the
 * search's node budget has to be tuned against; the other counts say
 * where the time went and let two runs be compared beyond their clock.
 */
struct LoopShapingStatistics
{
    /// Wall-clock time of solve(), in milliseconds.
    double milliseconds = 0.0;
    /// The most nodes the live list held at once.
    std::size_t peakLiveNodes = 0;
    /// Nodes taken from the head of the live list.
    std::size_t nodesProcessed = 0;
    /// Boxes classified against the boundaries (zero for MR, which works on
    /// constraints rather than boundaries).
    std::size_t boxesClassified = 0;
    /// How those verdicts split. They add up to boxesClassified; the
    /// ambiguous ones are the boxes the search had to keep bisecting, which
    /// is where a permissive or a conservative boundary shows first.
    std::size_t boxesFeasible = 0;
    std::size_t boxesInfeasible = 0;
    std::size_t boxesAmbiguous = 0;
    /// Nominal stability verdicts asked, and how many needed a new profile.
    std::size_t stabilityVerdicts = 0;
    std::size_t stabilityProfiles = 0;

    /// Nodes by depth of the search tree and what each was found to be
    /// (DepthAccounting); empty for an algorithm that does not account.
    struct DepthRow {
        std::size_t nodes = 0;
        std::size_t feasible = 0;
        std::size_t infeasible = 0;
        std::size_t ambiguous = 0;
    };
    std::vector<DepthRow> byDepth;
    /// How many times each design frequency made a box ambiguous.
    std::vector<std::size_t> ambiguousByFrequency;
};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_STATISTICS_H
