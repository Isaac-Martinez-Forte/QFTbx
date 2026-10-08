/**
 * @file
 * @brief Loads a .qft project file into the core's objects.
 *
 * Sections may appear in any subset; the result says which design steps the
 * file carried and whether the templates came with a contour. Malformed
 * content throws a parse error with the offending line, a missing or
 * unreadable file a file error, and any version other than the one this
 * build writes is refused rather than guessed at. load() first drops
 * whatever an earlier load left, so its result describes that file alone.
 * The reader owns what it loaded until the caller claims a section through
 * the take functions; whatever is left unclaimed dies with the reader. The
 * accessors only look and keep ownership: a section the file did not carry
 * is null or empty, and the epsilon's plane is the complex plane when the
 * file names none.
 */

#ifndef QFTBX_PROJECT_READER_H
#define QFTBX_PROJECT_READER_H

#include "src/core/pipeline/pipeline_step.h"
#include <optional>

#include "src/core/templates/cloud_set.h"
#include "src/core/templates/parameter_grids.h"
#include "src/core/templates/hull_metric.h"
#include <complex>

#include <string>
#include <vector>

#include "src/core/system/lti_system.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/loopshaping/loop_shaping_result.h"
#include "src/core/frequencies/omega.h"

namespace qftbx {

class ProjectReader
{
public:
    ProjectReader();

    struct Loaded {
        qftbx::StepSet steps;
        bool hasContour = false;
    };

    Loaded load(const std::string & filePath);

    ~ProjectReader();

    ProjectReader(const ProjectReader &) = delete;
    ProjectReader & operator=(const ProjectReader &) = delete;

    const std::string & name() const { return m_name; }
    const std::string & description() const { return m_description; }
    const std::string & doi() const { return m_doi; }
    LtiSystem * plant() const { return m_plant.get(); }
    const qftbx::SpecificationRecords * specifications() const
    {
        return m_specifications.has_value() ? &m_specifications.value() : nullptr;
    }
    Omega * omega() const { return m_omega.get(); }
    const CloudSet & templates() const { return m_templates; }
    const CloudSet & contour() const { return m_contour; }
    const std::vector <double> * epsilon() const
    {
        return m_epsilon.has_value() ? &m_epsilon.value() : nullptr;
    }
    EpsilonMetric epsilonMetric() const { return m_epsilonMetric; }
    const BoundaryData * boundaries() const
    {
        return m_boundaries.has_value() ? &m_boundaries.value() : nullptr;
    }
    LtiSystem * controller() const { return m_controller.get(); }
    LoopShapingResult * loopShaping() const { return m_loopShaping.get(); }

    std::unique_ptr<LtiSystem> takePlant() { return std::move(m_plant); }
    std::optional<qftbx::SpecificationRecords> takeSpecifications()
    {
        std::optional<qftbx::SpecificationRecords> taken = std::move(m_specifications);
        m_specifications.reset();

        return taken;
    }
    std::unique_ptr<Omega> takeOmega() { return std::move(m_omega); }
    CloudSet takeTemplates() { return std::move(m_templates); }
    CloudSet takeContour() { return std::move(m_contour); }
    ParameterGrids takeSweepGrids() { return std::move(m_sweepGrids); }
    std::optional<std::vector <double>> takeEpsilon()
    {
        std::optional<std::vector <double>> taken = std::move(m_epsilon);
        m_epsilon.reset();

        return taken;
    }
    std::optional<BoundaryData> takeBoundaries()
    {
        std::optional<BoundaryData> taken = std::move(m_boundaries);
        m_boundaries.reset();

        return taken;
    }
    std::unique_ptr<LtiSystem> takeController() { return std::move(m_controller); }
    std::unique_ptr<LoopShapingResult> takeLoopShaping() { return std::move(m_loopShaping); }

private:
    std::unique_ptr<LtiSystem> m_plant;
    std::optional<qftbx::SpecificationRecords> m_specifications;
    std::unique_ptr<Omega> m_omega;
    CloudSet m_templates;
    CloudSet m_contour;
    ParameterGrids m_sweepGrids;
    std::optional<std::vector <double>> m_epsilon;
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
