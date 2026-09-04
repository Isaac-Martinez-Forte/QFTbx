#ifndef QFTBX_PROJECT_DATA_H
#define QFTBX_PROJECT_DATA_H

#include <memory>
#include <optional>

#include "src/core/templates/cloud_set.h"
#include <complex>

#include <vector>

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

    ProjectData(const ProjectData &) = delete;
    ProjectData & operator=(const ProjectData &) = delete;

    //Movable: opening a file REPLACES the project, and "start from an empty
    //one" is a move-assignment. Spelled out because deleting the copy
    //operations above silences the implicit move as well.
    ProjectData(ProjectData &&) = default;
    ProjectData & operator=(ProjectData &&) = default;

    LtiSystem * plant() const;
    void setPlant(std::unique_ptr<LtiSystem> plant);

    Omega * omega() const;
    void setOmega(std::unique_ptr<Omega> omega);
    std::vector<double> * frequencies() const;

    /// The seven specification slots, or nullptr when none were ever set.
    /// Held BY VALUE in an optional, like the boundaries and the epsilon.
    SpecificationRecords * specifications();
    const SpecificationRecords * specifications() const;
    void setSpecifications(std::optional<SpecificationRecords> specifications);

    const CloudSet & templates() const;
    void setTemplates(CloudSet templates);

    const CloudSet & contour() const;
    void setContour(CloudSet contour);

    /// Whether there is a contour to save or to walk: set when a non-empty
    /// one is published, cleared when the templates are dropped. (An earlier
    /// comment here claimed it meant "ever computed, even if empty"; the code
    /// never did that, and the drop relies on it not doing so.)
    bool hasContour() const;

    /// The epsilon used for the contours, or nullptr when none was ever
    /// set. Held BY VALUE in an optional, like the boundaries.
    std::vector<double> * epsilon();
    void setEpsilon(std::optional<std::vector<double>> epsilon);

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
    std::optional<SpecificationRecords> m_specifications;
    CloudSet m_templates;
    CloudSet m_contour;
    bool m_hasContour = false;
    std::optional<std::vector<double>> m_epsilon;
    std::optional<BoundaryData> m_boundaries;
    std::unique_ptr<LtiSystem> m_controller;
    std::unique_ptr<LoopShapingResult> m_loopShaping;
};

} // namespace qftbx

#endif // QFTBX_PROJECT_DATA_H
