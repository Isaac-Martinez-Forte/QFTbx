/**
 * @file
 * @brief The Nichols rectangle of a controller box at one frequency:
 * magnitude in dB and phase in degrees on the (-360, 0] branch, by the
 * natural interval extension of the controller's frequency response (thesis
 * section 1.2.5).
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
 * factors then multiply their magnitudes and add their phases. A product
 * of rectangles instead would grow its shape with every factor, and its
 * phase would have to be read off the corners of the result. The zero and
 * pole products of a box or a point at one frequency, its Factors, are the
 * part of the enclosure that does not depend on the gain: the gain
 * contractors and the gain bisections project the same zeros and poles with
 * one gain interval after another, and computing the products once with
 * factorsOf() and finishing with nicholsOf() gives exactly the enclosures
 * nicholsBox() gives. The product over no factors, that of a pure-gain
 * controller, is one. nicholsBox() takes the gain of the box or another one
 * it is given, and nicholsPoint() encloses a single controller with the same
 * arithmetic over degenerate intervals. The cutting equations of the
 * parameters read one term at a time: a numerator factor (jw + z) p0, a
 * denominator factor p0 / (jw + p), and the gain k p0.
 *
 * The modulus and the phase of the loop are computed apart, by the very
 * operations the polar product would apply to each, so mayReachUnitModulus
 * can ask whether the modulus over a box may be one at a frequency, given
 * the square of the frequency and the modulus of the nominal plant there,
 * from the moduli of the factors alone, and answer as the enclosure would,
 * without the phases. The squares of the box's zeros and poles do not depend
 * on the frequency, so squaresOf forms them once per box, with its gain. The magnitude is converted to dB with its ends clamped
 * to the positive finite doubles, so the conversion stays finite, and the
 * phase is mapped onto the (-2 pi, 0] branch. Since the logarithm is zero
 * only at one, and widening a value that is not zero by a few ulps never
 * crosses zero, the clamped modulus contains one exactly when the decibels
 * contain zero. A phase set that crosses the branch cut
 * (0/-360 degrees) is not a single interval inside the branch: the
 * enclosure degrades to the whole branch, which is conservative but keeps
 * the containment guarantee. Only ZeroPoleGain controller structures are
 * supported; other structures throw qftbx::InvalidInput rather than be
 * projected as if they were.
 */

#ifndef QFTBX_NATURAL_INTERVAL_EXTENSION_H
#define QFTBX_NATURAL_INTERVAL_EXTENSION_H

#include <complex>
#include <vector>

#include "src/core/math/interval.h"
#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"
#include "src/core/loopshaping/common/point_controller.h"

namespace qftbx {

struct NicholsBox
{
    Interval magnitudeDb;
    Interval phaseDegrees;
};

class NaturalIntervalExtension
{
public:
    struct Factors {
        PolarInterval numerator;
        PolarInterval denominator;
    };

    NicholsBox nicholsBox(LtiSystem * controller, double w, std::complex<double> p0);

    NicholsBox nicholsBox(LtiSystem * controller, double w, std::complex<double> p0,
                          const Interval & gain);

    NicholsBox nicholsPoint(const PointController & point, double w, std::complex<double> p0);
    NicholsBox nicholsPoint(double gain, const std::vector<double> & zeros,
                            const std::vector<double> & poles, double w, std::complex<double> p0);

    Factors factorsOf(LtiSystem * controller, double w);
    Factors factorsOf(const std::vector<double> & zeros, const std::vector<double> & poles, double w);

    NicholsBox nicholsOf(const Interval & gain, const Factors & factors, std::complex<double> p0);

    struct BoxSquares {
        std::vector<Interval> zeros;
        std::vector<Interval> poles;
        Interval gain;
    };

    BoxSquares squaresOf(LtiSystem * controller);

    bool mayReachUnitModulus(const BoxSquares & box, const Interval & wSquared, const Interval & nominalModulus);

    NicholsBox numeratorTermBox(Parameter & zero, double w, std::complex<double> p0);
    NicholsBox denominatorTermBox(Parameter & pole, double w, std::complex<double> p0);
    NicholsBox gainTermBox(Parameter & gain, std::complex<double> p0);

private:
    PolarInterval factorProduct(std::vector<Parameter> & parameters, double w);
    PolarInterval factorProduct(const std::vector<double> & values, double w);

    NicholsBox toNichols(const PolarInterval & loop);
};

}

#endif
