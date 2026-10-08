/**
 * @file
 * @brief Narrows the gain of a box of controllers to what can still hold a
 * design, with a proof for every gain removed.
 *
 * Two proofs, in order. The specifications: at each design frequency the
 * magnitude strips the exact sector verdict forbids at every phase of the
 * box's enclosure, taken whole and not folded onto one turn, are carried to
 * the gain in interval arithmetic. The stability: the Routh table over
 * pieces of the gain, with a few plants
 * (FamilyStabilityChecker::shaveUnstableGains), and then the zero exclusion
 * on the imaginary axis where the table stalls. An empty interval discards
 * the box. The frequency that emptied the last box is asked first, on a
 * copy of the gains, since what it proves of the whole interval holds of
 * any part. The strip ends are floating-point roots, as in the search's
 * cuts, and rest on the 1e-12 dB the certification funnel keeps from every
 * bound.
 */

#ifndef QFTBX_LOOPSHAPING_GAIN_CONTRACTOR_H
#define QFTBX_LOOPSHAPING_GAIN_CONTRACTOR_H

#include <complex>
#include <cstddef>
#include <limits>
#include <vector>

#include "src/core/loopshaping/common/exact_point_check.h"
#include "src/core/loopshaping/common/family_stability_checker.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/math/range.h"
#include "src/core/system/lti_system.h"

namespace qftbx {

class GainContractor
{
public:
    GainContractor(ExactPointCheck & exact, FamilyStabilityChecker & family, const std::vector<double> & omega,
                   const std::vector<std::complex<double>> & nominalPlantValues);

    enum class Outcome { Unchanged, Contracted, EmptiedBySpecifications, EmptiedByStability, EmptiedByZeroExclusion };

    struct Contraction {
        Outcome outcome = Outcome::Unchanged;
        Range gains;
    };

    Contraction contract(LtiSystem * box);

    struct Statistics {
        std::size_t contracted = 0;
        std::size_t emptiedBySpecifications = 0;
        std::size_t emptiedByStability = 0;
        std::size_t emptiedByZeroExclusion = 0;
    };
    const Statistics & statistics() const { return m_statistics; }

private:
    bool contractBySpecifications(LtiSystem * box, Range & gains);
    bool contractAt(LtiSystem * box, std::size_t frequency, Range & gains);

    ExactPointCheck & m_exact;
    FamilyStabilityChecker & m_family;
    const std::vector<double> & m_omega;
    const std::vector<std::complex<double>> & m_nominalPlantValues;
    NaturalIntervalExtension m_extension;
    std::size_t m_lastEmptiedAt = std::numeric_limits<std::size_t>::max();
    Statistics m_statistics;
};

}

#endif
