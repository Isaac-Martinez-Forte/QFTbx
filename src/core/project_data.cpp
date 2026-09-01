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
    delete m_plant;
    delete m_omega;
    deleteSpecifications(m_specifications);
    delete m_epsilon;
    delete m_controller;
    delete m_loopShaping;
}

LtiSystem * ProjectData::plant() const
{
    return m_plant;
}

void ProjectData::setPlant(LtiSystem * plant)
{
    if (m_plant != plant) {
        delete m_plant;
    }
    m_plant = plant;
}

Omega * ProjectData::omega() const
{
    return m_omega;
}

void ProjectData::setOmega(Omega * omega)
{
    if (m_omega != omega) {
        delete m_omega;
    }
    m_omega = omega;
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

QVector<qreal> * ProjectData::epsilon() const
{
    return m_epsilon;
}

void ProjectData::setEpsilon(QVector<qreal> * epsilon)
{
    if (m_epsilon != epsilon) {
        delete m_epsilon;
    }
    m_epsilon = epsilon;
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
    return m_controller;
}

void ProjectData::setController(LtiSystem * controller)
{
    if (m_controller != controller) {
        delete m_controller;
    }
    m_controller = controller;
}

LoopShapingResult * ProjectData::loopShaping() const
{
    return m_loopShaping;
}

void ProjectData::setLoopShapingResult(LoopShapingResult * loopShaping)
{
    if (m_loopShaping != loopShaping) {
        delete m_loopShaping;
    }
    m_loopShaping = loopShaping;
}

} // namespace qftbx
