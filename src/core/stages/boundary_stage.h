#ifndef QFTBX_BOUNDARY_STAGE_H
#define QFTBX_BOUNDARY_STAGE_H

#include <cstdint>
#include <memory>

#include "src/core/boundaries/boundary_engine.h"
#include "src/core/project_data.h"
#include "src/core/range.h"

namespace qftbx {

/**
 * @brief The boundary stage: the QFT bounds on the Nichols plane, one set per
 * design frequency, plus their union.
 *
 * Like TemplateStage, it owns its preconditions, its engine, its parameters
 * and the publishing of its output - and NOT the dependency graph. A new set
 * of boundaries voids the design the search found against the old ones, but
 * that is applied by the facade.
 */
class BoundaryStage
{
public:
    /**
     * @brief Throws InvalidInput naming what is missing.
     * @param fromContour whether the computation will read the CONTOUR of
     *        each template or the whole cloud: which of the two has to be
     *        there depends on it.
     */
    void requirePrerequisites(const ProjectData & data, bool fromContour) const;

    /**
     * @brief Computes the boundaries over the given Nichols grid and
     * publishes them.
     * @return true when it produced a set.
     */
    bool run(ProjectData & data, Range phaseRange, std::int32_t phaseCount,
             Range magnitudeRange, std::int32_t magnitudeCount,
             double exportInfinity, bool fromContour, bool cuda);

private:
    BoundaryEngine & engine();

    std::unique_ptr<BoundaryEngine> m_engine;
};

} // namespace qftbx

#endif // QFTBX_BOUNDARY_STAGE_H
