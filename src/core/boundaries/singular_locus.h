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
 * \f$ |N| / |R + L_0| \f$ with \f$ R = P_0 / P \f$ over the value set, so it
 * is infinite where \f$ L_0 = -R \f$: the set \f$ \{-P_0/P\} \f$ is the
 * singular locus of the frequency (Gutman, Nordin and Cohen 2007,
 * section 6). The sweep takes the worst case over a finite sample, which
 * near the locus understates the supremum over the family; guard()
 * corrects it, never less conservatively than the sample.
 *
 * For a contour the border between consecutive points is the chord that
 * joins them, as for the epsilon-hull, and the extremes over that polygon
 * are exact: the largest \f$ |N| / |R + L_0| \f$ is |N| over the distance
 * from \f$ -L_0 \f$ to the polygon, the smallest is |N| over the distance
 * to its farthest vertex, and both are infinite when \f$ -L_0 \f$ is inside
 * it (Moreno, Banos and Berenguel 2006, algorithm 2.1, step 2). A cloud
 * has no border: its sampled extremes are widened by \f$ d / (d - h/2) \f$,
 * with d the distance to the nearest sample and h that sample's
 * nearest-neighbour spacing, and made infinite within half a spacing of a
 * sample, which is also how a pole inside the cloud is caught. Nothing
 * here has a free parameter.
 *
 * borderDistance, rayInside (the gains at which a point along a direction
 * lies inside a loop of the polygon, by the parity of crossings), loops()
 * and spacings() expose the geometry, for the boundary sweep and the tests.
 */
namespace qftbx {

class SingularLocus
{
public:
    SingularLocus(const std::vector<std::complex<double>> & nominalOverP, bool isContour);

    WorstCase guard(const WorstCase & sampled, std::complex<double> L0, std::complex<double> p0) const;

    double borderDistance(std::complex<double> minusL0) const;

    std::vector<Range> rayInside(std::complex<double> direction) const;

    bool isContour() const { return m_isContour; }
    std::size_t loopCount() const { return m_loops.size(); }
    const std::vector<double> & spacings() const { return m_spacing; }
    double largestSpacing() const;

    struct Segment {
        std::complex<double> a;
        std::complex<double> b;
        double length;
        double largestR;
    };

    const std::vector<std::vector<Segment>> & loops() const { return m_loops; }

private:
    std::vector<std::vector<Segment>> m_loops;
    std::vector<double> m_spacing;
    bool m_isContour = false;

    static int windingNumber(const std::vector<Segment> & loop, std::complex<double> z);
    static double distanceToSegment(std::complex<double> z, const Segment & s);
};

}

#endif
