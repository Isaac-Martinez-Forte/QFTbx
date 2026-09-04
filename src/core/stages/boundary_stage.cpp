#include "src/core/stages/boundary_stage.h"

#include "src/core/exception.h"

namespace qftbx {

BoundaryEngine & BoundaryStage::engine()
{
    if (m_engine == nullptr) {
        m_engine = std::make_unique<BoundaryEngine>();
    }

    return *m_engine;
}

void BoundaryStage::requirePrerequisites(const ProjectData & data,
                                         bool fromContour) const
{
    if (data.plant() == nullptr || data.frequencies() == nullptr) {
        throw InvalidInput("The boundaries need a plant and a set of "
                           "design frequencies.");
    }
    if (data.specifications() == nullptr) {
        throw InvalidInput("The boundaries need the specifications.");
    }
    if ((fromContour ? data.contour() : data.templates()).empty()) {
        throw InvalidInput("The boundaries need the templates, which "
                           "have to be recomputed after the plant or "
                           "the design frequencies change.");
    }
}

bool BoundaryStage::run(ProjectData & data, Range phaseRange,
                        std::int32_t phaseCount, Range magnitudeRange,
                        std::int32_t magnitudeCount, double exportInfinity,
                        bool fromContour, bool cuda)
{
    requirePrerequisites(data, fromContour);

    BoundaryEngine & bounds = engine();

    bounds.compute(data.frequencies(), data.plant(),
                   fromContour ? data.contour() : data.templates(),
                   data.specifications(), phaseRange, phaseCount,
                   magnitudeRange, magnitudeCount, exportInfinity, cuda);

    data.setBoundaries(bounds.boundaryData());

    return true;
}

} // namespace qftbx
