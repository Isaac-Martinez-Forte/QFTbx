#ifndef QFTBX_EXACT_POINT_CHECK_H
#define QFTBX_EXACT_POINT_CHECK_H

#include <complex>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "src/core/loopshaping/common/point_controller.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/math/range.h"
#include "src/core/math/range_union.h"
#include "src/core/specifications/specification.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/cloud_set.h"

/**
 * @file
 * @brief The verifier's specification criterion, asked of one candidate of
 * the search at a time, and the exact set of gains it admits at fixed zeros
 * and poles.
 *
 * The searches decide about single controllers with the boundaries: the
 * point's degenerate box is projected onto the Nichols chart and read
 * against the columns of the phase grid. That reading is not the
 * specification. Between two phase nodes a boundary can fall tens of
 * decibels - a type-1 loop sits at the notch of its low-frequency tracking
 * bound, 33 dB deep inside one degree - so the nearest node lets designs
 * through that violate the bound, and the two bracketing nodes together
 * refuse designs that meet it by twenty decibels. With the zeros and poles
 * fixed the loop has an exact phase and magnitude at every design
 * frequency, and the specification can be evaluated there directly, over
 * the whole value set, as the verifier does on the returned design.
 *
 * admits asks exactly that. It holds the SpecificationReference of the
 * problem and a clone of the controller structure, evaluates the loop at
 * each design frequency through the same valueAt the verifier reaches on
 * the returned system, and admits a point only when every excess is at or
 * below minus the tolerance, 1e-12 dB: the verifier accepts an excess of
 * zero, and a candidate put on the very edge of a bound by a root formula
 * must not be returned to be refused by a rounding. So whatever is admitted
 * here the verifier accepts, on the specifications, and the two cannot
 * disagree by an inlined copy of the comparison, because there is one.
 * Exact means exact with respect to the sampled value set, the verifier's
 * own reference: the sampling of the family stays the input. A rejection
 * leaves at the first frequency that fails, and the frequency that failed
 * last is asked first next time, since the candidates of a search resemble
 * one another; an admission always walks every plant of every frequency.
 *
 * lowestAdmissibleGain is the exact best gain at fixed zeros and poles, the
 * T3 step of the MC family done against the specifications instead of a
 * column. With the zeros and poles fixed, the loop at each frequency is
 * K u_i with u_i known, so its phase does not depend on K and every
 * specification becomes a quadratic inequality in the magnitude g = K|u_i|
 * (Chait and Yaniv 1993): for a plant q_n of the value set, the violating
 * set is a disc or a half-plane of the complex plane, and along the ray of
 * the loop's phase it is one quadratic a g^2 + b g + c >= 0 whose
 * coefficients are those of the disc. The tracking spread compares the
 * farthest plant with the nearest, |q_n + L|^2 = g^2 + (2 c_n g + |q_n|^2):
 * a term common to every plant plus a line in g, so the farthest is the
 * upper envelope of the lines and the nearest the lower one, and the
 * spread is a quadratic on each piece of the two envelopes, O(N log N) per
 * frequency rather than the O(N^2) pairs of the literature. The admissible
 * gains are the intersection over the frequencies of these sets carried to
 * the gain's frame, a union of intervals in decibels. The quadratics are
 * solved with each bound lowered by the tolerance, so their roots are where
 * the excess is minus the tolerance, the point admits accepts from: at the
 * low-frequency tracking bound of a type-1 plant the excess moves with the
 * gain a million times more slowly than a decibel per decibel, and a ladder
 * that only covers rounding would never reach the tolerance from a root at
 * excess zero.
 *
 * Doing that over the whole cloud at every node would cost what the
 * verifier costs, so the set is computed over a working set of plants per
 * frequency, seeded with the vertices of the convex hull of the inverse
 * template, which make the farthest plant exact from the start (Rodrigues,
 * Chait and Hollot 1997). A subset of plants gives a superset of admissible
 * gains, so its smallest member is at or below the true one; the candidate
 * is then confirmed by admits over every plant, and if refused, the plants
 * that refused it - the nearest and the farthest to -L, and the worst of
 * the disturbance and effort bounds when those are in force - join the
 * working set and the set is computed again (the exchange method of
 * Blankenship and Falk). A candidate confirmed is the smallest admissible
 * gain, up to the ladder: the gain is taken a hair above the end of the
 * component, 0, 1e-15, ..., 1e-7 relative, capped at the component's
 * middle, until admits agrees, since the roots are computed by a formula
 * and the verifier by evaluation. Each refusal that teaches nothing new
 * climbs the ladder instead; the working sets persist across calls, so the
 * exchange converges once per run and the calls after it cost a few
 * quadratics. admissibleGainsDb is the same set over the whole cloud, for
 * the tests and for reference.
 *
 * A project whose templates do not cover every design frequency has no
 * reference: the check is then unusable and the caller keeps to the
 * columns, as the verifier reports such a design unverified. checkOf gives
 * the full list of a point's excesses, for reporting and for the test that
 * pins the identity with the verifier; the family is not its business.
 */
namespace qftbx {

class ExactPointCheck
{
public:
    static constexpr double kToleranceDb = 1e-12;

    ExactPointCheck(LtiSystem & plant, LtiSystem * controller, const std::vector<double> & omega,
                    const CloudSet & templates, const SpecificationSet & specifications);

    bool usable() const { return m_reference.has_value(); }

    bool admits(const PointController & point);

    SpecificationCheck checkOf(const PointController & point) const;

    struct GainSearch {
        std::optional<double> gain;
        std::size_t rounds = 0;
        std::size_t ladderSteps = 0;
        std::size_t confirmations = 0;
    };

    GainSearch lowestAdmissibleGain(const std::vector<double> & zeros, const std::vector<double> & poles,
                                    Range gainRange);

    RangeUnion admissibleGainsDb(const std::vector<double> & zeros, const std::vector<double> & poles,
                                 Range gainRange) const;

    struct Statistics {
        std::size_t verdicts = 0;
        std::size_t rejections = 0;
        std::size_t kernelPasses = 0;
        std::size_t gainSearches = 0;
        std::size_t exchangeRounds = 0;
        std::size_t ladderSteps = 0;
        std::size_t largestWorkingSet = 0;
    };
    const Statistics & statistics() const { return m_statistics; }

private:
    std::complex<double> loopAt(const FrequencyReference & at, const PointController & point) const;

    void requireUsable() const;

    RangeUnion admissibleMagnitudes(const FrequencyReference & at, std::complex<double> direction,
                                    const std::vector<std::size_t> & plants) const;

    RangeUnion admissibleGainsDbOver(const std::vector<double> & zeros, const std::vector<double> & poles,
                                     Range gainRange,
                                     const std::vector<std::vector<std::size_t>> * workingSets) const;

    bool growWorkingSet(std::size_t frequency, const PointController & point);

    std::unique_ptr<LtiSystem> m_controller;
    std::optional<SpecificationReference> m_reference;
    std::vector<std::vector<std::size_t>> m_working;
    std::size_t m_firstToAsk = 0;
    Statistics m_statistics;
};

}

#endif
