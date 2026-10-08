#ifndef QFTBX_LOOPSHAPING_ALGORITHM_NT_H
#define QFTBX_LOOPSHAPING_ALGORITHM_NT_H

#include "src/core/project/settings.h"
#include "src/core/pipeline/cancellation.h"
#include "src/core/loopshaping/loop_shaping_statistics.h"
#include <cstdint>
#include <vector>
#include <cmath>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/loopshaping/common/search_node.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"
#include "src/core/loopshaping/common/depth_accounting.h"
#include "src/core/loopshaping/common/nominal_stability_checker.h"
#include "src/core/loopshaping/common/family_stability_checker.h"
#include "src/core/loopshaping/common/ordered_list.h"

#include "src/core/loopshaping/common/common_functions.h"

/**
 * @file
 * @brief Algorithm NT (Nataraj and Tharewal): QFT loop shaping as an
 * interval branch and bound over the controller parameter box.
 *
 * The base algorithm the others extend. The reference is Tharewal's
 * doctoral thesis (2005), ch. 3 for the algorithm and ch. 5 for the
 * constraint-propagation acceleration, whose section numbers the
 * implementation cites. The gain is minimised over the box of the
 * controller's parameters subject to the QFT boundaries. A box is a set of
 * controllers, so its loop \f$ L_0 = P_0 C \f$ is a region of the Nichols
 * chart: the natural interval extension encloses it in a rectangle
 * (NaturalIntervalExtension), and comparing that rectangle with the
 * boundaries at every design frequency (BoundaryViolationDetector)
 * classifies the whole box as certainly infeasible, certainly feasible or
 * ambiguous.
 *
 * A certainly infeasible box is discarded whole, and a certainly feasible
 * one realises its optimum at its lower gain corner, so the first feasible
 * box at the head of a list ordered by \f$ \inf(k) \f$ holds the global
 * optimum; nothing biases that key. Ambiguous boxes are bisected along
 * their widest parameter and classified again (sec. 3.3.3, steps 1-7), and
 * the search ends on the feasible head or on a head narrower than epsilon
 * at every frequency (Remark 3.1), from which the feasible corner is taken.
 *
 * Chapter 5's contractors cut certified subranges of the gain instead of
 * bisecting them, by the monotonicity of \f$ |L_0| \f$ in the gain: C_g-
 * removes the low gains entirely below the minimum boundary magnitude over
 * the box's phase interval, and C_g+ splits off the high gains entirely
 * above the maximum, each certified by the classification of its corner.
 *
 * Nominal stability is checked with the Nichols-chart Nyquist criterion
 * (NominalStabilityChecker) rather than by the zeros of \f$ 1 + L_0 \f$:
 * satisfied stability bounds plus one nominally stable point make a
 * bounds-feasible box robustly stable (sec. 3.3.5). A whole ambiguous box
 * is discarded when one member is unstable and its enclosure excludes the
 * critical point at every frequency of the checker's grid. The
 * cancellation token has to outlive solve(), and without the grids of the
 * sweep the family is left out.
 */
namespace qftbx {

class AlgorithmNt
{
public:
    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> *omega, const BoundaryData * boundaries,
                    double epsilon);

    void setCancellation(const qftbx::CancellationToken * token)
    { m_cancellation = token; }

    void setSettings(const qftbx::Settings & settings) { m_settings = settings; }

    void setPlantFamily(qftbx::ParameterGrids sweep) { m_sweep = std::move(sweep); }

    bool solve();

    std::unique_ptr<LtiSystem> controllerStructure();

    LoopShapingStatistics statistics() const;

private:

    void check_box_feasibility(std::unique_ptr<LtiSystem> box);
    std::unique_ptr<LtiSystem> accelerated(std::unique_ptr<LtiSystem> v, double minBoundary,
                                          const NaturalIntervalExtension::Factors & factors,
                                          std::size_t frequencyIndex, bool above);
    bool feasibleGainFrom(LtiSystem * v, double maxBoundary, NicholsBox projection,
                          const NaturalIntervalExtension::Factors & factors,
                          std::size_t frequencyIndex, double & from);

    LtiSystem * plant = nullptr;
    std::unique_ptr<LtiSystem> controller;
    std::vector <double> * omega = nullptr;
    const BoundaryData * boundaries = nullptr;
    std::unique_ptr<NaturalIntervalExtension> conversion;
    std::unique_ptr<OrderedList> liveList;
    double epsilon = 0.0;

    std::unique_ptr<LtiSystem> designedController;

    std::unique_ptr<BoundaryViolationDetector> detector;

    DepthAccounting depthAccounting;
    std::unique_ptr<NominalStabilityChecker> stability;
    std::unique_ptr<FamilyStabilityChecker> family;
    qftbx::ParameterGrids m_sweep;
    std::vector<std::complex<double>> nominalPlantValues;

    const qftbx::CancellationToken * m_cancellation = nullptr;

    qftbx::Settings m_settings;

};

}

#endif
