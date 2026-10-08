#ifndef QFTBX_QUICK_SOLUTION_H
#define QFTBX_QUICK_SOLUTION_H

#include <cmath>
#include "src/core/math/constants.h"
#include <complex>

#include <vector>

/**
 * @file
 * @brief The cutting equations of the interval loop-shaping algorithms: the
 * magnitude cuts of NK's Quick Solution (Nataraj and Kubal, Int. J. Robust
 * Nonlinear Control 17:251-264, 2007, sec. 3.3) and the phase cuts of MC
 * (Martinez-Forte and Cervera, Int. J. Robust Nonlinear Control 31, 2021,
 * DOI 10.1002/rnc.5499, sec. 3.1, QS2 stage 2).
 *
 * The loop magnitude and every term of the loop phase are monotonic in
 * each controller parameter, so when a strip of the Nichols rectangle is
 * certainly forbidden, fixing the other parameters at the corner closest
 * to the allowed side and solving for one parameter gives the point where
 * its range stops being certainly infeasible. For a strip below
 * \f$ |B|_{min} \f$, with the gain and the zeros at their supremum and the
 * poles at their infimum, in linear magnitudes:
 *
 *   \f$ k' = |B|_{min} / (|N(j\omega,\bar z)| / |D(j\omega,\underline p)| \, |p_0|) \f$ (k to [k', sup k])
 *   \f$ z'_i = \sqrt{ (|B|_{min} |D| / (\bar k |N_{-i}| |p_0|))^2 - \omega^2 } \f$ (z_i to [z', sup z])
 *   \f$ p'_j = \sqrt{ (\bar k |N| / (|B|_{min} |D_{-j}|) |p_0|)^2 - \omega^2 } \f$ (p_j to [inf p, p'])
 *
 * The pole cut lowers the upper end, as in the paper's worked example. For
 * the phase, \f$ \angle L_0 = \varphi_0 + \sum_i \arctan(\omega/z_i) - \sum_l \arctan(\omega/p_l) \f$,
 * in radians on the box's own branch, and the gain adds none. With the
 * phases above \f$ \theta_{max} \f$ forbidden the closest corner is the
 * phase minimum, zeros at their supremum and poles at their infimum, and a
 * zero below \f$ \omega/\tan(m) \f$, or a pole above it, m the margin left
 * to that term, stays beyond the threshold; with the phases below
 * \f$ \theta_{min} \f$ forbidden it is the mirror.
 *
 * Every function returns the cut point, or a negative value when the
 * equation has no real solution or the margin is outside (0, pi/2), where
 * the conservative answer is not to cut.
 */
namespace qftbx {
namespace quick_solution {

inline double factorProductMagnitude(const std::vector<double> & values, double w,
                                    int skipIndex = -1)
{
    double product = 1.0;

    for (std::size_t i = 0; i < values.size(); ++i) {
        if (static_cast<int>(i) != skipIndex) {
            product *= std::abs(std::complex<double>(values.at(i), w));
        }
    }

    return product;
}

inline double gainCut(double boundMinLinear, const std::vector<double> & zeroSups,
                     const std::vector<double> & poleInfs, double w,
                     std::complex<double> p0)
{
    const double rest = factorProductMagnitude(zeroSups, w) /
                       factorProductMagnitude(poleInfs, w) * std::abs(p0);

    if (rest <= 0.0) {
        return -1.0;
    }

    return boundMinLinear / rest;
}

inline double zeroCut(double boundMinLinear, double gainSup,
                     const std::vector<double> & zeroSups,
                     const std::vector<double> & poleInfs, int index, double w,
                     std::complex<double> p0)
{
    const double denominator = gainSup *
            factorProductMagnitude(zeroSups, w, index) * std::abs(p0);

    if (denominator <= 0.0) {
        return -1.0;
    }

    const double factor = boundMinLinear *
            factorProductMagnitude(poleInfs, w) / denominator;
    const double radicand = factor * factor - w * w;

    if (radicand < 0.0) {
        return -1.0;
    }

    return std::sqrt(radicand);
}

inline double poleCut(double boundMinLinear, double gainSup,
                     const std::vector<double> & zeroSups,
                     const std::vector<double> & poleInfs, int index, double w,
                     std::complex<double> p0)
{
    const double denominator = boundMinLinear *
            factorProductMagnitude(poleInfs, w, index);

    if (denominator <= 0.0) {
        return -1.0;
    }

    const double factor = gainSup * factorProductMagnitude(zeroSups, w) *
            std::abs(p0) / denominator;
    const double radicand = factor * factor - w * w;

    if (radicand < 0.0) {
        return -1.0;
    }

    return std::sqrt(radicand);
}

inline double termPhaseSum(const std::vector<double> & values, double w,
                          int skipIndex = -1)
{
    double sum = 0.0;

    for (std::size_t i = 0; i < values.size(); ++i) {
        if (static_cast<int>(i) != skipIndex) {
            sum += std::atan2(w, values.at(i));
        }
    }

    return sum;
}

inline double zeroPhaseCutHigh(double thetaMax, double phi0,
                              const std::vector<double> & zeroSups,
                              const std::vector<double> & poleInfs, int index,
                              double w)
{
    const double margin = thetaMax - phi0 - termPhaseSum(zeroSups, w, index) +
                         termPhaseSum(poleInfs, w);

    if (margin <= 0.0 || margin >= (qftbx::math::kPi / 2.0)) {
        return -1.0;
    }

    return w / std::tan(margin);
}

inline double polePhaseCutHigh(double thetaMax, double phi0,
                              const std::vector<double> & zeroSups,
                              const std::vector<double> & poleInfs, int index,
                              double w)
{
    const double margin = phi0 + termPhaseSum(zeroSups, w) -
                         termPhaseSum(poleInfs, w, index) - thetaMax;

    if (margin <= 0.0 || margin >= (qftbx::math::kPi / 2.0)) {
        return -1.0;
    }

    return w / std::tan(margin);
}

inline double zeroPhaseCutLow(double thetaMin, double phi0,
                             const std::vector<double> & zeroInfs,
                             const std::vector<double> & poleSups, int index,
                             double w)
{
    const double margin = thetaMin - phi0 - termPhaseSum(zeroInfs, w, index) +
                         termPhaseSum(poleSups, w);

    if (margin <= 0.0 || margin >= (qftbx::math::kPi / 2.0)) {
        return -1.0;
    }

    return w / std::tan(margin);
}

inline double polePhaseCutLow(double thetaMin, double phi0,
                             const std::vector<double> & zeroInfs,
                             const std::vector<double> & poleSups, int index,
                             double w)
{
    const double margin = phi0 + termPhaseSum(zeroInfs, w) -
                         termPhaseSum(poleSups, w, index) - thetaMin;

    if (margin <= 0.0 || margin >= (qftbx::math::kPi / 2.0)) {
        return -1.0;
    }

    return w / std::tan(margin);
}

}
}

#endif
