#ifndef QFTBX_LOOPSHAPING_DEPTH_ACCOUNTING_H
#define QFTBX_LOOPSHAPING_DEPTH_ACCOUNTING_H

#include <cmath>
#include <cstddef>
#include <vector>

#include "src/core/loopshaping/loop_shaping_statistics.h"
#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/system/lti_system.h"

namespace qftbx {

/**
 * @brief Where in the search tree the nodes are, and what they are found to
 * be: one row per depth, and the frequencies that make boxes ambiguous.
 *
 * Things to keep in mind:
 * - Nodes carry no depth; it is read off the box itself as the number of
 *   halvings that separate it from the initial box, summed over the
 *   variable parameters: log2 of the width ratio per parameter, rounded.
 *   A cut that shrinks a parameter without halving it counts as a fraction
 *   of a level, which is what it is.
 * - It measures; it decides nothing. Off the hot path: a few logarithms per
 *   node against thousands of interval operations.
 */
class DepthAccounting
{
public:
    void start(LtiSystem & initial)
    {
        m_initialWidths.clear();
        collect(initial, m_initialWidths);
        m_byDepth.clear();
        m_ambiguousByFrequency.clear();
    }

    std::size_t depthOf(LtiSystem & box) const
    {
        std::vector<double> widths;
        collect(box, widths);
        double levels = 0.0;
        for (std::size_t i = 0; i < widths.size() && i < m_initialWidths.size(); ++i) {
            if (widths[i] > 0.0 && m_initialWidths[i] > 0.0) {
                levels += std::log2(m_initialWidths[i] / widths[i]);
            }
        }
        return levels > 0.0 ? static_cast<std::size_t>(std::lround(levels)) : 0;
    }

    void record(LtiSystem & box, BoxFlag verdict)
    {
        const std::size_t d = depthOf(box);
        if (m_byDepth.size() <= d) {
            m_byDepth.resize(d + 1);
        }
        LoopShapingStatistics::DepthRow & row = m_byDepth[d];
        ++row.nodes;
        if (verdict == feasible) ++row.feasible;
        else if (verdict == infeasible) ++row.infeasible;
        else ++row.ambiguous;
    }

    void ambiguousAt(std::size_t frequency)
    {
        if (m_ambiguousByFrequency.size() <= frequency) {
            m_ambiguousByFrequency.resize(frequency + 1, 0);
        }
        ++m_ambiguousByFrequency[frequency];
    }

    void fill(LoopShapingStatistics & statistics) const
    {
        statistics.byDepth = m_byDepth;
        statistics.ambiguousByFrequency = m_ambiguousByFrequency;
    }

private:
    static void collect(LtiSystem & system, std::vector<double> & widths)
    {
        for (Parameter & p : system.numerator()) if (p.isUncertain()) widths.push_back(p.range().width());
        for (Parameter & p : system.denominator()) if (p.isUncertain()) widths.push_back(p.range().width());
        if (system.gain().isUncertain()) widths.push_back(system.gain().range().width());
    }

    std::vector<double> m_initialWidths;
    std::vector<LoopShapingStatistics::DepthRow> m_byDepth;
    std::vector<std::size_t> m_ambiguousByFrequency;
};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_DEPTH_ACCOUNTING_H
