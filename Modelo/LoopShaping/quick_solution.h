#ifndef QFTBX_QUICK_SOLUTION_H
#define QFTBX_QUICK_SOLUTION_H

#include <cmath>
#include <complex>

#include <QVector>

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

inline qreal factorProductMagnitude(const QVector<qreal> & values, qreal w,
                                    int skipIndex = -1)
{
    qreal product = 1.0;

    for (int i = 0; i < values.size(); ++i) {
        if (i != skipIndex) {
            product *= std::abs(std::complex<qreal>(values.at(i), w));
        }
    }

    return product;
}

/// k' such that |L0(k', sup z, inf p)| equals the linear bound minimum.
inline qreal gainCut(qreal boundMinLinear, const QVector<qreal> & zeroSups,
                     const QVector<qreal> & poleInfs, qreal w,
                     std::complex<qreal> p0)
{
    const qreal rest = factorProductMagnitude(zeroSups, w) /
                       factorProductMagnitude(poleInfs, w) * std::abs(p0);

    if (rest <= 0.0) {
        return -1.0;
    }

    return boundMinLinear / rest;
}

/// z' for the zero at 'index', the other parameters at their |L0|-maximal
/// corner. Negative when the equation has no real solution.
inline qreal zeroCut(qreal boundMinLinear, qreal gainSup,
                     const QVector<qreal> & zeroSups,
                     const QVector<qreal> & poleInfs, int index, qreal w,
                     std::complex<qreal> p0)
{
    const qreal denominator = gainSup *
            factorProductMagnitude(zeroSups, w, index) * std::abs(p0);

    if (denominator <= 0.0) {
        return -1.0;
    }

    const qreal factor = boundMinLinear *
            factorProductMagnitude(poleInfs, w) / denominator;
    const qreal radicand = factor * factor - w * w;

    if (radicand < 0.0) {
        return -1.0;
    }

    return std::sqrt(radicand);
}

/// p' for the pole at 'index', the other parameters at their |L0|-maximal
/// corner. Negative when the equation has no real solution.
inline qreal poleCut(qreal boundMinLinear, qreal gainSup,
                     const QVector<qreal> & zeroSups,
                     const QVector<qreal> & poleInfs, int index, qreal w,
                     std::complex<qreal> p0)
{
    const qreal denominator = boundMinLinear *
            factorProductMagnitude(poleInfs, w, index);

    if (denominator <= 0.0) {
        return -1.0;
    }

    const qreal factor = gainSup * factorProductMagnitude(zeroSups, w) *
            std::abs(p0) / denominator;
    const qreal radicand = factor * factor - w * w;

    if (radicand < 0.0) {
        return -1.0;
    }

    return std::sqrt(radicand);
}

} // namespace quick_solution
} // namespace qftbx

#endif // QFTBX_QUICK_SOLUTION_H
