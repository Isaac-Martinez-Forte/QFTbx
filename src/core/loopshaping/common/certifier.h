#ifndef QFTBX_CERTIFIER_H
#define QFTBX_CERTIFIER_H

#include <cstddef>

#include "src/core/loopshaping/common/exact_point_check.h"
#include "src/core/loopshaping/common/family_stability_checker.h"
#include "src/core/loopshaping/common/nominal_stability_checker.h"
#include "src/core/loopshaping/common/point_controller.h"

/**
 * @file
 * @brief The one funnel a candidate passes before it leaves a search or
 * prunes it.
 *
 * Under the exact point reading a controller becomes the design returned,
 * or the best design so far, only through this, in order: every plant of
 * the sweep stable by the Routh table, the nominal plant stable by the same
 * table, the nominal closed loop stable by the nominal criterion, the
 * specifications themselves over the whole template at its own loop value
 * (ExactPointCheck), and the nominal plant and the same plants by the roots
 * with the verifier's tolerance. It leaves at the first refusal, the
 * cheapest questions first; the roots settle the band where the table and
 * the verifier's tolerance disagree. So whatever certify accepts the
 * verifier accepts, by construction. certifyAdmitted skips the
 * specifications for a point the exact gain search already admitted. The
 * counts say how many candidates were asked and where each refusal fell.
 */
namespace qftbx {

class Certifier
{
public:
    Certifier(ExactPointCheck & exact, NominalStabilityChecker & stability, FamilyStabilityChecker & family);

    bool certify(const PointController & point);

    bool certifyAdmitted(const PointController & point);

    struct Statistics {
        std::size_t certifications = 0;
        std::size_t refusedByRouth = 0;
        std::size_t refusedByNominalRouth = 0;
        std::size_t refusedByNominalStability = 0;
        std::size_t refusedBySpecifications = 0;
        std::size_t refusedByRoots = 0;
    };
    const Statistics & statistics() const { return m_statistics; }

private:
    bool funnel(const PointController & point, bool askSpecifications);

    ExactPointCheck & m_exact;
    NominalStabilityChecker & m_stability;
    FamilyStabilityChecker & m_family;
    Statistics m_statistics;
};

}

#endif
