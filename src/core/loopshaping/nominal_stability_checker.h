#ifndef QFTBX_NOMINAL_STABILITY_CHECKER_H
#define QFTBX_NOMINAL_STABILITY_CHECKER_H

#include "src/core/project/settings.h"

#include <complex>
#include <cstddef>
#include <unordered_map>
#include <vector>

#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/point_controller.h"

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
 *
 * How the verdict is computed. The searches ask for hundreds of thousands
 * of verdicts, each over thousands of samples, and this check was most of
 * their running time. Two facts keep it cheap without touching the
 * criterion:
 *
 * - The phase of the loop does not depend on the gain, and the gain scales
 *   every magnitude by the same factor. Everything the criterion reads
 *   that does not depend on the gain - the refinement of the grid, where
 *   the curve crosses the rays and with what magnitude at unit gain,
 *   whether it starts on a ray, the magnitude of its last sample - is
 *   computed once per set of zeros and poles into a Profile, and a verdict
 *   for a gain is a pass over the few crossings of the profile. The
 *   profiles are cached, since the searches ask about the same corner
 *   again and again with other gains.
 * - A profile is computed without an arc tangent per sample. The samples
 *   are kept as arrays of real and imaginary parts, the phase-step test of
 *   the refinement compares the dot product of two consecutive samples
 *   with their magnitudes, and a ray crossing is a change of sign of the
 *   imaginary part on the negative half-plane; the arc tangent is taken
 *   only at the crossings themselves, where the criterion interpolates the
 *   phase, and at the first sample.
 */
class NominalStabilityChecker
{
public:
    /**
     * @brief Builds the checker and samples the nominal loop.
     * @param tolerances the resolution of the sampling, from the settings.
     *        They trade time against how reliably a verdict is reached; the
     *        criterion itself is not among them.
     */
    NominalStabilityChecker(LtiSystem * nominalPlant, std::vector<double> * omega,
                            Settings::Stability tolerances = Settings::Stability());

    /// Nyquist-on-Nichols verdict for a POINT controller (every parameter
    /// at its nominal value). Returns false when the criterion cannot be
    /// decided (a crossing budget exhausted or the loop not proper), which
    /// conservatively discards the candidate.
    bool isNominallyStable(LtiSystem * pointController);

    /// The same verdict for a point given as its values, which is how the
    /// searches hold their candidates before one becomes a result.
    bool isNominallyStable(const PointController & point);

    /// One signed crossing of a -180 degree ray, with the loop magnitude at
    /// the crossing for a gain of one.
    struct Crossing {
        double magnitudeAtUnitGain;
        double sign;
    };

    /// What the criterion reads of a loop that does not depend on the
    /// gain: see the class description.
    struct Profile {
        /// False when the refinement budget ran out: the verdict is then
        /// "not decided", which the searches treat as unstable.
        bool decided = false;
        /// |L| of the last sample at unit gain; the loop is not proper
        /// when the gain brings it to 0 dB or above.
        double lastMagnitudeAtUnitGain = 0.0;
        /// The first sample sits on a ray (two or more integrators): a half
        /// crossing towards the departure side when its magnitude exceeds
        /// 0 dB.
        bool startsOnRay = false;
        double startMagnitudeAtUnitGain = 0.0;
        double startDirection = 0.0;
        std::vector<Crossing> crossings;
    };

    /// The profile of a controller shape: its zeros and poles, and the sign
    /// of its gain (a negative gain turns the phase by 180 degrees). The
    /// magnitude of the gain plays no part. Cached.
    const Profile & profileOf(const PointController & shape);

    /// The verdict for a profile and a gain magnitude.
    static bool isStable(const Profile & profile, double gainMagnitude);

    /// How many verdicts were asked and how many profiles had to be
    /// computed for them, for the benchmarks.
    struct Statistics {
        std::size_t verdicts = 0;
        std::size_t profilesComputed = 0;
    };
    Statistics statistics() const { return m_statistics; }

private:
    struct KeyHash {
        std::size_t operator()(const std::vector<double> & key) const;
    };

    Profile computeProfile(const PointController & shape);

    /// The loop at one frequency for the shape at unit gain magnitude: the
    /// same operations, in the same order, as the array pass over the grid.
    std::complex<double> loopAt(const PointController & shape, double sign, double w);

    std::complex<double> plantAt(double w);

    /// Whether the phase turns by more than the unwrapping tolerance between
    /// two consecutive samples.
    bool phaseStepExceeded(std::size_t i) const;

    LtiSystem * m_plant;

    /// Grid resolution, from the settings.
    Settings::Stability m_tolerances;

    /// The cosine of the unwrapping tolerance, what the phase-step test
    /// compares against.
    double m_cosMaxPhaseStep = 0.0;

    //Cached nominal plant samples over the base grid.
    std::vector<double> m_frequencies;
    std::vector<double> m_plantRe;
    std::vector<double> m_plantIm;

    //The working curve of one profile, as arrays: frequency, real and
    //imaginary part of the loop at unit gain. Kept between calls so a
    //profile does not allocate.
    std::vector<double> m_w;
    std::vector<double> m_re;
    std::vector<double> m_im;
    std::vector<double> m_key;

    std::unordered_map<std::vector<double>, Profile, KeyHash> m_profiles;
    Statistics m_statistics;
};

} // namespace qftbx

#endif // QFTBX_NOMINAL_STABILITY_CHECKER_H
