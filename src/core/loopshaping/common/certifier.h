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
 * Under the exact point reading a controller may become the design the
 * search returns, or the best design so far that cuts every box above its
 * gain, only through this: the specifications themselves over the whole
 * template at its own loop value (ExactPointCheck), the nominal closed loop
 * stable, every plant of the sweep stable by the Routh table, and then the
 * same plants by the roots with the verifier's tolerance. The order is the
 * order of cost, and the funnel leaves at the first refusal. The last step
 * exists because the Routh table and the roots disagree within the
 * tolerance the roots are computed to, and a design accepted by one and
 * refused by the other would be returned and then rejected by the verifier;
 * on the DC motor the optimum sits exactly in that band.
 *
 * So whatever comes out of certify the verifier accepts, on the
 * specifications and on the family, by construction; and nothing the search
 * prunes with can be refused afterwards. The counts say how many candidates
 * were asked and where each refusal fell, which is what the cost of the
 * exact reading is measured by.
 */
namespace qftbx {

class Certifier
{
public:
    Certifier(ExactPointCheck & exact, NominalStabilityChecker & stability, FamilyStabilityChecker & family);

    bool certify(const PointController & point);

    struct Statistics {
        std::size_t certifications = 0;
        std::size_t refusedBySpecifications = 0;
        std::size_t refusedByNominalStability = 0;
        std::size_t refusedByRouth = 0;
        std::size_t refusedByRoots = 0;
    };
    const Statistics & statistics() const { return m_statistics; }

private:
    ExactPointCheck & m_exact;
    NominalStabilityChecker & m_stability;
    FamilyStabilityChecker & m_family;
    Statistics m_statistics;
};

}

#endif
