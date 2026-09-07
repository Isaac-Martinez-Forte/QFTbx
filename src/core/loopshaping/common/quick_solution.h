#ifndef QFTBX_QUICK_SOLUTION_H
#define QFTBX_QUICK_SOLUTION_H

#include <cmath>
#include "src/core/math/constants.h"
#include <complex>

#include <vector>

/**
 * @brief Quick Solution cutting equations of algorithm NK (Paluri/Nataraj
 * and Kubal, "Automatic loop shaping in QFT using hybrid optimisation and
 * constraint propagation techniques", Int. J. Robust Nonlinear Control
 * 17:251-264, 2007, sec. 3.3).
 *
 * For a box straddling a bound whose forbidden side is BELOW, the loop
 * magnitude is monotonic in every controller parameter, so fixing all
 * other parameters at the corner that maximises \f$ |L_0| \f$ (gain and
 * zeros at their supremum, poles at their infimum) and solving
 * \f$ |L_0| = |B_i|_{min} \f$ for one parameter yields the point where its
 * range stops being certainly infeasible:
 *
 *   \f$ k' = |B|_{min} / (|N(j\omega,\bar z)| / |D(j\omega,\underline p)|
 *            \, |p_0|) \f$                       (cut k to [k', sup k])
 *   \f$ z'_i = \sqrt{ (|B|_{min} |D| / (\bar k |N_{-i}| |p_0|))^2
 *            - \omega^2 } \f$                    (cut z_i to [z', sup z])
 *   \f$ p'_j = \sqrt{ (\bar k |N| / (|B|_{min} |D_{-j}|) |p_0|)^2
 *            - \omega^2 } \f$                    (cut p_j to [inf p, p'])
 *
 * All quantities are LINEAR magnitudes (the historical implementation
 * mixed decibels into the quotients and subtracted the logarithm of
 * omega^2, producing dimensionless noise). The pole reduces its UPPER end:
 * a larger pole lowers the loop towards the forbidden side (the QFTbx
 * thesis text, sec. 3.2, states the opposite interval - an erratum; the
 * paper's worked example reduces p1 = [1025.5, 4834.5] to
 * [1025.5, 4692.15]).
 *
 * The functions return the cut point, or a negative value when the
 * equation has no real solution (no cut possible at this frequency).
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

/// k' such that |L0(k', sup z, inf p)| equals the linear bound minimum.
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

/// z' for the zero at 'index', the other parameters at their |L0|-maximal
/// corner. Negative when the equation has no real solution.
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

/// p' for the pole at 'index', the other parameters at their |L0|-maximal
/// corner. Negative when the equation has no real solution.
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

/**
 * @brief Phase cutting equations of algorithm MC (Martinez-Forte and
 * Cervera, "Accelerated quantitative feedback theory interval automatic
 * loop shaping algorithm", Int. J. Robust Nonlinear Control 31, 2021,
 * DOI 10.1002/rnc.5499, sec. 3.1 and algorithm QS2 stage 2).
 *
 * The loop phase decomposes term by term,
 *
 *   \f$ \angle L_0 = \varphi_0 + \sum_i \arctan(\omega/z_i)
 *                  - \sum_l \arctan(\omega/p_l) \f$,
 *
 * with \f$ \varphi_0 = \angle P_0(j\omega) \f$ on the \f$ (-2\pi, 0] \f$
 * branch (the gain adds no phase, which is why QS2 stage 2 skips it). Every
 * term is monotonic in its parameter, so when a vertical strip of the
 * Nichols rectangle is certainly forbidden, fixing the other parameters at
 * the corner that puts the loop CLOSEST to the allowed side and solving for
 * the remaining term yields the point where its range stops being certainly
 * infeasible:
 *
 * - Strip of HIGH phase forbidden (right side, threshold
 *   \f$ \theta_{max} \f$): the closest-to-allowed corner is the phase
 *   minimum (zeros at their supremum, poles at their infimum). A zero
 *   below \f$ \omega/\tan(m) \f$, \f$ m = \theta_{max} - \varphi_0 -
 *   \sum_{i \ne j}\arctan(\omega/\bar z_i) + \sum_l\arctan(\omega/
 *   \underline p_l) \f$, keeps even that corner beyond the threshold
 *   (cut z to [z', sup z]); a pole above \f$ \omega/\tan(m') \f$ does the
 *   same (cut p to [inf p, p']).
 * - Strip of LOW phase forbidden (left side, threshold
 *   \f$ \theta_{min} \f$): symmetric with the phase maximum corner (zeros
 *   at their infimum, poles at their supremum); the zero cuts its upper
 *   end and the pole its lower end.
 *
 * The published pseudocode of QS2 stage 2 assigns zeros to sup and poles
 * to inf while its comment says the assignment "maximizes the contribution
 * to the phase"; those extremes MINIMIZE it (the phase of a zero term is
 * arctan(omega/z), decreasing). The assignments are the ones consistent
 * with the right-side cut the paper illustrates; the comment is the
 * erratum.
 *
 * All angles are RADIANS on the box's own branch. The functions return the
 * cut point, or a negative value when no sound cut exists at this
 * frequency (the margin must lie strictly inside (0, pi/2): outside it the
 * equation has no solution in the parameter's positive range, and the
 * conservative answer is not to cut).
 */

/// Sum of the phase contributions atan(w/x) of the terms (jw + x),
/// optionally skipping one term.
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

/// z' for the zero at 'index' when the phases ABOVE thetaMax are certainly
/// forbidden: below z' even the phase-minimal corner of the other
/// parameters stays beyond the threshold (cut z to [z', sup z]).
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

/// p' for the pole at 'index' when the phases ABOVE thetaMax are certainly
/// forbidden (cut p to [inf p, p']).
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

/// z' for the zero at 'index' when the phases BELOW thetaMin are certainly
/// forbidden: above z' even the phase-maximal corner of the other
/// parameters stays under the threshold (cut z to [inf z, z']).
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

/// p' for the pole at 'index' when the phases BELOW thetaMin are certainly
/// forbidden (cut p to [p', sup p]).
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

} // namespace quick_solution
} // namespace qftbx

#endif // QFTBX_QUICK_SOLUTION_H
