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
 * @brief Algorithm MC3: branch and bound over the zeros and poles with the
 * gain treated exactly, never bisected.
 *
 * The loop's phase does not depend on the gain and its magnitude in dB is
 * the gain in dB plus a term that does not depend on it. So for a box B of
 * zeros and poles, projected at unit gain to the rectangle
 * [m_lo, m_hi] x [phi_lo, phi_hi] at each design frequency, the boundary
 * columns give EXACTLY, as unions of intervals of g = 20 log10 k:
 *
 * - the gains for which the whole box is certainly feasible at every
 *   frequency (the shifted rectangle fits inside an allowed interval of
 *   every column its phase span covers): G_in(B);
 * - the gains for which the whole box is certainly infeasible at some
 *   frequency (the shifted rectangle lies inside a forbidden gap of every
 *   column of the span): G_inf(B).
 *
 * A node carries its box and its set K of admissible gains, a union of
 * intervals. Each visit contracts K by G_inf(B); the node's lower bound is
 * min K; G_in(B) intersected with K yields a controller certified for the
 * WHOLE box (upper bound) as soon as the box is narrower than the allowed
 * corridor. Nodes whose lower bound cannot beat the best certified gain by
 * the tolerance are discarded. Only the zeros and poles are bisected, on a
 * logarithmic scale, the widest first.
 *
 * This generalises the gain contractors of NT (C_g-, C_g+), the Quick
 * Solution of NK and its mirror in the thesis, and the exact best gain of
 * MC2 (a point), to the whole box, both sides, every gap of every column,
 * and it removes the gain from the search tree altogether.
 *
 * The tolerance 'epsilon' plays two roles, both in the Nichols plane: a
 * node is discarded when its lower bound is within epsilon dB of the best
 * certified gain, and a box is not bisected further when its unit-gain
 * projection is narrower than epsilon in dB and degrees at every frequency.
 *
 * Measurements that motivated it: documentos/Tesis (para-retomar/15).
 *
 * @author Isaac Martínez Forte
 */
class AlgorithmMc3
{
public:
    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega,
                    const BoundaryData * boundaries, double epsilon);

    void setCancellation(const qftbx::CancellationToken * token) { m_cancellation = token; }

    void setSettings(const qftbx::Settings & settings) { m_settings = settings; }

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
        RangeUnion certified;   //G_in(B), dB
        RangeUnion forbidden;   //G_inf(B), dB
        bool small = true;      //every projection narrower than epsilon
        std::size_t ambiguousFrequencies = 0;
    };

    class Node : public SearchNode
    {
    public:
        Node(double lowerBoundDb, std::unique_ptr<LtiSystem> box, RangeUnion gains)
            : SearchNode(lowerBoundDb, std::move(box)), gains(std::move(gains)) {}
        RangeUnion gains;
        //Filled on the first visit; a node re-queued with its own, higher,
        //lower bound is not projected again.
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

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_ALGORITHM_MC3_H
