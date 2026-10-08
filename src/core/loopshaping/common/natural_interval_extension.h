/**
 * @file
 * @brief The Nichols rectangle of a controller box at one frequency:
 * magnitude in dB and phase in degrees on the (-360, 0] branch, by the
 * natural interval extension of the controller's frequency response (thesis
 * section 1.2.5).
 *
 * For a zero-pole-gain controller box and a nominal plant value
 * \f$ p_0 = P_0(j\omega) \f$, nicholsBox() encloses
 * \f$ L_0(j\omega, x) = k \, p_0 \prod_i (j\omega + z_i) / \prod_j (j\omega + p_j) \f$
 * for every instance of the box, which is what the interval algorithms
 * classify boxes with. The product is assembled in polar form: each factor
 * \f$ j\omega + \mathbf{z} \f$ is a horizontal segment whose magnitude and
 * phase ranges are exact, and the factors multiply their magnitudes and add
 * their phases. The zero and pole products, the Factors, do not depend on
 * the gain: factorsOf() computes them once and nicholsOf() finishes them
 * with a gain interval, exactly as nicholsBox() does. nicholsPoint()
 * encloses one controller, and the three term boxes are the single factors
 * the cutting equations read.
 *
 * mayReachUnitModulus asks whether the modulus over a box may be one at a
 * frequency from the moduli of the factors alone, as the enclosure would
 * answer, with the squares of the box's parameters formed once by
 * squaresOf. The magnitude is converted to dB with its ends clamped to the
 * positive finite doubles, and the phase is mapped onto the (-2 pi, 0]
 * branch; a phase that crosses the cut at 0/-360 degrees becomes the whole
 * branch. Only zero-pole-gain structures are supported, and others throw
 * qftbx::InvalidInput.
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
