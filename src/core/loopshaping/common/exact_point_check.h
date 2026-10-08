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
#include "src/core/math/line_envelope.h"
#include "src/core/math/range.h"
#include "src/core/math/range_union.h"
#include "src/core/specifications/specification.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/cloud_set.h"

/**
 * @file
 * @brief The verifier's specification criterion asked of one candidate at a
 * time, the exact set of gains it admits at fixed zeros and poles, and its
 * verdict on a whole sector of loop values.
 *
 * With the zeros and poles fixed the loop has an exact phase and magnitude
 * at each design frequency, so the specifications are evaluated there,
 * over the whole sampled value set as the verifier does, and not read off
 * the columns of the phase grid. admits accepts a point when every excess
 * is at or below minus the tolerance, 1e-12 dB, which absorbs the last-bit
 * differences between this evaluation and the verifier's. The frequency
 * that failed last is asked first.
 *
 * lowestAdmissibleGain is the exact best gain at fixed zeros and poles.
 * The loop at each frequency is K u_i, so every specification is a
 * quadratic inequality in g = K|u_i| (Chait and Yaniv 1993): a disc or a
 * half-plane per plant, and for the tracking spread a quadratic on each
 * piece of the upper and lower envelopes of lines, O(N log N) per
 * frequency. The admissible gains are their intersection over the
 * frequencies, a union of intervals in decibels, with every bound lowered
 * by the tolerance. They are computed over a working set of plants per
 * frequency, seeded with the vertices of the convex hull of the inverse
 * template (Rodrigues, Chait and Hollot 1997); the candidate is confirmed
 * by admits over every plant, and the plants that refuse it join the
 * working set (the exchange method of Blankenship and Falk). The gain is
 * taken a hair above the end of its component, up a ladder from 0 to 1e-7
 * relative, until admits agrees; an exchange not settled after 64 rounds
 * gives up on the vertex, which costs a candidate, never a proof.
 * admissibleGainsDb is the same set over the whole cloud. The buffers are
 * kept in the check, which answers one call at a time.
 *
 * sectorVerdict asks the same geometry of a whole box, given the enclosure
 * of its loop at one frequency: an annular sector. A plant's quadratic is
 * largest at the largest cosine over the phase interval; if that is below
 * zero at every magnitude of the sector, the sector lies in the plant's
 * disc and the box is infeasible with a proof, and the magnitudes it
 * forbids at every phase, where their union covers a strip from zero or up
 * to infinity, are a strip the search may cut off. The tracking pairs take
 * the farther plant among the vertices of the hull and the nearer among
 * the working set, which proves less but never wrongly. The bounds are not
 * lowered here: what is discarded violates the specification itself. A
 * plant whose quadratic has positive leading and constant terms can only
 * be negative when the interval lies within an angle of the direction
 * opposite to it, so those plants are kept sorted by direction and a
 * verdict visits only the ones in that window. The last verdict of each
 * frequency is remembered while its working set stands.
 *
 * Without templates for every design frequency the check is unusable and
 * MC2 refuses to run. checkOf gives the full list of a point's excesses.
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
                                 Range gainRange);

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

    struct WindowedPlantEntry {
        double angle;
        std::size_t index;
    };

    struct PlantWindow {
        std::vector<std::size_t> always;
        std::vector<double> angles;
        std::vector<std::size_t> byAngle;
        double halfWidth = 0.0;

        void sortByAngle(std::vector<WindowedPlantEntry> & windowed);

        template <class Visit>
        void visit(double from, double to, Visit && visitPlant) const;
    };

    struct TrackingPairs {
        std::size_t workingSize = 0;
        std::vector<TrackingPair> pairs;
        PlantWindow window;
    };

    struct Scratch {
        std::vector<math::Line> lines;
        math::EnvelopeScratch envelope;
        std::vector<math::EnvelopePiece> nearest;
        std::vector<math::EnvelopePiece> farthest;
        std::vector<double> trackingLower;
        std::vector<double> trackingUpper;
        RangeUnion trackingPiece;
        RangeUnion tracking;
        RangeUnion quadratic;
        std::vector<std::size_t> everyPlant;
        RangeUnion magnitudes;
        std::vector<double> gainLower;
        std::vector<double> gainUpper;
        RangeUnion gainParts;
    };

    struct RememberedSector {
        bool valid = false;
        std::size_t workingSize = 0;
        Range phase;
        Range magnitude;
        SectorVerdict verdict;
    };

    void requireUsable() const;

    std::complex<double> loopAt(const FrequencyReference & at, const PointController & point) const;

    bool admitsFrom(const PointController & point, std::size_t & firstToAsk);

    void admissibleMagnitudes(std::size_t frequency, std::complex<double> direction,
                              const std::vector<std::size_t> & plants, RangeUnion & allowed);

    void trackingMagnitudes(std::size_t frequency, std::complex<double> direction,
                            const std::vector<std::size_t> & plants, double boundDb, RangeUnion & set);

    RangeUnion admissibleGainsDbOver(const std::vector<double> & zeros, const std::vector<double> & poles,
                                     Range gainRange,
                                     const std::vector<std::vector<std::size_t>> * workingSets);

    bool growWorkingSet(std::size_t frequency, const PointController & point);

    const TrackingPairs & trackingPairs(std::size_t frequency, std::size_t bound);

    std::unique_ptr<LtiSystem> m_controller;
    std::optional<SpecificationReference> m_reference;
    std::vector<std::vector<std::size_t>> m_working;
    std::vector<std::vector<std::size_t>> m_hull;
    std::vector<std::size_t> m_referenceOf;
    std::vector<std::vector<Quotient>> m_quotients;
    std::vector<std::vector<PlantWindow>> m_windows;
    std::vector<TrackingPairs> m_pairs;
    std::vector<RememberedSector> m_sectors;
    Scratch m_scratch;
    std::vector<double> m_forbiddenLower;
    std::vector<double> m_forbiddenUpper;
    std::size_t m_firstToAsk = 0;
    std::size_t m_ladderFirstToAsk = 0;
    Statistics m_statistics;
};

}

#endif
