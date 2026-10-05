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
 * @brief Whether a candidate controller leaves every plant of the swept
 * family asymptotically stable in closed loop.
 *
 * The QFT bounds are imposed at a handful of design frequencies chosen by
 * the engineer, and satisfying them there does not make the closed loop
 * stable: the crossing that decides it can fall between two of them. That
 * is why a search can return, and did return, a design that meets every
 * bound and leaves a third of the family unstable.
 *
 * This closes the loop with every member of the family the templates were
 * swept over and asks whether the characteristic polynomial
 * \f$ N_P N_C + D_P D_C \f$ is Hurwitz. It adds no frequency to the problem
 * the engineer posed - it is exact algebra on the plants already in hand -
 * so it belongs inside the search, as the last thing asked of a candidate
 * before it is returned. Validating a design against a denser set of
 * requirements is a different job and a later step.
 *
 * The plants' polynomials are the SweptFamily's, built once, when the
 * checker is, and the nominal plant's beside them. A plant or controller
 * with a delay, one that is not a rational function, or a project with no
 * record of its sweep leaves the family unusable: isStable and
 * isStableByRoots then answer stable, which leaves the caller where it would
 * be without the checker, and isBoxUnstable answers nothing proven. The
 * nominal plant needs no sweep, only rational polynomials and no delay in
 * the plant or the controller, so isBoxUnstableAtNominal proves boxes also
 * on the problems whose family was never swept, and with the family gate
 * off, where the search hands the checker no sweep.
 *
 * isStable is the Routh table per member, tens of microseconds for a family
 * of hundreds, stopping at the first member that fails; the member that
 * failed last is asked first, since the candidates of a search resemble one
 * another and a refusal then costs one table.
 *
 * isStableAtNominal is the same Routh table with the nominal plant. The
 * nominal criterion decides the nominal loop from a frequency grid, and a
 * grid can miss the crossing that makes a loop unstable: on the ACC'90
 * problem with its robust performance specifications it approved a loop
 * whose nominal closed loop has its poles at +2.22 +- 2.42j. The table needs
 * no grid and no sweep, only the nominal plant's polynomials.
 *
 * isStableByRoots is the verifier's own criterion (rootVerdictOf) on the
 * nominal plant and on the same members, the roots of every characteristic
 * polynomial with the tolerance the verifier applies to the axis,
 * milliseconds rather than microseconds: the two agree except in that
 * tolerance band, and a candidate that is to be returned under the exact
 * reading is confirmed by the second so that the verifier cannot refuse
 * afterwards what the search accepted. It is the tolerance, not the table,
 * that refuses a design whose pole sits on the axis but for a rounding.
 *
 * isBoxUnstable asks the opposite question of a whole box of controllers:
 * whether every one of them destabilises some plant of the family. The box's
 * zeros and poles become interval coefficients of the controller's
 * polynomials, the plants that refused most recently (four are kept, the
 * first plant that refuses a point is rarely the one that refuses it most)
 * are closed with them one by one, and the Routh table in interval arithmetic
 * (math/interval_polynomial.h) either proves every member non-Hurwitz or says
 * nothing. The gain enters the characteristic polynomial affinely,
 * A(s) + k B(s), and appears in several coefficients at once, so a single
 * interval for it makes the enclosure of a Routh entry straddle zero long
 * before the box does; the gain interval is therefore bisected, twelve levels
 * at most, until every piece is proven or a piece becomes negligible. Before
 * any of that, the lower corner of the box is closed with the same plant by
 * the real Routh table: a plant that leaves that corner stable proves nothing
 * about the box, and that answer costs a table. A search that reaches the
 * gain the specifications need only beyond the family's stability limit
 * bisects such boxes down to its resolution and tries their vertices one by
 * one; this discards them whole, with a proof.
 *
 * isBoxUnstableAtNominal asks the same of the nominal plant, which is what
 * the nominal criterion's own box test answers from a frequency grid, so that
 * a discard on that answer can be told proven from grid-backed. There the
 * zeros and poles are bisected along with the gain, the widest relative to
 * its size first, since on the DC motor of Tharewal's example 3.1 the gain
 * alone proves none of the boxes that criterion refuses and every parameter
 * proves a third of them. Only structures whose polynomials are products of
 * linear factors are asked (zero-pole-gain and time-constant forms).
 *
 * shaveUnstableGains narrows a gain interval by the same table over pieces
 * of the gain, with a working set of up to eight plants. Where the table
 * stalls, as on a near cancellation of a zero and a pole, the zero exclusion
 * proves a box unstable instead (shaveByZeroExclusion, and
 * isBoxUnstableAtNominalOnAxis with the nominal plant): an unstable point of
 * the box, proven by the table, and a characteristic polynomial that cannot
 * vanish on the imaginary axis over the box, shown in polar form over pieces
 * of the frequency, leave every controller of the box unstable.
 *
 * It keeps a clone of the controller structure, since the search gives its
 * own away to the first box of the list.
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
