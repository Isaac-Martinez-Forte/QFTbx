#ifndef QFTBX_LOOPSHAPING_ALGORITHM_NT_H
#define QFTBX_LOOPSHAPING_ALGORITHM_NT_H

#include <cstdint>
#include <vector>
#include <QHash>
#include <cmath>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/loopshaping/search_node.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
#include "src/core/loopshaping/nominal_stability_checker.h"
#include "src/core/loopshaping/ordered_list.h"

#include "src/core/loopshaping/common_functions.h"


/**
 * @brief Algorithm NT (Nataraj and Tharewal): QFT loop shaping as an
 * interval branch & bound over the controller parameter box.
 *
 * The base algorithm of the five, and the one the other four extend. The
 * reference is Tharewal's doctoral thesis (2005), ch. 3 for the algorithm
 * and ch. 5 for the constraint-propagation acceleration; the section
 * numbers cited throughout the implementation are its.
 *
 * The design problem is a global minimisation of the controller gain over
 * the box of its parameters, subject to the QFT boundaries. A box of
 * parameters is a set of controllers, so its loop transmission
 * \f$ L_0 = P_0 C \f$ is a region of the Nichols chart: the natural
 * interval extension encloses it in a rectangle
 * (NaturalIntervalExtension), and comparing that rectangle against the
 * boundary union at every design frequency
 * (BoundaryViolationDetector) classifies the whole box at once as
 * certainly infeasible, certainly feasible, or ambiguous.
 *
 * That classification is what makes the search a proof rather than a
 * sampling: a certainly infeasible box is discarded whole, and a
 * certainly feasible box realises its optimum at its lower gain corner,
 * so the FIRST feasible box to reach the head of a list ordered by
 * \f$ \inf(k) \f$ holds the global optimum. Ambiguous boxes are bisected
 * along their widest parameter and re-classified (sec. 3.3.3, steps 1-7).
 * The search terminates on that feasible head, or on a head narrower than
 * epsilon at every frequency (Remark 3.1), from which the feasible corner
 * is extracted.
 *
 * Chapter 5 adds two geometric contractors that cut certified subranges
 * of the gain instead of bisecting them, using the monotonicity of
 * \f$ |L_0| \f$ in the gain: C_g- removes the low-gain subrange lying
 * entirely below the minimum boundary magnitude over the box's phase
 * interval, and C_g+ splits off the high-gain subrange lying entirely
 * above the maximum. Both are certified by the parity classification of
 * the corresponding corner, so neither depends on a heuristic for
 * correctness.
 *
 * QFTbx deviations, documented:
 * - Nominal closed-loop stability is checked with the Nichols-chart
 *   Nyquist criterion (NominalStabilityChecker) rather than by the zeros
 *   of \f$ 1 + L_0 \f$: satisfied stability bounds plus one nominally
 *   stable point make a bounds-feasible box robustly stable (sec. 3.3.5),
 *   and an unstable point discards it. Without this, an ACC'90-style
 *   marginally unstable plant drives the honest search to the bottom of
 *   the gain box, which the QFT boundaries alone do not exclude. The
 *   criterion presumes a nominal plant with no right-half-plane poles.
 * - The C_g+ split is re-certified by the same box test it came from, so
 *   its heuristic gate cannot compromise the result; degenerate slivers
 *   are skipped because they would only bloat the list.
 * - The historical implementation carried a penalty outside the paper
 *   (at \f$ \omega = 2 \f$ with \f$ \sup(\Im) < -180 \f$, the list key
 *   became \f$ \inf(k) + 100 \f$), which broke the optimality guarantee.
 *   It was a hand-made version of the nominal stability rule above, and
 *   it is gone.
 */
class AlgorithmNt
{
public:
    AlgorithmNt();
    ~AlgorithmNt();

    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> *omega, const BoundaryData * boundaries,
                    double epsilon);

    bool solve();

    /// The designed controller, handed over to the caller.
    std::unique_ptr<LtiSystem> controllerStructure();

    /// The most boxes the search kept alive at once (see kDefaultMaxLiveNodes).
    std::size_t peakLiveNodes() const;


private:

    inline void check_box_feasibility(std::unique_ptr<LtiSystem> box);
    inline std::unique_ptr<LtiSystem> acelerated(std::unique_ptr<LtiSystem> v, double minBoundary,
                                                 double o, std::int32_t frequencyIndex, bool above);
    inline bool feasibleGainFrom(LtiSystem * v, double maxBoundary, cxsc::cinterval projection,
                                 double o, std::int32_t frequencyIndex, double & from);

    LtiSystem * plant = nullptr;
    std::unique_ptr<LtiSystem> controller;
    std::vector <double> * omega = nullptr;
    const BoundaryData * boundaries = nullptr;
    std::unique_ptr<NaturalIntervalExtension> conversion;
    std::unique_ptr<OrderedList> liveList;
    double epsilon = 0.0;

    std::unique_ptr<LtiSystem> designedController;
    double minBoundary = 0.0;




    std::int32_t tamFas = 0;

    std::unique_ptr<BoundaryViolationDetector> detector;
    std::unique_ptr<NominalStabilityChecker> stability;
    std::vector <complex> nominalPlantValues;

};

#endif // QFTBX_LOOPSHAPING_ALGORITHM_NT_H
