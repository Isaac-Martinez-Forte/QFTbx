#ifndef QFTBX_PROJECT_DATA_H
#define QFTBX_PROJECT_DATA_H

#include <memory>
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
 * rule, which is now the language's: every member owns what it holds, so
 * a setter frees what it replaces and the store frees the rest when it
 * dies. What a setter takes says whether it takes ownership, and the
 * accessors hand out observers.
 */
class ProjectData
{
public:
    ProjectData() = default;

    //Still hand-written for ONE member: the specifications are a pointer to
    //a QVector of pointers, and consolidating that two-level container is
    //the next step.
    ~ProjectData();

    ProjectData(const ProjectData &) = delete;
    ProjectData & operator=(const ProjectData &) = delete;

    LtiSystem * plant() const;
    void setPlant(std::unique_ptr<LtiSystem> plant);

    Omega * omega() const;
    void setOmega(std::unique_ptr<Omega> omega);
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

    /// The epsilon used for the contours, or nullptr when none was ever
    /// set. Held BY VALUE in an optional, like the boundaries.
    QVector<qreal> * epsilon();
    void setEpsilon(std::optional<QVector<qreal>> epsilon);

    /// The boundaries, or nullptr when none have been computed. The store
    /// holds them BY VALUE in an optional; the pointer is only how callers
    /// ask "are there any", which is what they already did.
    BoundaryData * boundaries();
    const BoundaryData * boundaries() const;
    void setBoundaries(std::optional<BoundaryData> boundaries);

    LtiSystem * controller() const;
    void setController(std::unique_ptr<LtiSystem> controller);

    LoopShapingResult * loopShaping() const;
    void setLoopShapingResult(std::unique_ptr<LoopShapingResult> loopShaping);

private:
    std::unique_ptr<LtiSystem> m_plant;
    std::unique_ptr<Omega> m_omega;
    QVector<SpecificationRecord *> * m_specifications = nullptr;
    CloudSet m_templates;
    CloudSet m_contour;
    bool m_hasContour = false;
    std::optional<QVector<qreal>> m_epsilon;
    std::optional<BoundaryData> m_boundaries;
    std::unique_ptr<LtiSystem> m_controller;
    std::unique_ptr<LoopShapingResult> m_loopShaping;
};

} // namespace qftbx

#endif // QFTBX_PROJECT_DATA_H
