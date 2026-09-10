#include "src/core/boundaries/closed_form_columns.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "src/core/boundaries/singular_locus.h"
#include "src/core/math/constants.h"

namespace qftbx {

namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();

//The set of g > 0 where a g^2 + b g + c >= 0.
RangeUnion nonNegative(double a, double b, double c)
{
    const RangeUnion positive = RangeUnion::of(0.0, kInfinity);

    if (a == 0.0) {
        if (b == 0.0) {
            return c >= 0.0 ? positive : RangeUnion();
        }
        const double root = -c / b;
        return b > 0.0 ? RangeUnion::of(std::max(0.0, root), kInfinity)
                       : RangeUnion::of(0.0, root);
    }

    const double discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0) {
        return a > 0.0 ? positive : RangeUnion();
    }

    //Stable roots: the larger-magnitude one from the formula, the other
    //from the product.
    const double s = std::sqrt(discriminant);
    const double qv = -0.5 * (b + (b >= 0.0 ? s : -s));
    double r1, r2;
    if (qv != 0.0) {
        r1 = qv / a;
        r2 = c / qv;
    } else {
        r1 = r2 = 0.0;
    }
    if (r1 > r2) {
        std::swap(r1, r2);
    }

    if (a > 0.0) {
        //Outside the roots.
        RangeUnion out = RangeUnion::of(std::max(0.0, r2), kInfinity);
        if (r1 > 0.0) {
            const double lower[2] = {0.0, std::max(0.0, r2)};
            const double upper[2] = {r1, kInfinity};
            out = RangeUnion::of(lower, upper, 2);
        }
        return out;
    }
    //Between the roots.
    return RangeUnion::of(std::max(0.0, r1), r2);
}

//The gains at which the plants along one segment of the contour, read as
//the guard reads them, violate: the numerator of the magnitude bounded over
//the segment's ends (or g itself), the denominator |q + L| by the distance
//from -L = g m to the segment. For the point g m the signed distance to the
//segment's line and the projection onto it are both linear in g, so the
//forbidden set is one interval: where |alpha g - beta| < N(g) / W and the
//projection lies within the segment.
Range segmentForbidden(SpecificationType type, double W, std::complex<double> p0,
                       std::complex<double> qa, std::complex<double> qb,
                       std::complex<double> pa, std::complex<double> pb,
                       std::complex<double> m)
{
    const Range none(1.0, 0.0);   //empty (min > max)
    const std::complex<double> e = qb - qa;
    const double length = std::abs(e);
    if (!(length > 0.0)) {
        return none;
    }
    const std::complex<double> u = e / length;
    const std::complex<double> n(-u.imag(), u.real());
    const auto dot = [](std::complex<double> x, std::complex<double> y) { return x.real() * y.real() + x.imag() * y.imag(); };

    //Where the projection of g m falls within the segment: gamma g - delta in [0, length].
    const double gamma = dot(m, u), delta = dot(qa, u);
    double gLow = 0.0, gHigh = kInfinity;
    if (gamma > 0.0) {
        gLow = std::max(gLow, delta / gamma);
        gHigh = std::min(gHigh, (delta + length) / gamma);
    } else if (gamma < 0.0) {
        gLow = std::max(gLow, (delta + length) / gamma);
        gHigh = std::min(gHigh, delta / gamma);
    } else if (!(-delta >= 0.0 && -delta <= length)) {
        return none;   //the whole ray projects outside the segment
    }
    if (gLow > gHigh) {
        return none;
    }

    //The distance condition |alpha g - beta| < N / W, with N constant or
    //proportional to g.
    const double alpha = dot(m, n), beta = dot(qa, n);
    double slope = 0.0, constant = 0.0;   //N(g) = slope * g + constant
    switch (type) {
    case SpecificationType::Stability:
    case SpecificationType::SensorNoise:
        slope = 1.0; break;
    case SpecificationType::OutputDisturbance:
        constant = std::max(std::abs(qa), std::abs(qb)); break;
    case SpecificationType::InputDisturbance:
        constant = std::abs(p0); break;
    case SpecificationType::ControlEffort: {
        const double pMin = std::min(std::abs(pa), std::abs(pb));
        if (!(pMin > 0.0)) return Range(gLow, gHigh);
        slope = 1.0 / pMin; break;
    }
    default:
        return none;
    }
    //  -(slope g + constant)/W < alpha g - beta < (slope g + constant)/W
    //  (alpha - slope/W) g < beta + constant/W        [upper side]
    //  (alpha + slope/W) g > beta - constant/W        [lower side]
    double lo = gLow, hi = gHigh;
    const double a1 = alpha - slope / W, b1 = beta + constant / W;
    if (a1 > 0.0) hi = std::min(hi, b1 / a1);
    else if (a1 < 0.0) lo = std::max(lo, b1 / a1);
    else if (!(0.0 < b1)) return none;
    const double a2 = alpha + slope / W, b2 = beta - constant / W;
    if (a2 > 0.0) lo = std::max(lo, b2 / a2);
    else if (a2 < 0.0) hi = std::min(hi, b2 / a2);
    else if (!(0.0 > b2)) return none;

    return lo < hi ? Range(lo, hi) : none;
}

