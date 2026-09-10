#ifndef QFTBX_CLOSED_LOOP_WORST_CASE_H
#define QFTBX_CLOSED_LOOP_WORST_CASE_H

#include <algorithm>
#include <complex>
#include <limits>
#include <vector>

#include "src/core/templates/cloud_set.h"

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
 * a returned controller, to verify it against the specifications themselves.
 * The two used to carry separate copies of the formulas; this is the one
 * definition.
 */
namespace qftbx {

/// The worst case of each magnitude over the value set at one loop value,
/// and the best case of the tracking magnitude too, since tracking bounds
/// the spread. Linear units.
struct WorstCase
{
    double stabilityNoise = -std::numeric_limits<double>::infinity();
    double trackingMin = std::numeric_limits<double>::infinity();
    double outputDisturbance = -std::numeric_limits<double>::infinity();
    double inputDisturbance = -std::numeric_limits<double>::infinity();
    double controlEffort = -std::numeric_limits<double>::infinity();
    /// The distance from \f$ -L_0 \f$ to the nearest sample point of the
    /// value set: the smallest \f$ |R + L_0| \f$ met, which is what the
    /// guard near the singular locus needs (see SingularLocus).
    double nearestSample = std::numeric_limits<double>::infinity();
    /// The index in the value set of that nearest sample.
    std::size_t nearestIndex = 0;
};

/// The quotients \f$ P_0 / P \f$ of the value set, computed once per
/// frequency: they do not depend on the loop value, and the sweep used to
/// divide them again at every one of its tens of thousands of grid points.
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

/// Which of the magnitudes a caller wants. The sheet sweep asks for the
/// ones the specifications in use at that frequency need - on example 2,
/// two of the five - and used to compute all five at every grid point for
/// every plant. Stability, sensor noise and tracking share one magnitude.
struct WorstCaseMask
{
    bool stabilityNoiseTracking = true;
    bool outputDisturbance = true;
    bool inputDisturbance = true;
    bool controlEffort = true;

    static WorstCaseMask all() { return WorstCaseMask{}; }
};

/// The magnitudes the mask asks for, at the loop value L over the value set,
/// given the nominal plant value and the quotients nominalOverValueSet()
/// returns. A magnitude not asked for keeps its initial value.
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

        //A NaN candidate compares false and leaves the running value alone,
        //as the explicit comparisons this replaces did.
        if (mask.stabilityNoiseTracking) {
            //Stability and sensor noise share the same transfer magnitude,
            //and tracking bounds its spread.
            const double stabilityNoise = std::abs(L / denominator);
            worst.stabilityNoise = std::max(worst.stabilityNoise, stabilityNoise);
            worst.trackingMin = std::min(worst.trackingMin, stabilityNoise);
        }
        if (mask.outputDisturbance) {
            //Disturbance rejection at the plant output.
            worst.outputDisturbance = std::max(worst.outputDisturbance, std::abs(p0OverP / denominator));
        }
        if (mask.inputDisturbance) {
            //Disturbance rejection at the plant input.
            worst.inputDisturbance = std::max(worst.inputDisturbance, std::abs(p0 / denominator));
        }
        if (mask.controlEffort) {
            //Control effort.
            worst.controlEffort = std::max(worst.controlEffort, std::abs((L / p) / denominator));
        }
    }

    return worst;
}

/// All five magnitudes.
inline WorstCase worstCaseAt(std::complex<double> p0, std::complex<double> L,
                             const ComplexCloud & valueSet,
                             const std::vector<std::complex<double>> & nominalOverP)
{
    return worstCaseAt(p0, L, valueSet, nominalOverP, WorstCaseMask::all());
}

} // namespace qftbx

#endif // QFTBX_CLOSED_LOOP_WORST_CASE_H
