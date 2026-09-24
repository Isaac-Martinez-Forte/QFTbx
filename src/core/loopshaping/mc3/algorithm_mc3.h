/**
 * @file
 * @brief Algorithm MC3, under development.
 *
 * A trial of a branch and bound over the zeros and poles alone, with the
 * admissible gains of a box carried as a union of intervals instead of
 * being bisected. It runs only from the benchmark and is not part of the
 * published algorithms.
 */

#ifndef QFTBX_LOOPSHAPING_ALGORITHM_MC3_H
#define QFTBX_LOOPSHAPING_ALGORITHM_MC3_H

#include <complex>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"
#include "src/core/loopshaping/common/depth_accounting.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/loopshaping/common/nominal_stability_checker.h"
#include "src/core/loopshaping/common/family_stability_checker.h"
#include "src/core/loopshaping/common/ordered_list.h"
#include "src/core/loopshaping/common/point_controller.h"
#include "src/core/loopshaping/common/search_node.h"
#include "src/core/loopshaping/loop_shaping_statistics.h"
#include "src/core/math/range_union.h"
#include "src/core/pipeline/cancellation.h"
#include "src/core/project/settings.h"
#include "src/core/system/lti_system.h"

namespace qftbx {

/**
 * @brief Algorithm MC3, under development: a trial that keeps the gain out
 * of the search tree.
 */
class AlgorithmMc3
{
public:
    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega,
                    const BoundaryData * boundaries, double epsilon);

    void setCancellation(const qftbx::CancellationToken * token) { m_cancellation = token; }

    void setSettings(const qftbx::Settings & settings) { m_settings = settings; }

    /**
     * @brief The grids the plant family was swept over, by parameter name:
     * what the search closes the loop with before it returns a design.
     *
     * Empty leaves the check out, which is what a project with no record of
     * its sweep gets.
     */
    void setPlantFamily(qftbx::ParameterGrids sweep) { m_sweep = std::move(sweep); }

    /// Runs the search. Returns false only when the structure has nothing
    /// to search; throws qftbx::InvalidInput when no gain is feasible.
    bool solve();

    /// The designed controller (a point), once solve() returned true.
    std::unique_ptr<LtiSystem> controllerStructure();

    LoopShapingStatistics statistics() const;

private:
    /// A node of the live list: the box of zeros and poles, and the gains
    /// (dB) still admissible for it. The list index is the lower bound.
    /// What the boundary columns say about a box at unit gain.
    struct GainSets {
        RangeUnion certified;   ///< G_in(B), dB
        RangeUnion forbidden;   ///< G_inf(B), dB
        bool small = true;   ///< every projection narrower than epsilon
        std::size_t ambiguousFrequencies = 0;
    };

    class Node : public SearchNode
    {
    public:
        Node(double lowerBoundDb, std::unique_ptr<LtiSystem> box, RangeUnion gains)
            : SearchNode(lowerBoundDb, std::move(box)), gains(std::move(gains)) {}
        RangeUnion gains;
        /// Filled on the first visit; a node re-queued with its own, higher,
        /// lower bound is not projected again.
        std::optional<GainSets> sets;
        bool unstableChecked = false;
    };

    GainSets gainSetsOf(LtiSystem * box);

    static RangeUnion complementOf(const RangeUnion & set);
    static RangeUnion unionOf(const RangeUnion & a, const RangeUnion & b);

    /// The box with its gain parameter narrowed to the hull of 'gains'.
    std::unique_ptr<LtiSystem> withGains(LtiSystem * box, const RangeUnion & gains) const;

    /// The two halves of the box along its widest parameter (log scale).
    std::pair<std::unique_ptr<LtiSystem>, std::unique_ptr<LtiSystem>> bisect(LtiSystem * box) const;

    /// The centre of the box with the given gain in dB.
    static PointController centreOf(LtiSystem * box, double gainDb);

    /// The gains (dB) at which one point controller satisfies every column
    /// its phases fall in, at every frequency: the exact admissible set of
    /// the point (the best-gain theorem of MC2), as a union of intervals.
    RangeUnion pointGainSet(const PointController & point);

    /// Records a certified controller when it improves the best one.
    void certify(LtiSystem * box, double gainDb);

    LtiSystem * plant = nullptr;
    std::unique_ptr<LtiSystem> controller;
    std::vector<double> * omega = nullptr;
    const BoundaryData * boundaries = nullptr;
    double epsilon = 0.5;

    qftbx::Settings m_settings;
    const qftbx::CancellationToken * m_cancellation = nullptr;

    std::unique_ptr<NaturalIntervalExtension> conversion;
    std::unique_ptr<BoundaryViolationDetector> detector;
    std::unique_ptr<NominalStabilityChecker> stability;
    std::unique_ptr<FamilyStabilityChecker> family;
    qftbx::ParameterGrids m_sweep;
    std::unique_ptr<OrderedList> liveList;
    std::vector<std::complex<double>> nominalPlantValues;

    double bestGainDb = 0.0;
    std::unique_ptr<LtiSystem> bestController;
    std::unique_ptr<LtiSystem> designedController;

    DepthAccounting depthAccounting;
    std::size_t m_projections = 0;
    std::size_t m_certificates = 0;
    std::size_t m_emptyGainSets = 0;
    std::size_t m_prunedByBound = 0;
    std::size_t m_ambiguousVisits = 0;
    std::size_t m_unstableBoxes = 0;
    std::size_t m_smallDropped = 0;
};

}

#endif
