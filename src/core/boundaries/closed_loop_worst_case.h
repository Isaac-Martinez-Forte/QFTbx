/**
 * @file
 * @brief The closed-loop magnitudes a QFT specification bounds, evaluated
 * over a value set at one loop value.
 *
 * Every specification bounds the magnitude of one closed-loop transfer
 * function in the worst case over the plant family, and tracking bounds the
 * spread between its worst and best case. With the nominal loop
 * \f$ L_0 = G P_0 \f$ and a plant \f$ P \f$ of the family, \f$ L = L_0 P / P_0 \f$
 * and the magnitudes read
 *
 * - stability and sensor noise: \f$ |L / (1 + L)| = |L_0 / (P_0/P + L_0)| \f$,
 * - output disturbance: \f$ |1 / (1 + L)| = |(P_0/P) / (P_0/P + L_0)| \f$,
 * - input disturbance: \f$ |P / (1 + L)| = |P_0 / (P_0/P + L_0)| \f$,
 * - control effort: \f$ |G / (1 + L)| = |(L_0 / P) / (P_0/P + L_0)| \f$.
 *
 * The boundary sweep evaluates them at every grid point of the Nichols
 * chart (Moreno, Banos and Berenguel 2006, equation (10)) and the
 * specification checker at the loop value of a returned controller: one
 * definition for both. WorstCase holds, in linear units, the worst case of
 * each magnitude and the best case of tracking, the smallest
 * \f$ |P_0/P + L_0| \f$ met and the index of that sample (see SingularLocus).
 * nominalOverValueSet() computes the quotients \f$ P_0 / P \f$ once per
 * frequency, and a WorstCaseMask says which magnitudes a caller wants.
 *
 * Each magnitude is an extreme of a modulus over the plants, ordered as its
 * square, so worstCaseAt finds the extremes with squares alone and
 * evaluates the expressions above only at the plants within 1e-10,
 * relatively, of each; the result is that of evaluating every plant, to the
 * bit. Where a square could leave [1e-290, 1e290] or is not finite, every
 * plant is evaluated (detail::worstCaseEverywhere).
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

namespace detail {

constexpr double kCandidateMargin = 1e-10;

inline bool squareInSafeRange(double value)
{
    return value >= 1e-290 && value <= 1e290;
}

inline WorstCase worstCaseEverywhere(std::complex<double> p0, std::complex<double> L,
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

}

inline WorstCase worstCaseAt(std::complex<double> p0, std::complex<double> L,
                             const ComplexCloud & valueSet,
                             const std::vector<std::complex<double>> & nominalOverP,
                             const WorstCaseMask & mask)
{
    const std::size_t count = valueSet.size();
    const double loopNorm = L.real() * L.real() + L.imag() * L.imag();
    const double plantNorm = p0.real() * p0.real() + p0.imag() * p0.imag();
    if (count == 0 || !detail::squareInSafeRange(loopNorm)
            || (mask.inputDisturbance && !detail::squareInSafeRange(plantNorm))) {
        return detail::worstCaseEverywhere(p0, L, valueSet, nominalOverP, mask);
    }

    double nearest = std::numeric_limits<double>::infinity();
    double farthest = 0.0;
    double outputLargest = 0.0;
    double effortSmallest = std::numeric_limits<double>::infinity();
    bool safe = true;
    for (std::size_t i = 0; i < count; ++i) {
        const std::complex<double> q = nominalOverP[i];
        const double dr = q.real() + L.real();
        const double di = q.imag() + L.imag();
        const double squared = dr * dr + di * di;
        safe = safe && detail::squareInSafeRange(squared);
        nearest = std::min(nearest, squared);
        farthest = std::max(farthest, squared);
        if (mask.outputDisturbance) {
            const double qNorm = q.real() * q.real() + q.imag() * q.imag();
            const double ratio = qNorm / squared;
            safe = safe && detail::squareInSafeRange(qNorm) && detail::squareInSafeRange(ratio);
            outputLargest = std::max(outputLargest, ratio);
        }
        if (mask.controlEffort) {
            const std::complex<double> p = valueSet[i];
            const double pNorm = p.real() * p.real() + p.imag() * p.imag();
            const double product = pNorm * squared;
            safe = safe && detail::squareInSafeRange(pNorm) && detail::squareInSafeRange(product);
            effortSmallest = std::min(effortSmallest, product);
        }
    }
    if (!safe) {
        return detail::worstCaseEverywhere(p0, L, valueSet, nominalOverP, mask);
    }

    const double nearLimit = nearest * (1.0 + detail::kCandidateMargin);
    const double farLimit = farthest * (1.0 - detail::kCandidateMargin);
    const double outputLimit = outputLargest * (1.0 - detail::kCandidateMargin);
    const double effortLimit = effortSmallest * (1.0 + detail::kCandidateMargin);

    WorstCase worst;
    for (std::size_t i = 0; i < count; ++i) {
        const std::complex<double> & p0OverP = nominalOverP[i];
        const double dr = p0OverP.real() + L.real();
        const double di = p0OverP.imag() + L.imag();
        const double squared = dr * dr + di * di;
        const bool near = squared <= nearLimit;
        const bool far = mask.stabilityNoiseTracking && squared >= farLimit;
        bool output = false;
        bool effort = false;
        if (mask.outputDisturbance) {
            const double qNorm = p0OverP.real() * p0OverP.real() + p0OverP.imag() * p0OverP.imag();
            output = qNorm / squared >= outputLimit;
        }
        if (mask.controlEffort) {
            const std::complex<double> & p = valueSet[i];
            const double pNorm = p.real() * p.real() + p.imag() * p.imag();
            effort = pNorm * squared <= effortLimit;
        }
        if (!(near || far || output || effort)) {
            continue;
        }

        const std::complex<double> & p = valueSet[i];
        const std::complex<double> denominator = p0OverP + L;
        if (near) {
            const double distance = std::abs(denominator);
            if (distance < worst.nearestSample) {
                worst.nearestSample = distance;
                worst.nearestIndex = i;
            }
            if (mask.stabilityNoiseTracking) {
                worst.stabilityNoise = std::max(worst.stabilityNoise, std::abs(L / denominator));
            }
            if (mask.inputDisturbance) {
                worst.inputDisturbance = std::max(worst.inputDisturbance, std::abs(p0 / denominator));
            }
        }
        if (far) {
            worst.trackingMin = std::min(worst.trackingMin, std::abs(L / denominator));
        }
        if (output) {
            worst.outputDisturbance = std::max(worst.outputDisturbance, std::abs(p0OverP / denominator));
        }
        if (effort) {
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
