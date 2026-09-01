#ifndef QFTBX_PROJECT_DATA_H
#define QFTBX_PROJECT_DATA_H

#include <optional>

#include "src/core/templates/cloud_set.h"
#include <complex>

#include <QVector>

#include "src/core/system/lti_system.h"
#include "src/core/frequencies/omega.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/loop_shaping_result.h"



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

    const CloudSet & templates() const;
    void setTemplates(CloudSet templates);

    const CloudSet & contour() const;
    void setContour(CloudSet contour);

    /// Whether a contour was ever computed. Not the same as "the contour is
    /// non-empty": the epsilon-hull of an empty template set is empty too.
    bool hasContour() const;

    QVector<qreal> * epsilon() const;
    void setEpsilon(QVector<qreal> * epsilon);

    /// The boundaries, or nullptr when none have been computed. The store
    /// holds them BY VALUE in an optional; the pointer is only how callers
    /// ask "are there any", which is what they already did.
    BoundaryData * boundaries();
    const BoundaryData * boundaries() const;
    void setBoundaries(std::optional<BoundaryData> boundaries);

    LtiSystem * controller() const;
    void setController(LtiSystem * controller);

    LoopShapingResult * loopShaping() const;
    void setLoopShapingResult(LoopShapingResult * loopShaping);

private:
    LtiSystem * m_plant = nullptr;
    Omega * m_omega = nullptr;
    QVector<SpecificationRecord *> * m_specifications = nullptr;
    CloudSet m_templates;
    CloudSet m_contour;
    bool m_hasContour = false;
    QVector<qreal> * m_epsilon = nullptr;
    std::optional<BoundaryData> m_boundaries;
    LtiSystem * m_controller = nullptr;
    LoopShapingResult * m_loopShaping = nullptr;
};

} // namespace qftbx

#endif // QFTBX_PROJECT_DATA_H
