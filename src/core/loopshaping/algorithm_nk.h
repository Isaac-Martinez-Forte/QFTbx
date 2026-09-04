#ifndef QFTBX_LOOPSHAPING_ALGORITHM_NK_H
#define QFTBX_LOOPSHAPING_ALGORITHM_NK_H

#include "src/core/project/settings.h"
#include "src/core/pipeline/cancellation.h"
#include <cstdint>
#include <complex>

#include <vector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
#include "src/core/loopshaping/ordered_list.h"
#include "src/core/loopshaping/search_node.h"
#include "src/core/loopshaping/nominal_stability_checker.h"
#include "src/core/math/sequence_vectors.h"

#include "src/core/loopshaping/common_functions.h"

/**
 * @brief Algorithm NK: the NT branch & bound with Quick Solution cuts,
 * local optimisation and constraint propagation.
 *
 * Paluri/Nataraj and Kubal, "Automatic loop shaping in QFT
 * using hybrid optimization and constraint propagation techniques",
 * Int. J. Robust Nonlinear Control 17:251-264, 2007): the NT branch &
 * bound extended with
 *
 * - Quick Solution (sec. 3.3): before a box enters the live list, the
 *   certainly infeasible subranges of the gain, every zero and every pole
 *   are cut off with the closed-form monotonicity equations (see
 *   quick_solution.h), applied per design frequency with the latest
 *   updated values.
 * - Local optimization (sec. 3.2): a coordinate-pattern search launched
 *   from the leading box when its gain infimum differs by more than 10%
 *   from every previous launch point; a feasible local solution prunes
 *   every node whose gain infimum cannot beat it, clips the gain range of
 *   new boxes, and stands in as the answer if the list ever empties.
 *
 * The feasibility test is completed with the nominal closed-loop
 * stability check (zeros of 1 + L0, demanded by the paper's problem
 * formulation), implemented on the Nichols chart by the Cohen-Chait-Yaniv
 * criterion (NominalStabilityChecker).
 */
namespace qftbx {

class AlgorithmNk
{
public:

    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> *omega, const BoundaryData * boundaries,
                   double epsilon, std::int32_t initialisation);

    /**
     * @brief Installs the flag the search reads once per node.
     *
     * A pointer, and null by default: a caller that never cancels - every
     * test that drives this algorithm directly - carries on unchanged. The
     * token has to outlive solve().
     */
    void setCancellation(const qftbx::CancellationToken * token)
    { m_cancellation = token; }

    /**
     * @brief The values the user may have changed.
     *
     * The whole struct rather than one setter per value: what an algorithm
     * needs from it is copied here, once, before solve() - so the hot path
     * reads a member and never a configuration lookup. Not calling it leaves
     * the compiled defaults, which is what every existing caller does.
     */
    void setSettings(const qftbx::Settings & settings) { m_settings = settings; }

    bool solve();

    /// The designed controller, handed over to the caller.
    std::unique_ptr<LtiSystem> controllerStructure();

    /// The most boxes the search kept alive at once (see kDefaultMaxLiveNodes).
    std::size_t peakLiveNodes() const;

private:

    //Starting point of the local search. The historical 'random' option
    //made the result non-deterministic and is gone (decision 2026-09-01);
    //the numeric values are the GUI/orchestrator contract.
    enum StartingPoint {Centre = 0, Extremes = 1};

    void check_box_feasibility(std::unique_ptr<LtiSystem> box);
    std::unique_ptr<LtiSystem> quickSolution(std::unique_ptr<LtiSystem> v, double boundMinDb, double w,
                                     std::complex<double> p0);

    void localOptimization(LtiSystem * box);
    double minimalFeasibleGain(const std::vector<double> & zeros, const std::vector<double> & poles,
                                     LtiSystem * box, std::int32_t & budget);
    bool pointIsFeasible(const std::vector<NaturalIntervalExtension::Factors> & factors, double gain);
    std::unique_ptr<LtiSystem> pointSystem(const std::vector<double> & zeros, const std::vector<double> & poles,
                                   double gain);
    void startingPoint(LtiSystem * box, std::vector<double> & zeros,
                              std::vector<double> & poles, double & gain);

    LtiSystem * plant = nullptr;
    std::unique_ptr<LtiSystem> controller;
    std::vector<double> * omega = nullptr;
    const BoundaryData * boundaries = nullptr;
    double epsilon = 0;

    std::unique_ptr<NaturalIntervalExtension> conversion;
    std::unique_ptr<BoundaryViolationDetector> detector;
    std::unique_ptr<NominalStabilityChecker> stability;
    std::unique_ptr<OrderedList> liveList;

    std::vector<cxsc::complex> nominalPlantValues;
    std::vector<std::complex<double>> nominalPlantValuesStd;

    std::unique_ptr<LtiSystem> designedController;
    std::unique_ptr<LtiSystem> prototype;

    //Local optimization state: the best certified feasible gain (prunes
    //the tree), its controller point, and the previous launch points of
    //the 10% decision rule.
    double bestLocalGain = 0;
    std::unique_ptr<LtiSystem> bestLocalController;
    std::vector<double> launchGains;

    //Starting-point strategy of the local search, from the GUI.
    StartingPoint m_start = Centre;

    /// Not owned. Null means this run cannot be cancelled.
    const qftbx::CancellationToken * m_cancellation = nullptr;

    /// Copied whole and read as fields; the defaults are the compiled ones.
    qftbx::Settings m_settings;

};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_ALGORITHM_NK_H
