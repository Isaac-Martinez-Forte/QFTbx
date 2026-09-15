#ifndef QFTBX_SINGULAR_LOCUS_H
#define QFTBX_SINGULAR_LOCUS_H

#include <complex>
#include <cstddef>
#include <vector>

#include "src/core/boundaries/closed_loop_worst_case.h"
#include "src/core/math/range.h"

/**
 * @file
 * @brief The set of loop values at which the closed loop of some plant of
 * the family is singular, and the guard the boundary sweep needs near it.
 *
 * Every closed-loop magnitude the sheets are built from has the form
 * \f$ |N| / |R + L_0| \f$ with \f$ R = P_0 / P \f$ ranging over the value
 * set: it is infinite where \f$ L_0 = -R \f$, and the set \f$ \{-P_0/P\} \f$
 * is the singular locus of the frequency (Gutman, Nordin and Cohen 2007,
 * section 6, the template of plant inverses, "always on the infeasible
 * side"). The sweep takes the worst case over a FINITE sample of the value
 * set, and near the locus that sample understates the supremum over the
 * continuous family in two ways:
 *
 * - when \f$ -L_0 \f$ lies INSIDE the template, some plant of the family
 *   makes the loop singular and the true worst case is infinite, while the
 *   sample - and above all the contour, which has no interior points - stays
 *   finite. Moreno, Banos and Berenguel 2006 make this test step 2 of their
 *   algorithm 2.1 and warn that computing from the border alone is wrong
 *   without it;
 * - when \f$ -L_0 \f$ lies OUTSIDE, the extremes of the magnitudes over
 *   the family are attained on its border (maximum modulus), and the sample
 *   only holds points OF that border, not the border between them. For the
 *   contour the border between two consecutive points is taken as the
 *   chord joining them - the same assumption the epsilon-hull rests on - and
 *   then the extremes over the polygonal border are EXACT: the largest
 *   \f$ |N| / |R + L_0| \f$ is \f$ |N| \f$ over the distance from
 *   \f$ -L_0 \f$ to the polygon, and the smallest is \f$ |N| \f$ over the
 *   distance to its farthest vertex. No bound is needed, and the values
 *   differ from the sampled ones only where a chord passes closer to the
 *   pole than its endpoints do, which is of the second order in the spacing
 *   over the distance. (A first-order Lipschitz bound was tried first and
 *   touched two fifths of the Nichols grid; the exact polygon touches the
 *   cells next to the locus, as it should.)
 *
 * For the cloud there is no border to walk: the sample fills the template
 * and a point of the family may lie up to half the local spacing closer to
 * the pole than the nearest sample does. The sampled extremes are widened
 * by that ratio, \f$ d / (d - h/2) \f$ with \f$ d \f$ the distance to the
 * nearest sample and \f$ h \f$ its nearest-neighbour distance, and made
 * infinite when the pole is within half a spacing of a sample - which is
 * also how a pole INSIDE the cloud is caught, since there is no inside test
 * without an order. This is a first-order bound and it is the right one
 * here: the cloud carries less structure than the contour, so it guards
 * more widely. A single spacing for the whole cloud would not do: the
 * spacing of a swept template varies by two to three orders of magnitude
 * between its dense and its sparse regions (measured on example 2).
 *
 * Nothing here has a free parameter: the spacing is that of the sample and
 * the distances are geometry.
 */
namespace qftbx {

class SingularLocus
{
public:
    /**
     * @param nominalOverP the quotients \f$ P_0 / P \f$ of the value set,
     *        in the order of the value set.
     * @param isContour whether that value set is the epsilon-hull contour
     *        (an ordered walk, one or more closed loops concatenated) or an
     *        unordered cloud.
     */
    SingularLocus(const std::vector<std::complex<double>> & nominalOverP, bool isContour);

    /**
     * @brief The worst case over the FAMILY at the loop value L0, given the
     * worst case over the sample the sweep computed there.
     *
     * Contour: the exact extremes over the polygonal border, infinite when
     * \f$ -L_0 \f$ is inside it. Cloud: the sampled extremes widened by the
     * local spacing, infinite within half a spacing of a sample. In both
     * the result is never less conservative than the sample: the maxima
     * can only rise and the tracking minimum can only fall.
     *
     * @param sampled the worst case over the sample at L0, with its nearest
     *        sample distance and index.
     * @param L0 the loop value; \f$ -L_0 \f$ is the pole in the plane of R.
     * @param p0 the nominal plant value at this frequency.
     */
    WorstCase guard(const WorstCase & sampled, std::complex<double> L0, std::complex<double> p0) const;

    /// Contour: the distance from a point to the polygonal border, zero
    /// when the point is inside it. Exposed for the tests.
    double borderDistance(std::complex<double> minusL0) const;

    /// For a contour: the intervals of g > 0 at which the point g * direction
    /// (a unit vector) lies inside one of the template's polygon loops, by
    /// the parity of the ray's crossings of each loop. Empty for a cloud.
    std::vector<Range> rayInside(std::complex<double> direction) const;

    bool isContour() const { return m_isContour; }
    std::size_t loopCount() const { return m_loops.size(); }
    /// Cloud mode: the nearest-neighbour distance of each sample, and the
    /// largest of them.
    const std::vector<double> & spacings() const { return m_spacing; }
    double largestSpacing() const;

    struct Segment {
        std::complex<double> a;
        std::complex<double> b;
        double length;
        /// The larger |R| of its two endpoints: the bound on |R| along it.
        double largestR;
    };

    /// The polygon of a contour, loop by loop, in the plane of P0/P: the
    /// closed loops of the walk, each a run of segments (the last one
    /// closing back to the loop's first point). Empty for a cloud.
    const std::vector<std::vector<Segment>> & loops() const { return m_loops; }

private:
    std::vector<std::vector<Segment>> m_loops;
    //Cloud mode: per sample.
    std::vector<double> m_spacing;
    bool m_isContour = false;

    static int windingNumber(const std::vector<Segment> & loop, std::complex<double> z);
    static double distanceToSegment(std::complex<double> z, const Segment & s);
};

} // namespace qftbx

#endif // QFTBX_SINGULAR_LOCUS_H
