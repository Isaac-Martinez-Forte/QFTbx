#include "project_data.h"

namespace qftbx {

LtiSystem * ProjectData::plant() const
{
    return m_plant.get();
}

void ProjectData::setPlant(std::unique_ptr<LtiSystem> plant)
{
    m_plant = std::move(plant);
}

Omega * ProjectData::omega() const
{
    return m_omega.get();
}

void ProjectData::setOmega(std::unique_ptr<Omega> omega)
{
    m_omega = std::move(omega);
}

std::vector<double> * ProjectData::frequencies() const
{
    return m_omega != nullptr ? m_omega->values() : nullptr;
}

SpecificationRecords * ProjectData::specifications()
{
    return m_specifications.has_value() ? &m_specifications.value() : nullptr;
}

const SpecificationRecords * ProjectData::specifications() const
{
    return m_specifications.has_value() ? &m_specifications.value() : nullptr;
}

void ProjectData::setSpecifications(std::optional<SpecificationRecords> specifications)
{
    m_specifications = std::move(specifications);
}

//By value: the assignment frees the previous set, so the "delete what you
//replace" rule these setters used to spell out is now the language's job.
const CloudSet & ProjectData::templates() const
{
    return m_templates;
}

void ProjectData::setTemplates(CloudSet templates)
{
    m_templates = std::move(templates);
}

const CloudSet & ProjectData::contour() const
{
    return m_contour;
}

void ProjectData::setContour(CloudSet contour)
{
    m_contour = std::move(contour);

    //Follows the contour itself: publishing an empty one - which is how the
    //templates are dropped - clears it, so the writer does not save a contour
    //section for nothing.
    m_hasContour = !m_contour.empty();
}

bool ProjectData::hasContour() const
{
    return m_hasContour;
}

std::vector<double> * ProjectData::epsilon()
{
    return m_epsilon.has_value() ? &m_epsilon.value() : nullptr;
}

void ProjectData::setEpsilon(std::optional<std::vector<double>> epsilon)
{
    m_epsilon = std::move(epsilon);
}

BoundaryData * ProjectData::boundaries()
{
    return m_boundaries.has_value() ? &m_boundaries.value() : nullptr;
}

const BoundaryData * ProjectData::boundaries() const
{
    return m_boundaries.has_value() ? &m_boundaries.value() : nullptr;
}

void ProjectData::setBoundaries(std::optional<BoundaryData> boundaries)
{
    m_boundaries = std::move(boundaries);
}

LtiSystem * ProjectData::controller() const
{
    return m_controller.get();
}

void ProjectData::setController(std::unique_ptr<LtiSystem> controller)
{
    m_controller = std::move(controller);
}

LoopShapingResult * ProjectData::loopShaping() const
{
    return m_loopShaping.get();
}

void ProjectData::setLoopShapingResult(std::unique_ptr<LoopShapingResult> loopShaping)
{
    m_loopShaping = std::move(loopShaping);
}

} // namespace qftbx
