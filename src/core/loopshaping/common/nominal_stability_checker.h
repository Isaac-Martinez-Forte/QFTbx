/**
 * @file
 * @brief Nominal closed-loop stability of loop-shaping candidates, by the
 * Nyquist criterion on the Nichols chart.
 *
 * The criterion is Cohen, Chait and Yaniv's (Int. J. Robust and Nonlinear
 * Control, 1994), the one the Matlab QFT Toolbox applies. For a nominal loop
 * \f$ L_0 = C P_0 \f$ the closed loop is stable if and only if the net number
 * of signed crossings of the rays \f$ \angle L_0 \equiv -180^\circ,\ |L_0| > 0\ dB \f$
 * over positive frequencies is \f$ P/2 \f$, half the right half-plane poles of
 * the nominal plant: Nyquist's \f$ N = -P \f$ read on the Nichols chart. The
 * boundaries alone do not exclude loops that encircle the critical point, so
 * the interval algorithms complete their feasibility test with this check on
 * one point of each bounds-feasible box (the boundary crossing principle,
 * Tharewal 2005 sec. 3.3.5).
 *
 * \f$ P \f$ is asked of the plant once, and a plant that cannot place its
 * poles is refused rather than assumed stable. The nominal verdict carries to
 * the family only while every member has the same \f$ P \f$, which the template
 * sweep checks. Poles of the plant on the imaginary axis are an input: there
 * the loop passes through infinity and its phase falls by 180 degrees per pole,
 * the indentation of the Nyquist contour, and the frequencies where that
 * happens come from the plant's own poles, not from the samples. A loop that
 * starts on a ray at a finite magnitude (an odd number of real unstable poles,
 * a negative static gain) counts half a crossing towards its departure side.
 * Two integrators put it on the ray at infinite magnitude, where the
 * indentation around the origin reaches the ray from the higher phase: a loop
 * leaving towards the higher phase does not cross it, one leaving towards the
 * lower phase crosses it whole. The start is read off the low-frequency
 * asymptote \f$ A (j\omega)^m \f$ from the polynomials of the plant and the
 * controller, not off the first sample, which sits a fraction of a degree off
 * the ray: read that way, the ACC'90 double integrator was approved with every
 * closed-loop pole in the right half-plane.
 *
 * The nominal plant is sampled once on a logarithmic grid three decades beyond
 * the design frequencies, refined wherever the loop phase turns faster than
 * the unwrapping tolerance; a verdict that cannot be decided counts as
 * unstable. The phase does not depend on the gain and the gain scales every
 * magnitude alike, so everything gain-independent - the refinement, the ray
 * crossings with their magnitude at unit gain, the start, the last sample - is
 * computed once per set of zeros and poles into a cached Profile, and a verdict
 * for a gain is a pass over its few crossings. A profile needs no arc tangent
 * per sample: the phase step is a dot product and a crossing a change of sign.
 * isBoxUnstable rejects a whole box when one member is unstable and the box's
 * interval enclosure keeps the critical point out at one sample in eight of
 * the base grid, since the crossing count then cannot change inside it.
 */

#ifndef QFTBX_NOMINAL_STABILITY_CHECKER_H
#define QFTBX_NOMINAL_STABILITY_CHECKER_H

#include "src/core/project/settings.h"

#include <complex>
#include <cstddef>
#include <unordered_map>
#include <vector>

#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/common/point_controller.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"

namespace qftbx {

class NominalStabilityChecker
{
public:
    NominalStabilityChecker(LtiSystem * nominalPlant, std::vector<double> * omega,
                            Settings::Stability tolerances = Settings::Stability());

    bool isNominallyStable(LtiSystem * pointController);

    bool isNominallyStable(const PointController & point);

    struct Crossing {
        double magnitudeAtUnitGain;
        double sign;
    };

    struct Profile {
        bool decided = false;
        double lastMagnitudeAtUnitGain = 0.0;
        bool startsOnRay = false;
        double startMagnitudeAtUnitGain = 0.0;
        double startDirection = 0.0;
        std::vector<Crossing> crossings;
    };

    const Profile & profileOf(const PointController & shape);

    bool isStable(const Profile & profile, double gainMagnitude) const;

    int rightHalfPlanePoles() const { return m_rhpPoles; }

    bool isBoxUnstable(LtiSystem * box, NaturalIntervalExtension & extension);

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

    std::complex<double> loopAt(const PointController & shape, double sign, double w);

    std::complex<double> plantAt(double w);

    bool phaseStepExceeded(std::size_t i) const;

    std::size_t axisPolesBetween(double lo, double hi) const;

    LtiSystem * m_plant;

    Settings::Stability m_tolerances;

    double m_cosMaxPhaseStep = 0.0;

    std::vector<double> m_axisPoles;

    int m_rhpPoles = 0;

    std::complex<double> m_plantAtZero;
    bool m_asymptoteKnown = false;
    int m_plantOrderAtZero = 0;
    double m_plantCoefficientAtZero = 1.0;

    std::vector<double> m_frequencies;
    std::vector<double> m_plantRe;
    std::vector<double> m_plantIm;

    std::vector<double> m_w;
    std::vector<double> m_re;
    std::vector<double> m_im;
    std::vector<double> m_key;

    std::unordered_map<std::vector<double>, Profile, KeyHash> m_profiles;
    Statistics m_statistics;
};

}

#endif
