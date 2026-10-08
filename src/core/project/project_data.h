/**
 * @file
 * @brief Owning store of everything a QFT project holds.
 *
 * Declares the container for the plant, the design frequencies, the
 * specifications, the templates with their contours and epsilon, the
 * boundaries, the controller search box and the loop-shaping result. Every
 * member owns what it holds, so a setter frees what it replaces and the
 * accessors hand out observers; the type a setter takes says whether it
 * takes ownership. The store is movable and not copyable, since opening a
 * file replaces the whole project. Whether a contour exists follows the
 * contour itself: publishing an empty one clears it, and so does dropping
 * the templates.
 *
 * The specifications, the epsilon and the boundaries are held by value in
 * an optional, and their accessors return null when none were set. The
 * sweep grids, by parameter name, are what the verifier walks to check the
 * family member by member; they are empty when the templates came from a
 * file that did not record them. The epsilon is measured in the complex
 * plane unless the project names another; the name, the description and
 * the DOI are empty unless the file carried them.
 */

#ifndef QFTBX_PROJECT_DATA_H
#define QFTBX_PROJECT_DATA_H

#include <memory>
#include <string>
#include <optional>

#include "src/core/templates/cloud_set.h"
#include "src/core/templates/parameter_grids.h"
#include "src/core/templates/hull_metric.h"
#include <complex>

#include <vector>

#include "src/core/system/lti_system.h"
#include "src/core/frequencies/omega.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/loop_shaping_result.h"

namespace qftbx {

class ProjectData
{
public:
    ProjectData() = default;

    ProjectData(const ProjectData &) = delete;
    ProjectData & operator=(const ProjectData &) = delete;

    ProjectData(ProjectData &&) = default;
    ProjectData & operator=(ProjectData &&) = default;

    LtiSystem * plant() const;
    void setPlant(std::unique_ptr<LtiSystem> plant);

    Omega * omega() const;
    void setOmega(std::unique_ptr<Omega> omega);
    std::vector<double> * frequencies() const;

    SpecificationRecords * specifications();
    const SpecificationRecords * specifications() const;
    void setSpecifications(std::optional<SpecificationRecords> specifications);

    const CloudSet & templates() const;
    void setTemplates(CloudSet templates);

    const CloudSet & contour() const;
    void setContour(CloudSet contour);

    const ParameterGrids & sweepGrids() const { return m_sweepGrids; }
    void setSweepGrids(ParameterGrids grids) { m_sweepGrids = std::move(grids); }

    bool hasContour() const;

    std::vector<double> * epsilon();
    void setEpsilon(std::optional<std::vector<double>> epsilon);
    EpsilonMetric epsilonMetric() const { return m_epsilonMetric; }
    void setEpsilonMetric(EpsilonMetric metric) { m_epsilonMetric = metric; }

    const std::string & name() const { return m_name; }
    void setName(std::string name) { m_name = std::move(name); }
    const std::string & description() const { return m_description; }
    void setDescription(std::string description) { m_description = std::move(description); }
    const std::string & doi() const { return m_doi; }
    void setDoi(std::string doi) { m_doi = std::move(doi); }

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
    ParameterGrids m_sweepGrids;
    bool m_hasContour = false;
    std::optional<std::vector<double>> m_epsilon;
    std::string m_name;
    std::string m_description;
    std::string m_doi;
    EpsilonMetric m_epsilonMetric;
    std::optional<BoundaryData> m_boundaries;
    std::unique_ptr<LtiSystem> m_controller;
    std::unique_ptr<LoopShapingResult> m_loopShaping;
};

}

#endif
