/**
 * @file
 * @brief The certification funnel, the Routh table first.
 */

#include "src/core/loopshaping/common/certifier.h"

namespace qftbx {

Certifier::Certifier(ExactPointCheck & exact, NominalStabilityChecker & stability, FamilyStabilityChecker & family)
    : m_exact(exact), m_stability(stability), m_family(family)
{
}

bool Certifier::certify(const PointController & point, bool specificationsAdmitted)
{
    ++m_statistics.certifications;

    if (!m_family.isStable(point)) {
        ++m_statistics.refusedByRouth;
        return false;
    }
    if (!m_stability.isNominallyStable(point)) {
        ++m_statistics.refusedByNominalStability;
        return false;
    }
    if (!specificationsAdmitted && !m_exact.admits(point)) {
        ++m_statistics.refusedBySpecifications;
        return false;
    }
    if (!m_family.isStableByRoots(point)) {
        ++m_statistics.refusedByRoots;
        return false;
    }

    return true;
}

}
