#include "project_data.h"

namespace qftbx {

namespace {

void deleteSpecifications(QVector<SpecificationRecord *> * specifications)
{
    if (specifications == nullptr) {
        return;
    }

    foreach (SpecificationRecord * record, *specifications) {
        if (record != nullptr) {
            delete record->system;
            delete record;
        }
    }

    delete specifications;
}


} // namespace

ProjectData::~ProjectData()
{
    deleteSpecifications(m_specifications);
}

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

QVector<qreal> * ProjectData::frequencies() const
{
    return m_omega != nullptr ? m_omega->values() : nullptr;
}

QVector<SpecificationRecord *> * ProjectData::specifications() const
{
    return m_specifications;
}

void ProjectData::setSpecifications(QVector<SpecificationRecord *> * specifications)
{
    if (m_specifications != specifications) {
        deleteSpecifications(m_specifications);
    }
    m_specifications = specifications;
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

    //Set even for an empty contour: "was one ever computed" is a different
    //question from "is it non-empty", and dropping the templates has to be
    //able to answer no.
    m_hasContour = !m_contour.empty();
}

bool ProjectData::hasContour() const
{
    return m_hasContour;
}

QVector<qreal> * ProjectData::epsilon()
{
    return m_epsilon.has_value() ? &m_epsilon.value() : nullptr;
}

void ProjectData::setEpsilon(std::optional<QVector<qreal>> epsilon)
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
