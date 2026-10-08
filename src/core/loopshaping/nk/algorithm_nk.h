#ifndef QFTBX_LOOPSHAPING_ALGORITHM_NK_H
#define QFTBX_LOOPSHAPING_ALGORITHM_NK_H

#include "src/core/project/settings.h"
#include "src/core/pipeline/cancellation.h"
#include "src/core/loopshaping/loop_shaping_statistics.h"
#include <cstdint>
#include <complex>

#include <vector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"
#include "src/core/loopshaping/common/ordered_list.h"
#include "src/core/loopshaping/common/search_node.h"
#include "src/core/loopshaping/common/nominal_stability_checker.h"
#include "src/core/loopshaping/common/family_stability_checker.h"
#include "src/core/math/sequence_vectors.h"

#include "src/core/loopshaping/common/common_functions.h"

/**
 * @file
 * @brief Algorithm NK: the NT branch and bound with Quick Solution cuts,
 * local optimisation and constraint propagation.
 *
 * Paluri/Nataraj and Kubal, "Automatic loop shaping in QFT using hybrid
 * optimization and constraint propagation techniques", Int. J. Robust
 * Nonlinear Control 17:251-264, 2007. The NT branch and bound extended
 * with
 *
 * - Quick Solution (sec. 3.3): before a box enters the live list, the
 *   certainly infeasible subranges of the gain, every zero and every pole
 *   are cut off with the closed-form monotonicity equations
 *   (quick_solution.h), per design frequency, on the latest values.
 * - Local optimisation (sec. 3.2): a coordinate-pattern search launched
 *   from the leading box when its gain infimum differs by more than 10%
 *   from every previous launch point; a feasible local solution prunes
 *   every node whose gain infimum cannot beat it, caps the gain of new
 *   boxes, and stands as the answer if the list empties. Its starting point
 *   comes from the interface, never at random, and the numeric values of
 *   its enumeration are the interface's.
 *
 * Nominal closed-loop stability is checked with the Cohen-Chait-Yaniv
 * criterion on the Nichols chart (NominalStabilityChecker), on the returned
 * point and, over a whole ambiguous box whose enclosure excludes the
 * critical point, at classification (isBoxUnstable, as in NT). The
 * cancellation token has to outlive solve().
 */
namespace qftbx {

class AlgorithmNk
{
public:

    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> *omega, const BoundaryData * boundaries,
                   double epsilon, std::int32_t initialisation);

    void setCancellation(const qftbx::CancellationToken * token)
    { m_cancellation = token; }

    void setSettings(const qftbx::Settings & settings) { m_settings = settings; }

    void setPlantFamily(qftbx::ParameterGrids sweep) { m_sweep = std::move(sweep); }

    bool solve();

    std::unique_ptr<LtiSystem> controllerStructure();

    std::size_t peakLiveNodes() const;

    LoopShapingStatistics statistics() const;

private:

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
    std::unique_ptr<FamilyStabilityChecker> family;
    qftbx::ParameterGrids m_sweep;
    std::unique_ptr<OrderedList> liveList;

    std::vector<std::complex<double>> nominalPlantValues;

    std::unique_ptr<LtiSystem> designedController;
    std::unique_ptr<LtiSystem> prototype;

    double bestLocalGain = 0;
    std::unique_ptr<LtiSystem> bestLocalController;
    std::vector<double> launchGains;

    StartingPoint m_start = Centre;

    const qftbx::CancellationToken * m_cancellation = nullptr;

    qftbx::Settings m_settings;

};

}

#endif
