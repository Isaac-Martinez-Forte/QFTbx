#ifndef QFTBX_NOMINAL_STABILITY_CHECKER_H
#define QFTBX_NOMINAL_STABILITY_CHECKER_H

#include <complex>
#include <vector>

#include <QVector>

#include "src/core/system/lti_system.h"

namespace qftbx {

/**
 * @brief Nominal closed-loop stability test for loop-shaping candidates,
 * by the Nyquist criterion on the Nichols chart (Cohen, Chait and Yaniv,
 * "Stability analysis using Nichols charts", Int. J. Robust and Nonlinear
 * Control, 1994 - the criterion the Matlab QFT Toolbox applies).
 *
 * For a nominal open loop \f$ L_0(j\omega) = C(j\omega) P_0(j\omega) \f$
 * with NO poles in the open right half-plane, the closed loop is stable
 * if and only if the net number of signed crossings of the rays
 * \f$ \{\angle L_0 \equiv -180^\circ \ (mod\ 360^\circ),\ |L_0| > 0\ dB\} \f$
 * is zero. A curve starting on a ray (two or more integrators) counts a
 * half crossing towards its departure side.
 *
 * The QFT bound constraints alone do not exclude loops that encircle the
 * critical point: a loop with \f$ |L_0| \gg 1 \f$ beyond \f$ -180^\circ \f$
 * satisfies every magnitude bound and is still closed-loop unstable. The
 * interval algorithms therefore complete their feasibility test with this
 * check on one point controller of each bounds-feasible box: by the
 * boundary crossing principle (Tharewal 2005, sec. 3.3.5), satisfied
 * stability bounds plus one nominally stable point make the whole box,
 * and the whole plant family, robustly stable. (This replaces a
 * historical hard-coded penalty at 2 rad/s that only biased the search.)
 *
 * PRECONDITION (documented, not verifiable here): the nominal plant has
 * no poles in the open right half-plane, and marginal poles on the
 * imaginary axis are lightly damped, as the thesis benchmarks do.
 *
 * The nominal plant response is sampled once on a logarithmic frequency
 * grid three decades beyond the design frequencies on both sides, refined
 * adaptively wherever the loop phase turns faster than the unwrapping
 * tolerance. The controller is evaluated in zero-pole-gain semantics,
 * matching the projection the optimiser used.
 */
class NominalStabilityChecker
{
public:
    NominalStabilityChecker(LtiSystem * nominalPlant, QVector<double> * omega);

    /// Nyquist-on-Nichols verdict for a POINT controller (every parameter
    /// at its nominal value). Returns false when the criterion cannot be
    /// decided (a crossing budget exhausted or the loop not proper), which
    /// conservatively discards the candidate.
    bool isNominallyStable(LtiSystem * pointController);

private:
    std::complex<double> plantAt(double w);
    static std::complex<double> controllerAt(LtiSystem * controller, double w);

    LtiSystem * m_plant;

    //Cached nominal plant samples over the base grid.
    std::vector<double> m_frequencies;
    std::vector<std::complex<double>> m_plantValues;
};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::NominalStabilityChecker;

#endif // QFTBX_NOMINAL_STABILITY_CHECKER_H
