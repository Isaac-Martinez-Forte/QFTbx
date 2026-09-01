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

void deleteClouds(QVector<QVector<std::complex<qreal>> *> * clouds)
{
    if (clouds != nullptr) {
        qDeleteAll(*clouds);
        delete clouds;
    }
}

} // namespace

ProjectData::~ProjectData()
{
    delete m_plant;
    delete m_omega;
    deleteSpecifications(m_specifications);
    deleteClouds(m_templates);
    deleteClouds(m_contour);
    delete m_epsilon;
    delete m_boundaries;
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

QVector<QVector<std::complex<qreal>> *> * ProjectData::templates() const
{
    return m_templates;
}

void ProjectData::setTemplates(QVector<QVector<std::complex<qreal>> *> * templates)
{
    if (m_templates != templates) {
        deleteClouds(m_templates);
    }
    m_templates = templates;
}

QVector<QVector<std::complex<qreal>> *> * ProjectData::contour() const
{
    return m_contour;
}

void ProjectData::setContour(QVector<QVector<std::complex<qreal>> *> * contour)
{
    if (m_contour != contour) {
        deleteClouds(m_contour);
    }
    m_contour = contour;
    m_hasContour = true;
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

BoundaryData * ProjectData::boundaries() const
{
    return m_boundaries;
}

void ProjectData::setBoundaries(BoundaryData * boundaries)
{
    if (m_boundaries != boundaries) {
        delete m_boundaries;
    }
    m_boundaries = boundaries;
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

DatosLoopShaping * ProjectData::loopShaping() const
{
    return m_loopShaping;
}

void ProjectData::setLoopShaping(DatosLoopShaping * loopShaping)
{
    if (m_loopShaping != loopShaping) {
        delete m_loopShaping;
    }
    m_loopShaping = loopShaping;
}

} // namespace qftbx
