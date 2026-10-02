/**
 * @file
 * @brief The closed-loop magnitudes a QFT specification bounds, evaluated
 * over a value set at one loop value.
 *
 * Every specification of the toolbox is a bound on the magnitude of one
 * closed-loop transfer function, taken in the worst case over the plant
 * family; the tracking specification bounds the spread between the worst
 * and the best case of the same magnitude. With the nominal loop
 * \f$ L_0 = G P_0 \f$ and a plant \f$ P \f$ of the family, the loop of that
 * plant is \f$ L = L_0 P / P_0 \f$, and the five magnitudes read
 *
 * - stability and sensor noise: \f$ |L / (1 + L)| = |L_0 / (P_0/P + L_0)| \f$,
 * - output disturbance: \f$ |1 / (1 + L)| = |(P_0/P) / (P_0/P + L_0)| \f$,
 * - input disturbance: \f$ |P / (1 + L)| = |P_0 / (P_0/P + L_0)| \f$,
 * - control effort: \f$ |G / (1 + L)| = |(L_0 / P) / (P_0/P + L_0)| \f$.
 *
 * The boundary sweep evaluates these at every grid point of the Nichols
 * chart to build its sheets (Moreno, Banos and Berenguel 2006, equation
 * (10)); the specification checker evaluates them once, at the loop value of
 * a returned controller, to verify it against the specifications
 * themselves. One definition for both.
 *
 * WorstCase holds, in linear units, the worst case of each magnitude over
 * the value set, and the best case of the tracking magnitude too, since
 * tracking bounds the spread; with them the distance from \f$ -L_0 \f$ to
 * the nearest sample point of the value set, the smallest
 * \f$ |P_0/P + L_0| \f$ met, which the guard near the singular locus needs
 * (see SingularLocus), and the index of that sample. The quotients
 * \f$ P_0 / P \f$ do not depend on the loop value, so nominalOverValueSet()
 * computes them once per frequency: the sweep asks for them at tens of
 * thousands of grid points. A WorstCaseMask says which magnitudes a caller
 * wants: the sheet sweep asks only for those the specifications in use at
 * that frequency need, two of the five on example 2, and stability, sensor
 * noise and tracking share one. worstCaseAt() evaluates the magnitudes the
 * mask asks for, leaving the others at their initial values, or all five
 * when given no mask.
 */

#ifndef QFTBX_CLOSED_LOOP_WORST_CASE_H
#define QFTBX_CLOSED_LOOP_WORST_CASE_H

#include <algorithm>
#include <complex>
#include <limits>
#include <vector>

#include "src/core/templates/cloud_set.h"

namespace qftbx {

struct WorstCase
{
    double stabilityNoise = -std::numeric_limits<double>::infinity();
    double trackingMin = std::numeric_limits<double>::infinity();
    double outputDisturbance = -std::numeric_limits<double>::infinity();
    double inputDisturbance = -std::numeric_limits<double>::infinity();
    double controlEffort = -std::numeric_limits<double>::infinity();
    double nearestSample = std::numeric_limits<double>::infinity();
    std::size_t nearestIndex = 0;
};

inline std::vector<std::complex<double>> nominalOverValueSet(std::complex<double> p0,
                                                             const ComplexCloud & valueSet)
{
    std::vector<std::complex<double>> quotients;
    quotients.reserve(valueSet.size());

    for (const std::complex<double> & p : valueSet) {
        quotients.push_back(p0 / p);
    }

    return quotients;
}

struct WorstCaseMask
{
    bool stabilityNoiseTracking = true;
    bool outputDisturbance = true;
    bool inputDisturbance = true;
    bool controlEffort = true;

    static WorstCaseMask all() { return WorstCaseMask{}; }
};

inline WorstCase worstCaseAt(std::complex<double> p0, std::complex<double> L,
                             const ComplexCloud & valueSet,
                             const std::vector<std::complex<double>> & nominalOverP,
                             const WorstCaseMask & mask)
{
    WorstCase worst;

    for (std::size_t i = 0; i < valueSet.size(); ++i) {
        const std::complex<double> & p = valueSet[i];
        const std::complex<double> & p0OverP = nominalOverP[i];
        const std::complex<double> denominator = p0OverP + L;

        const double distance = std::abs(denominator);
        if (distance < worst.nearestSample) {
            worst.nearestSample = distance;
            worst.nearestIndex = i;
        }

        if (mask.stabilityNoiseTracking) {
            const double stabilityNoise = std::abs(L / denominator);
            worst.stabilityNoise = std::max(worst.stabilityNoise, stabilityNoise);
            worst.trackingMin = std::min(worst.trackingMin, stabilityNoise);
        }
        if (mask.outputDisturbance) {
            worst.outputDisturbance = std::max(worst.outputDisturbance, std::abs(p0OverP / denominator));
        }
        if (mask.inputDisturbance) {
            worst.inputDisturbance = std::max(worst.inputDisturbance, std::abs(p0 / denominator));
        }
        if (mask.controlEffort) {
            worst.controlEffort = std::max(worst.controlEffort, std::abs((L / p) / denominator));
        }
    }

    return worst;
}

inline WorstCase worstCaseAt(std::complex<double> p0, std::complex<double> L,
                             const ComplexCloud & valueSet,
                             const std::vector<std::complex<double>> & nominalOverP)
{
    return worstCaseAt(p0, L, valueSet, nominalOverP, WorstCaseMask::all());
}

}

#endif
