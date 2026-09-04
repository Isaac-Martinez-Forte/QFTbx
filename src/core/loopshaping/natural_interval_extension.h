#ifndef QFTBX_NATURAL_INTERVAL_EXTENSION_H
#define QFTBX_NATURAL_INTERVAL_EXTENSION_H

#include <vector>

#include "src/core/system/parameter.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/point_controller.h"

#include <interval.hpp>
#include <cinterval.hpp>

namespace qftbx {

/**
 * @brief Natural interval extension of a controller's frequency response
 * onto the Nichols plane (thesis section 1.2.5).
 *
 * For a zero-pole-gain controller box
 * \f$ \mathbf{x} = (\mathbf{k}, \mathbf{z}_1 \ldots, \mathbf{p}_1 \ldots) \f$
 * and a nominal plant value \f$ p_0 = P_0(j\omega) \f$, nicholsBox() encloses
 * \f$ L_0(j\omega, x) = k \, p_0 \prod_i (j\omega + z_i) / \prod_j (j\omega + p_j) \f$
 * for every instance \f$ x \in \mathbf{x} \f$ by interval arithmetic:
 * magnitude \f$ 20 \log_{10} |L_0| \f$ (dB) and phase \f$ \angle L_0 \f$
 * mapped onto the \f$ (-360^\circ, 0] \f$ Nichols branch. The fundamental
 * theorem of interval analysis guarantees the enclosure, which every
 * interval loop-shaping algorithm relies on to classify parameter boxes as
 * feasible, ambiguous or infeasible.
 *
 * A phase set that crosses the branch cut (0/-360 degrees) is not a single
 * interval inside the branch: the enclosure degrades to the whole branch,
 * which is conservative but keeps the containment guarantee (the branch
 * mapping this class replaces returned the complementary arc there, i.e.
 * boxes that EXCLUDED true controller phases). The atan2/log endpoints are
 * evaluated in double rounding-to-nearest, as historically; outward
 * rounding of those endpoints is a phase-8c candidate.
 *
 * Only ZeroPoleGain controller structures are supported; other structures
 * throw qftbx::InvalidInput (the historical code silently computed a
 * zero-pole projection for them).
 */
class NaturalIntervalExtension
{
public:
    /// Nichols-plane enclosure of the controller box times the nominal
    /// plant value p0: Re = magnitude interval (dB), Im = phase interval
    /// (degrees, branch (-360, 0]).
    cxsc::cinterval nicholsBox(LtiSystem * controller, double w,
                               cxsc::complex p0);

    /// The same enclosure with the controller's gain replaced by 'gain':
    /// what the gain contractors evaluate at one end of the gain range.
    /// They used to build a whole controller box with that one parameter
    /// changed, once per node and frequency, to ask this question.
    cxsc::cinterval nicholsBox(LtiSystem * controller, double w,
                               cxsc::complex p0, const cxsc::interval & gain);

    /// The enclosure of a single controller: the same arithmetic over
    /// degenerate intervals, so it answers exactly what nicholsBox() answers
    /// for the system built from the point, without building it.
    cxsc::cinterval nicholsPoint(const PointController & point, double w, cxsc::complex p0);
    cxsc::cinterval nicholsPoint(double gain, const std::vector<double> & zeros,
                                 const std::vector<double> & poles, double w, cxsc::complex p0);

    /// The zero and pole products of a box or a point at one frequency: the
    /// part of the enclosure that does not depend on the gain. The gain
    /// contractors and the gain bisections project the same zeros and poles
    /// with one gain interval after another; computing the products once
    /// and finishing with nicholsOf() gives exactly the enclosures
    /// nicholsBox() gives, without repeating the interval products, which
    /// are the dear part.
    struct Factors {
        cxsc::cinterval numerator;
        cxsc::cinterval denominator;
    };

    Factors factorsOf(LtiSystem * controller, double w);
    Factors factorsOf(const std::vector<double> & zeros, const std::vector<double> & poles, double w);

    /// Magnitude (dB) and phase (degrees) of gain * numerator * p0 /
    /// denominator: the enclosure the other projections end with.
    cxsc::cinterval nicholsOf(const cxsc::interval & gain, const Factors & factors, cxsc::complex p0);

    /// Enclosure of the numerator product alone (no gain, no plant),
    /// dB/degrees.
    cxsc::cinterval numeratorBox(std::vector<Parameter> & numerator, double w,
                                 LtiSystem::SystemType type);

    /// Enclosure of the denominator product alone, dB/degrees.
    cxsc::cinterval denominatorBox(std::vector<Parameter> & denominator, double w,
                                   LtiSystem::SystemType type);

    /// Per-term enclosures (dB/degrees) used by the parameter cutting
    /// equations: one numerator factor (jw + z) p0, one denominator factor
    /// p0 / (jw + p), and the gain k p0.
    cxsc::cinterval numeratorTermBox(Parameter & zero, double w, cxsc::complex p0);
    cxsc::cinterval denominatorTermBox(Parameter & pole, double w, cxsc::complex p0);
    cxsc::cinterval gainTermBox(Parameter & gain, cxsc::complex p0);

private:
    /// Interval product of (jw + parameter) factors; the neutral value 1
    /// for an empty vector (a pure-gain controller).
    cxsc::cinterval factorProduct(std::vector<Parameter> & parameters, double w);
    cxsc::cinterval factorProduct(const std::vector<double> & values, double w);

    cxsc::cinterval nicholsOf(const cxsc::interval & gain, const cxsc::cinterval & numerator,
                              const cxsc::cinterval & denominator, cxsc::complex p0);

    /// Enclosure of arg over a complex rectangle, on the (-2*pi, 0] branch.
    cxsc::interval argEnclosure(const cxsc::cinterval & z);

    /// Maps a phase interval from (-2*pi, 2*pi) back onto the (-2*pi, 0]
    /// branch; a set crossing the branch cut yields the whole branch.
    cxsc::interval wrapToBranch(const cxsc::interval & theta);

    /// 20*log10 of a magnitude interval, with the endpoints clamped to
    /// (0, MaxReal] so the conversion stays finite (an overflowing interval
    /// product used to reach log10(+inf), aborting the process in fi_lib).
    cxsc::interval toDecibel(cxsc::interval magnitude);
};

} // namespace qftbx


#endif // QFTBX_NATURAL_INTERVAL_EXTENSION_H
