#ifndef QFTBX_PROJECT_DATA_H
#define QFTBX_PROJECT_DATA_H

#include <complex>

#include <QVector>

#include "src/core/system/lti_system.h"
#include "src/core/frequencies/omega.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/boundaries/boundary_data.h"
#include "Modelo/EstructurasDatos/datosloopshaping.h"

namespace qftbx {

/**
 * @brief Owning store of everything a QFT project holds: the plant, the
 * design frequencies, the specifications, the templates (clouds, contours
 * and their epsilon), the boundaries, the controller search box and the
 * loop-shaping result.
 *
 * It replaces the historical DAO layer (seven interface/adapter pairs and
 * a factory whose polymorphism was never used) with one class and one
 * rule: every setter deletes what it replaces, and the store deletes
 * whatever it still holds on destruction. Setters tolerate being handed
 * the pointer they already own (the pipeline round-trips containers).
 *
 * Known transitional exception: BoundaryData is a non-owning VIEW over
 * containers whose ownership is still being consolidated (phase 9.4), so
 * replacing it deletes the view only, as the adapter it replaces did.
 */
class ProjectData
{
public:
    ProjectData() = default;
    ~ProjectData();

    ProjectData(const ProjectData &) = delete;
    ProjectData & operator=(const ProjectData &) = delete;

    LtiSystem * plant() const;
    void setPlant(LtiSystem * plant);

    Omega * omega() const;
    void setOmega(Omega * omega);
    QVector<qreal> * frequencies() const;

    QVector<SpecificationRecord *> * specifications() const;
    void setSpecifications(QVector<SpecificationRecord *> * specifications);

    QVector<QVector<std::complex<qreal>> *> * templates() const;
    void setTemplates(QVector<QVector<std::complex<qreal>> *> * templates);

    QVector<QVector<std::complex<qreal>> *> * contour() const;
    void setContour(QVector<QVector<std::complex<qreal>> *> * contour);
    bool hasContour() const;

    QVector<qreal> * epsilon() const;
    void setEpsilon(QVector<qreal> * epsilon);

    BoundaryData * boundaries() const;
    void setBoundaries(BoundaryData * boundaries);

    LtiSystem * controller() const;
    void setController(LtiSystem * controller);

    DatosLoopShaping * loopShaping() const;
    void setLoopShaping(DatosLoopShaping * loopShaping);

private:
    LtiSystem * m_plant = nullptr;
    Omega * m_omega = nullptr;
    QVector<SpecificationRecord *> * m_specifications = nullptr;
    QVector<QVector<std::complex<qreal>> *> * m_templates = nullptr;
    QVector<QVector<std::complex<qreal>> *> * m_contour = nullptr;
    bool m_hasContour = false;
    QVector<qreal> * m_epsilon = nullptr;
    BoundaryData * m_boundaries = nullptr;
    LtiSystem * m_controller = nullptr;
    DatosLoopShaping * m_loopShaping = nullptr;
};

} // namespace qftbx

#endif // QFTBX_PROJECT_DATA_H
