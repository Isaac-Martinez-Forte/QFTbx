#ifndef QFTBX_FAMILY_STABILITY_CHECKER_H
#define QFTBX_FAMILY_STABILITY_CHECKER_H

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "src/core/loopshaping/common/point_controller.h"
#include "src/core/loopshaping/common/swept_family.h"
#include "src/core/math/range.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/parameter_grids.h"

/**
 * @file
 * @brief Whether controllers leave every plant of the swept family, and the
 * nominal plant, asymptotically stable in closed loop: a candidate by the
 * Routh table or by the roots, a whole box proven unstable, and the gains
 * of a box narrowed with proofs.
 *
 * Meeting the bounds at the design frequencies does not make the closed
 * loop stable, since the crossing that decides it can fall between them.
 * The checker closes the loop with every member of the family the
 * templates were swept over, and with the nominal plant, and asks whether
 * \f$ N_P N_C + D_P D_C \f$ is Hurwitz: exact algebra on the plants in hand,
 * so it belongs inside the search. A delay, a plant or controller that is
 * not rational, or a project with no record of its sweep leaves the family
 * unusable: isStable and isStableByRoots then answer stable and nothing is
 * proven of a box. The nominal plant needs no sweep.
 *
 * isStable is the Routh table per member, the member that failed last
 * asked first; isStableAtNominal is the same table with the nominal plant,
 * which needs no frequency grid; isStableByRoots is the verifier's own
 * criterion (rootVerdictOf), with its tolerance on the axis, so that the
 * verifier cannot refuse what the search returns.
 *
 * isBoxUnstable proves that every controller of a box destabilises some
 * plant: the box's zeros and poles become interval coefficients, the plants
 * that refused last are closed with them, and the Routh table in interval
 * arithmetic (math/interval_polynomial.h) proves every member non-Hurwitz
 * or says nothing. The gain enters the polynomial affinely, A(s) + k B(s),
 * in several coefficients, so its interval is bisected, twelve levels at
 * most, and a plant that leaves the lower corner stable is not asked.
 * isBoxUnstableAtNominal does the same with the nominal plant and bisects
 * the zeros and poles too, for zero-pole-gain and time-constant structures.
 *
 * shaveUnstableGains narrows a gain interval by the same table over pieces
 * of the gain, with a working set of up to eight plants. Where the table
 * stalls, as on a near cancellation of a zero and a pole, the zero
 * exclusion proves a box unstable instead (shaveByZeroExclusion, and
 * isBoxUnstableAtNominalOnAxis with the nominal plant): an unstable point
 * of the box, proven by the table, and a characteristic polynomial that
 * cannot vanish on the imaginary axis over the box, shown in polar form
 * over pieces of the frequency.
 */
namespace qftbx {

class FamilyStabilityChecker
{
public:
    FamilyStabilityChecker(LtiSystem * plant, LtiSystem * controller, const ParameterGrids & sweep);

    bool usable() const { return m_family.usable(); }

    bool isStable(const PointController & point);

    bool isStableAtNominal(const PointController & point);

    bool isStableByRoots(const PointController & point);

    bool isBoxUnstable(LtiSystem * box);

    bool isBoxUnstableAtNominal(LtiSystem * box);

    std::optional<Range> shaveUnstableGains(LtiSystem * box, Range gains);

    std::optional<Range> shaveByZeroExclusion(LtiSystem * box, Range gains);

    bool isBoxUnstableAtNominalOnAxis(LtiSystem * box);

    struct Statistics {
        std::size_t verdicts = 0;
        std::size_t nominalVerdicts = 0;
        std::size_t rootVerdicts = 0;
        std::size_t boxVerdicts = 0;
        std::size_t boxPrunes = 0;
        std::size_t nominalBoxVerdicts = 0;
        std::size_t nominalBoxPrunes = 0;
        std::size_t gainShaves = 0;
        std::size_t gainPiecesProven = 0;
        std::size_t workingSetExchanges = 0;
        std::size_t axisExclusions = 0;
        std::size_t axisProofs = 0;
        std::size_t nominalAxisPrunes = 0;
    };
    const Statistics & statistics() const { return m_statistics; }

private:
    void rememberRefuser(std::size_t member);

    const LtiSystem::Polynomials & workingPlant(std::size_t member) const;

    void addWorkingPlant(std::size_t member);

    std::optional<std::size_t> refuserOutsideWorkingSet(const PointController & point);

    std::unique_ptr<LtiSystem> m_controller;
    SweptFamily m_family;
    std::optional<LtiSystem::Polynomials> m_nominal;
    std::vector<std::size_t> m_recentRefusers;
    std::vector<std::size_t> m_working;
    std::size_t m_shavesAsked = 0;
    double m_lastAxisFailure = -1.0;
    Statistics m_statistics;
};

}

#endif
