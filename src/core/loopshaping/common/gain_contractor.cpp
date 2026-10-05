/**
 * @file
 * @brief The contraction of a box's gain, by the specifications and then by
 * the stability of the plants.
 */

#include "src/core/loopshaping/common/gain_contractor.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "src/core/math/interval.h"

namespace qftbx {

namespace {

constexpr double kKeptMargin = 1e-12;

Interval linearOf(const Interval & decibels)
{
    return exp(decibels * log(Interval(10.0)) / Interval(20.0));
}

}

GainContractor::GainContractor(ExactPointCheck & exact, FamilyStabilityChecker & family,
                               const std::vector<double> & omega,
                               const std::vector<std::complex<double>> & nominalPlantValues)
    : m_exact(exact), m_family(family), m_omega(omega), m_nominalPlantValues(nominalPlantValues)
{
}

GainContractor::Contraction GainContractor::contract(LtiSystem * box)
{
    const Range initial = box->gain().range();
    Contraction result;
    result.gains = initial;

    if (!contractBySpecifications(box, result.gains)) {
        ++m_statistics.emptiedBySpecifications;
        result.outcome = Outcome::EmptiedBySpecifications;
        return result;
    }

    const std::optional<Range> stable = m_family.shaveUnstableGains(box, result.gains);
    if (!stable.has_value()) {
        ++m_statistics.emptiedByStability;
        result.outcome = Outcome::EmptiedByStability;
        return result;
    }
    result.gains = *stable;

    if (result.gains.min > initial.min || result.gains.max < initial.max) {
        ++m_statistics.contracted;
        result.outcome = Outcome::Contracted;
    }
    return result;
}

bool GainContractor::contractBySpecifications(LtiSystem * box, Range & gains)
{
    if (!(gains.min > 0.0)) {
        return true;
    }

    for (std::size_t i = 0; i < m_omega.size(); ++i) {
        const NicholsBox unit = m_extension.nicholsBox(box, m_omega[i], m_nominalPlantValues[i], Interval(1.0));
        const Interval loopDb = Interval(20.0) * log10(Interval(gains.min, gains.max)) + unit.magnitudeDb;

        const ExactPointCheck::SectorVerdict verdict = m_exact.sectorVerdict(
                    i, Range(unit.phaseDegrees.lower(), unit.phaseDegrees.upper()),
                    Range(loopDb.lower(), loopDb.upper()));
        if (verdict.provablyInfeasible) {
            return false;
        }

        if (std::isfinite(verdict.forbiddenBelowDb)) {
            const Interval lowest = linearOf(Interval(verdict.forbiddenBelowDb) - Interval(unit.magnitudeDb.upper()));
            gains.min = std::max(gains.min, lowest.lower() * (1.0 - kKeptMargin));
        }
        if (std::isfinite(verdict.forbiddenAboveDb)) {
            const Interval highest = linearOf(Interval(verdict.forbiddenAboveDb) - Interval(unit.magnitudeDb.lower()));
            gains.max = std::min(gains.max, highest.upper() * (1.0 + kKeptMargin));
        }
        if (gains.min > gains.max) {
            return false;
        }
    }
    return true;
}

}