double toDb(double g)
{
    if (g <= 0.0) {
        return -kInfinity;
    }
    if (!(g < kInfinity)) {
        return kInfinity;
    }
    return 20.0 * std::log10(g);
}

} // namespace

bool ClosedFormColumns::covers(SpecificationType type)
{
    return type == SpecificationType::Stability || type == SpecificationType::SensorNoise ||
           type == SpecificationType::OutputDisturbance || type == SpecificationType::InputDisturbance ||
           type == SpecificationType::ControlEffort;
}

RangeUnion ClosedFormColumns::allowedGains(SpecificationType type, double W,
                                           std::complex<double> p0, std::complex<double> p,
                                           std::complex<double> q, double phaseDegrees)
{
    const std::complex<double> direction = std::polar(1.0, phaseDegrees * qftbx::math::kPi / 180.0);
    const double c = std::real(std::conj(q) * direction);
    const double q2 = std::norm(q);
    const double W2 = W * W;

    switch (type) {
    case SpecificationType::Stability:
    case SpecificationType::SensorNoise:
        return nonNegative(W2 - 1.0, 2.0 * W2 * c, W2 * q2);
    case SpecificationType::OutputDisturbance:
        return nonNegative(W2, 2.0 * W2 * c, (W2 - 1.0) * q2);
    case SpecificationType::InputDisturbance:
        return nonNegative(W2, 2.0 * W2 * c, W2 * q2 - std::norm(p0));
    case SpecificationType::ControlEffort: {
        const double p2 = std::norm(p);
        return nonNegative(W2 - (p2 > 0.0 ? 1.0 / p2 : kInfinity), 2.0 * W2 * c, W2 * q2);
    }
    default:
        return RangeUnion::of(0.0, kInfinity);
    }
}

BoundaryColumns ClosedFormColumns::columns(SpecificationType type, double boundDb,
                                           std::complex<double> p0, const ComplexCloud & valueSet,
                                           const std::vector<std::complex<double>> & nominalOverP,
                                           const std::vector<double> & phases, Range phaseRange,
                                           const SingularLocus * locus)
{
    const double W = std::pow(10.0, boundDb / 20.0);
    std::vector<std::vector<BoundaryColumns::Span>> columns(phases.size());

    for (std::size_t j = 0; j < phases.size(); ++j) {
        RangeUnion allowed = RangeUnion::of(0.0, kInfinity);
        for (std::size_t i = 0; i < valueSet.size() && !allowed.isEmpty(); ++i) {
            allowed.intersectWith(allowedGains(type, W, p0, valueSet[i], nominalOverP[i], phases[j]));
        }

        if (locus != nullptr && locus->isContour() && !allowed.isEmpty()) {
            const std::complex<double> minus = -std::polar(1.0, phases[j] * qftbx::math::kPi / 180.0);
            const auto forbid = [&allowed](const Range & r) {
                if (!(r.min < r.max)) return;
                //Keep what is below and what is above the forbidden interval.
                const double lower[2] = {0.0, r.max};
                const double upper[2] = {r.min, kInfinity};
                allowed.intersectWith(RangeUnion::of(lower, upper, 2));
            };
            //Inside the template's polygon the closed loop of some plant of
            //the family is singular: forbidden at every specification.
            for (const Range & inside : locus->rayInside(minus)) {
                forbid(inside);
            }
            //And the plants along every segment between two samples.
            for (const auto & loop : locus->loops()) {
                for (const SingularLocus::Segment & s : loop) {
                    if (allowed.isEmpty()) break;
                    //The plant values at the segment's ends, for the numerators
                    //that need them: q = P0 / P, so P = P0 / q.
                    const std::complex<double> pa = p0 / s.a, pb = p0 / s.b;
                    forbid(segmentForbidden(type, W, p0, s.a, s.b, pa, pb, minus));
                }
            }
        }

        std::vector<BoundaryColumns::Span> & spans = columns[j];
        for (const Range & r : allowed.components()) {
            spans.push_back({toDb(r.min), toDb(r.max)});
        }
    }

    return BoundaryColumns(std::move(columns), static_cast<std::int32_t>(phases.size()), phaseRange);
}

} // namespace qftbx
