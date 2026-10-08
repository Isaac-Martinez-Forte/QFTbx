/**
 * @file
 * @brief Algorithm MC3, under development: a trial that keeps the gain out
 * of the search tree.
 *
 * A branch and bound over the zeros and poles alone. A node is a box of
 * zeros and poles with the gains, in dB, still admissible for it, and its
 * list index is the lower bound. At unit gain the boundary columns give
 * the gains certainly feasible and certainly forbidden for the box; the
 * exact admissible gains of one point controller are those of MC2's best
 * gain, a union of intervals. A box is split in its widest parameter on a
 * logarithmic scale, and a certified controller replaces the best one when
 * it improves it. solve() returns false only when the structure has
 * nothing to search, and throws qftbx::InvalidInput when no gain is
 * feasible. It runs only from the benchmark and is not part of the
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

class AlgorithmMc3
{
public:
    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega,
                    const BoundaryData * boundaries, double epsilon);

    void setCancellation(const qftbx::CancellationToken * token) { m_cancellation = token; }

    void setSettings(const qftbx::Settings & settings) { m_settings = settings; }

    void setPlantFamily(qftbx::ParameterGrids sweep) { m_sweep = std::move(sweep); }

    bool solve();

    std::unique_ptr<LtiSystem> controllerStructure();

    LoopShapingStatistics statistics() const;

private:
    struct GainSets {
        RangeUnion certified;
        RangeUnion forbidden;
        bool small = true;
        std::size_t ambiguousFrequencies = 0;
    };

    class Node : public SearchNode
    {
    public:
        Node(double lowerBoundDb, std::unique_ptr<LtiSystem> box, RangeUnion gains)
            : SearchNode(lowerBoundDb, std::move(box)), gains(std::move(gains)) {}
        RangeUnion gains;
        std::optional<GainSets> sets;
        bool unstableChecked = false;
    };

    GainSets gainSetsOf(LtiSystem * box);

    static RangeUnion complementOf(const RangeUnion & set);
    static RangeUnion unionOf(const RangeUnion & a, const RangeUnion & b);

    std::unique_ptr<LtiSystem> withGains(LtiSystem * box, const RangeUnion & gains) const;

    std::pair<std::unique_ptr<LtiSystem>, std::unique_ptr<LtiSystem>> bisect(LtiSystem * box) const;

    static PointController centreOf(LtiSystem * box, double gainDb);

    RangeUnion pointGainSet(const PointController & point);

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
