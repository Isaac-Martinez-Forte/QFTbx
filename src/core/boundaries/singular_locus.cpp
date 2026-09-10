#include "src/core/boundaries/singular_locus.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "src/core/math/constants.h"

namespace qftbx {

SingularLocus::SingularLocus(const std::vector<std::complex<double>> & nominalOverP, bool isContour)
    : m_isContour(isContour)
{
    const std::size_t n = nominalOverP.size();

    if (n == 0) {
        return;
    }

    if (!isContour) {
        //The local spacing of an unordered sample: each point's distance to
        //its nearest neighbour. Quadratic once per frequency; the sweep
        //that follows costs far more.
        m_spacing.assign(n, 0.0);
        for (std::size_t i = 0; i < n; ++i) {
            double nearest = std::numeric_limits<double>::infinity();
            for (std::size_t j = 0; j < n; ++j) {
                if (j != i) {
                    nearest = std::min(nearest, std::abs(nominalOverP[i] - nominalOverP[j]));
                }
            }
            m_spacing[i] = std::isfinite(nearest) ? nearest : 0.0;
        }
        return;
    }

    //The walk of one component closes when it returns to its first point,
    //and the contours of several components are concatenated: a loop runs
    //from a point to the next exact repeat of it. A run that never repeats
    //its start (the relaxed walk deduplicates its output) is closed on its
    //last point.
    std::size_t start = 0;
    while (start < n) {
        std::size_t end = n;   //one past the loop's last point
        for (std::size_t j = start + 1; j < n; ++j) {
            if (nominalOverP[j] == nominalOverP[start]) {
                end = j;   //the repeat closes it; the next loop starts after
                break;
            }
        }

        std::vector<Segment> loop;
        for (std::size_t j = start; j + 1 < end; ++j) {
            const std::complex<double> a = nominalOverP[j], b = nominalOverP[j + 1];
            if (a != b) {
                loop.push_back({a, b, std::abs(b - a), std::max(std::abs(a), std::abs(b))});
            }
        }
        //Close it: back to the first point, unless the walk already did.
        if (end - start >= 2) {
            const std::complex<double> a = nominalOverP[end - 1], b = nominalOverP[start];
            if (a != b) {
                loop.push_back({a, b, std::abs(b - a), std::max(std::abs(a), std::abs(b))});
            }
        }
        if (!loop.empty()) {
            m_loops.push_back(std::move(loop));
        }

        start = end < n ? end + 1 : n;
    }
}

std::vector<Range> SingularLocus::rayInside(std::complex<double> direction) const
{
    std::vector<Range> inside;
    if (!m_isContour) {
        return inside;
    }
    //Crossings of the ray {g * direction, g > 0} with each loop: a segment
    //a + t (b - a), t in [0, 1), meets the ray at g = cross(a, d) / cross(d, b - a)
    //... solved as a 2x2 system; the parity of the sorted crossings says
    //where the ray is inside.
    const double dx = direction.real(), dy = direction.imag();
    for (const std::vector<Segment> & loop : m_loops) {
        std::vector<double> crossings;
        for (const Segment & s : loop) {
            const double ex = s.b.real() - s.a.real(), ey = s.b.imag() - s.a.imag();
            const double det = dx * (-ey) - dy * (-ex);   // [d, -(b-a)] [g, t]^T = a
            if (det == 0.0) {
                continue;
            }
            const double ax = s.a.real(), ay = s.a.imag();
            const double g = (ax * (-ey) - ay * (-ex)) / det;
            const double t = (dx * ay - dy * ax) / det;
            if (g > 0.0 && t >= 0.0 && t < 1.0) {
                crossings.push_back(g);
            }
        }
        std::sort(crossings.begin(), crossings.end());
        for (std::size_t k = 0; k + 1 < crossings.size(); k += 2) {
            inside.push_back(Range(crossings[k], crossings[k + 1]));
        }
    }
    return inside;
}

int SingularLocus::windingNumber(const std::vector<Segment> & loop, std::complex<double> z)
{
    //The turning of the loop around z, in whole turns: the sum of the
    //signed angles subtended by its segments.
    double turning = 0.0;
    for (const Segment & s : loop) {
        const std::complex<double> u = s.a - z, v = s.b - z;
        turning += std::atan2(u.real() * v.imag() - u.imag() * v.real(),
                              u.real() * v.real() + u.imag() * v.imag());
    }
    return static_cast<int>(std::lround(turning / (2.0 * qftbx::math::kPi)));
}

double SingularLocus::distanceToSegment(std::complex<double> z, const Segment & s)
{
    const std::complex<double> ab = s.b - s.a;
    const double length2 = std::norm(ab);
    if (length2 <= 0.0) {
        return std::abs(z - s.a);
    }
    const double t = std::clamp(((z - s.a) * std::conj(ab)).real() / length2, 0.0, 1.0);
    return std::abs(z - (s.a + t * ab));
}

double SingularLocus::largestSpacing() const
{
    double largest = 0.0;
    for (const double h : m_spacing) {
        largest = std::max(largest, h);
    }
    return largest;
}

double SingularLocus::borderDistance(std::complex<double> minusL0) const
{
    double nearest = std::numeric_limits<double>::infinity();
    for (const std::vector<Segment> & loop : m_loops) {
        if (windingNumber(loop, minusL0) != 0) {
            return 0.0;
        }
        for (const Segment & s : loop) {
            nearest = std::min(nearest, distanceToSegment(minusL0, s));
        }
    }
    return nearest;
}

WorstCase SingularLocus::guard(const WorstCase & sampled, std::complex<double> L0, std::complex<double> p0) const
{
    constexpr double kInfinity = std::numeric_limits<double>::infinity();
    WorstCase guarded = sampled;

    const auto singular = [&guarded, &kInfinity]() {
        guarded.stabilityNoise = kInfinity;
        guarded.trackingMin = 0.0;
        guarded.outputDisturbance = kInfinity;
        guarded.inputDisturbance = kInfinity;
        guarded.controlEffort = kInfinity;
        return guarded;
    };

    if (!m_isContour) {
        if (sampled.nearestIndex >= m_spacing.size()) {
            return guarded;   //no sample: nothing to say
        }
        const double d = sampled.nearestSample;
        const double halfSpacing = 0.5 * m_spacing[sampled.nearestIndex];
        if (!(d > halfSpacing)) {
            return singular();
        }
        //A point of the family may be up to half a spacing nearer the pole
        //than the nearest sample: every magnitude with the pole in its
        //denominator may be larger by d / (d - h/2), the tracking minimum
        //smaller by the same ratio.
        const double widen = d / (d - halfSpacing);
        guarded.stabilityNoise = sampled.stabilityNoise * widen;
        guarded.trackingMin = sampled.trackingMin / widen;
        guarded.outputDisturbance = sampled.outputDisturbance * widen;
        guarded.inputDisturbance = sampled.inputDisturbance * widen;
        guarded.controlEffort = sampled.controlEffort * widen;
        return guarded;
    }

    if (m_loops.empty()) {
        return guarded;
    }

    //The extremes over the polygonal border, exactly: the pole's distance to
    //the nearest chord and to the farthest vertex, and for the magnitudes
    //whose numerator is |R| the largest |R| of a chord over that chord's
    //distance.
    const std::complex<double> z = -L0;
    double nearest = kInfinity;
    double farthest = 0.0;
    double largestROverDistance = 0.0;

    for (const std::vector<Segment> & loop : m_loops) {
        if (windingNumber(loop, z) != 0) {
            return singular();
        }
        for (const Segment & s : loop) {
            const double d = distanceToSegment(z, s);
            if (!(d > 0.0)) {
                return singular();
            }
            nearest = std::min(nearest, d);
            farthest = std::max(farthest, std::max(std::abs(z - s.a), std::abs(z - s.b)));
            largestROverDistance = std::max(largestROverDistance, s.largestR / d);
        }
    }

    const double lMagnitude = std::abs(L0);
    const double p0Magnitude = std::abs(p0);

    //Never less conservative than the sample, which lies on the border.
    guarded.stabilityNoise = std::max(sampled.stabilityNoise, lMagnitude / nearest);
    guarded.trackingMin = std::min(sampled.trackingMin, lMagnitude / farthest);
    guarded.outputDisturbance = std::max(sampled.outputDisturbance, largestROverDistance);
    guarded.inputDisturbance = std::max(sampled.inputDisturbance, p0Magnitude / nearest);
    guarded.controlEffort = std::max(sampled.controlEffort, lMagnitude / p0Magnitude * largestROverDistance);
    return guarded;
}

} // namespace qftbx
