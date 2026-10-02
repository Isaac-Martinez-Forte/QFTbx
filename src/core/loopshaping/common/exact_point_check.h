#ifndef QFTBX_EXACT_POINT_CHECK_H
#define QFTBX_EXACT_POINT_CHECK_H

#include <complex>
#include <cstddef>
#include <limits>
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
 * the search at a time, the exact set of gains it admits at fixed zeros and
 * poles, and the verdict it gives on a whole sector of loop values.
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
 * here the verifier accepts, on the specifications. The comparison is one,
 * but the loop is evaluated here and again in the verifier, and the two
 * evaluations may round differently in their last bit, as the compiler
 * fuses their products in one place and not in the other. The tolerance,
 * orders of magnitude above such a rounding, absorbs it.
 * Exact means exact with respect to the sampled value set, the verifier's
 * own reference: the sampling of the family stays the input. A rejection
 * leaves at the first frequency that fails, and the frequency that failed
 * last is asked first next time, since the candidates of a search resemble
 * one another; an admission always walks every plant of every frequency.
 * The gain search below keeps a rotation of its own, so which frequency
 * its confirmations fail at, and so which working set grows, depends on
 * its own candidates alone and not on what else the search asked in
 * between.
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
 * quadratics. An exchange that has not settled after 64 rounds gives up on
 * the vertex, which then offers no gain: it costs a candidate, never a
 * proof. The rounds of a search are those in which a working set grew, and
 * the statistics count apart the two ways a search gives up on a vertex
 * whose admissible set is not empty: a ladder climbed to its top with
 * neither a confirmation nor a plant to learn from, and the 64 rounds
 * spent. admissibleGainsDb is the same set over the whole cloud, for the
 * tests and for reference.
 *
 * sectorVerdict is the same geometry asked of a whole box of controllers,
 * given the enclosure of its loop at one frequency, a phase interval and a
 * magnitude interval, that is an annular sector of the complex plane. Each
 * plant's violating set is a disc, and the quadratic F(g, c) of a plant is
 * increasing in the cosine term c, so its largest value over the phase
 * interval is at the largest cosine, which is at an end of the interval or
 * at the plant's own direction when that lies inside; F below zero there
 * for every magnitude of the sector puts the whole sector inside that
 * plant's disc, and the box is infeasible with a proof. The magnitudes that
 * F at the largest cosine forbids are forbidden at every phase of the
 * interval, so their union over the plants, where it covers a strip from
 * zero upwards or up to infinity, is a strip the search may cut off the box
 * with a proof; the pairs of the tracking spread enter the same way, the
 * farther plant among the vertices of the hull and the nearer among the
 * working set, a subset of the discs, which proves less but never wrongly.
 * These are exact with respect to the value set, and conservative with
 * respect to the box only through its enclosure. The bound is not lowered
 * by the tolerance here: what is discarded is proven to violate the
 * specification itself. The moduli and directions a verdict needs, of each
 * plant and of each tracking pair, depend on the frequency alone, so they
 * are computed once, the pairs again only when a working set has grown,
 * and a verdict is products with the cosines and sines of the ends of the
 * phase interval.
 *
 * A project whose templates do not cover every design frequency has no
 * reference: the check is then unusable and MC2 refuses to run, as the
 * verifier reports such a design unverified. checkOf gives
 * the full list of a point's excesses, for the tests, among them the one
 * that pins the identity with the verifier; the family is not its business.
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

    struct SectorVerdict {
        bool provablyInfeasible = false;
        double forbiddenBelowDb = -std::numeric_limits<double>::infinity();
        double forbiddenAboveDb = std::numeric_limits<double>::infinity();
    };

    SectorVerdict sectorVerdict(std::size_t omegaIndex, Range phaseDegrees, Range magnitudeDb);

    struct Statistics {
        std::size_t verdicts = 0;
        std::size_t kernelPasses = 0;
        std::size_t gainSearches = 0;
        std::size_t exchangeRounds = 0;
        std::size_t ladderSteps = 0;
        std::size_t laddersExhausted = 0;
        std::size_t roundLimitsReached = 0;
        std::size_t largestWorkingSet = 0;
        std::size_t sectorVerdicts = 0;
    };
    const Statistics & statistics() const { return m_statistics; }

private:
    struct Quotient {
        double modulus;
        double cosine;
        double sine;
        double norm;
        double plantModulus;
    };

    struct TrackingPair {
        double modulus;
        double cosine;
        double sine;
        double constant;
    };

    struct TrackingPairs {
        std::size_t workingSize = 0;
        std::vector<TrackingPair> pairs;
    };

    void requireUsable() const;

    std::complex<double> loopAt(const FrequencyReference & at, const PointController & point) const;

    bool admitsFrom(const PointController & point, std::size_t & firstToAsk);

    RangeUnion admissibleMagnitudes(std::size_t frequency, std::complex<double> direction,
                                    const std::vector<std::size_t> & plants) const;

    RangeUnion trackingMagnitudes(std::size_t frequency, std::complex<double> direction,
                                  const std::vector<std::size_t> & plants, double boundDb) const;

    RangeUnion admissibleGainsDbOver(const std::vector<double> & zeros, const std::vector<double> & poles,
                                     Range gainRange,
                                     const std::vector<std::vector<std::size_t>> * workingSets) const;

    bool growWorkingSet(std::size_t frequency, const PointController & point);

    const std::vector<TrackingPair> & trackingPairs(std::size_t frequency, std::size_t bound);

    std::unique_ptr<LtiSystem> m_controller;
    std::optional<SpecificationReference> m_reference;
    std::vector<std::vector<std::size_t>> m_working;
    std::vector<std::vector<std::size_t>> m_hull;
    std::vector<std::size_t> m_referenceOf;
    std::vector<std::vector<Quotient>> m_quotients;
    std::vector<TrackingPairs> m_pairs;
    std::vector<double> m_forbiddenLower;
    std::vector<double> m_forbiddenUpper;
    std::size_t m_firstToAsk = 0;
    std::size_t m_ladderFirstToAsk = 0;
    Statistics m_statistics;
};

}

#endif
