#ifndef QFTBX_NATURAL_INTERVAL_EXTENSION_H
#define QFTBX_NATURAL_INTERVAL_EXTENSION_H

#include <complex>
#include <vector>

#include "src/core/math/interval.h"
#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"
#include "src/core/loopshaping/point_controller.h"

/**
 * @brief The Nichols rectangle of a controller box at one frequency:
 * magnitude in dB and phase in degrees on the (-360, 0] branch.
 */
namespace qftbx {

struct NicholsBox
{
    Interval magnitudeDb;
    Interval phaseDegrees;
};

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
 * The product is assembled in polar form, as the thesis writes it: each
 * factor \f$ j\omega + \mathbf{z} \f$ is a horizontal segment of the
 * complex plane whose magnitude and phase ranges are read exactly, and the
 * factors then multiply their magnitudes and add their phases. The
 * enclosure used to be computed as a product of rectangles, whose shape
 * grows with every factor, and its phase read off the corners of the final
 * rectangle in plain double arithmetic; both the magnitude and the phase
 * are rigorous intervals now.
 *
 * A phase set that crosses the branch cut (0/-360 degrees) is not a single
 * interval inside the branch: the enclosure degrades to the whole branch,
 * which is conservative but keeps the containment guarantee.
 *
 * Only ZeroPoleGain controller structures are supported; other structures
 * throw qftbx::InvalidInput (the historical code silently computed a
 * zero-pole projection for them).
 */
class NaturalIntervalExtension
{
public:
    /// The zero and pole products of a box or a point at one frequency, in
    /// polar form: the part of the enclosure that does not depend on the
    /// gain. The gain contractors and the gain bisections project the same
    /// zeros and poles with one gain interval after another; computing the
    /// products once and finishing with nicholsOf() gives exactly the
    /// enclosures nicholsBox() gives.
    struct Factors {
        PolarInterval numerator;
        PolarInterval denominator;
    };

    /// Nichols-plane enclosure of the controller box times the nominal
    /// plant value p0.
    NicholsBox nicholsBox(LtiSystem * controller, double w, std::complex<double> p0);

    /// The same enclosure with the controller's gain replaced by 'gain'.
    NicholsBox nicholsBox(LtiSystem * controller, double w, std::complex<double> p0,
                          const Interval & gain);

    /// The enclosure of a single controller: the same arithmetic over
    /// degenerate intervals.
    NicholsBox nicholsPoint(const PointController & point, double w, std::complex<double> p0);
    NicholsBox nicholsPoint(double gain, const std::vector<double> & zeros,
                            const std::vector<double> & poles, double w, std::complex<double> p0);

    Factors factorsOf(LtiSystem * controller, double w);
    Factors factorsOf(const std::vector<double> & zeros, const std::vector<double> & poles, double w);

    /// Magnitude (dB) and phase (degrees) of gain * numerator * p0 /
    /// denominator: the enclosure the other projections end with.
    NicholsBox nicholsOf(const Interval & gain, const Factors & factors, std::complex<double> p0);

    /// Per-term enclosures (dB/degrees) used by the parameter cutting
    /// equations: one numerator factor (jw + z) p0, one denominator factor
    /// p0 / (jw + p), and the gain k p0.
    NicholsBox numeratorTermBox(Parameter & zero, double w, std::complex<double> p0);
    NicholsBox denominatorTermBox(Parameter & pole, double w, std::complex<double> p0);
    NicholsBox gainTermBox(Parameter & gain, std::complex<double> p0);

private:
    /// Polar product of the factors (jw + parameter); the neutral value 1
    /// for an empty vector (a pure-gain controller).
    PolarInterval factorProduct(std::vector<Parameter> & parameters, double w);
    PolarInterval factorProduct(const std::vector<double> & values, double w);

    /// The polar set as a Nichols rectangle: 20 log10 of the magnitude,
    /// with the endpoints clamped to the positive finite doubles so the
    /// conversion stays finite, and the phase mapped onto the (-2 pi, 0]
    /// branch; a set crossing the branch cut yields the whole branch.
    NicholsBox toNichols(const PolarInterval & loop);
};

} // namespace qftbx

#endif // QFTBX_NATURAL_INTERVAL_EXTENSION_H
