#ifndef QFTBX_PROJECT_READER_H
#define QFTBX_PROJECT_READER_H

#include <optional>

#include "src/core/templates/cloud_set.h"
#include <complex>

#include <QString>
#include <QVector>

#include "src/core/system/lti_system.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/loopshaping/loop_shaping_result.h"
#include "src/core/frequencies/omega.h"

namespace qftbx {

/**
 * @brief Loads a .qft project file (pugixml DOM).
 *
 * Reads the legacy dialect (Spanish tags, no version attribute) and, from
 * version 2 on, the English dialect. Sections may appear in any order and
 * any subset; load() reports which ones were found. Malformed content
 * throws qftbx::ParseError with the offending line; a missing or unreadable
 * file throws qftbx::FileError.
 *
 * Ownership is transitional, mirroring the old parser: the caller takes the
 * loaded objects through the section getters.
 */
class ProjectReader
{
public:
    ProjectReader();

    /**
     * @brief Parses the file and returns the section-presence flags, in the
     * historical order: plant, specifications, omega, templates, boundaries,
     * controller, loop shaping, template contour.
     */
    std::vector<bool> load(const QString & filePath);

    ~ProjectReader();

    ProjectReader(const ProjectReader &) = delete;
    ProjectReader & operator=(const ProjectReader &) = delete;

    //Inspection: the reader KEEPS ownership, so a caller that only looks
    //at the parsed contents needs no cleanup of its own.
    LtiSystem * plant() const { return m_plant.get(); }
    const qftbx::SpecificationRecords * specifications() const
    {
        return m_specifications.has_value() ? &m_specifications.value() : nullptr;
    }
    Omega * omega() const { return m_omega.get(); }
    const CloudSet & templates() const { return m_templates; }
    const CloudSet & contour() const { return m_contour; }
    /// The epsilon of the templates section, or nullptr when the file
    /// carried none. Held by value, like the boundaries.
    const QVector <double> * epsilon() const
    {
        return m_epsilon.has_value() ? &m_epsilon.value() : nullptr;
    }
    const BoundaryData * boundaries() const
    {
        return m_boundaries.has_value() ? &m_boundaries.value() : nullptr;
    }
    LtiSystem * controller() const { return m_controller.get(); }
    LoopShapingResult * loopShaping() const { return m_loopShaping.get(); }

    //Claim: the caller becomes the owner and the reader forgets it. Used
    //by the facade, which hands everything to the project store; anything
    //left unclaimed dies with the reader (it used to leak).
    std::unique_ptr<LtiSystem> takePlant() { return std::move(m_plant); }
    std::optional<qftbx::SpecificationRecords> takeSpecifications()
    {
        return std::move(m_specifications);
    }
    std::unique_ptr<Omega> takeOmega() { return std::move(m_omega); }
    /// By value, so "take" is now just a move: nothing to null out, nothing
    /// that could be freed twice.
    CloudSet takeTemplates() { return std::move(m_templates); }
    CloudSet takeContour() { return std::move(m_contour); }
    std::optional<QVector <double>> takeEpsilon() { return std::move(m_epsilon); }
    /// The boundaries, or nothing when the file carried none. By value, so
    /// there is no owner to hand over.
    std::optional<BoundaryData> takeBoundaries()
    {
        std::optional<BoundaryData> taken = std::move(m_boundaries);
        m_boundaries.reset();

        return taken;
    }
    std::unique_ptr<LtiSystem> takeController() { return std::move(m_controller); }
    std::unique_ptr<LoopShapingResult> takeLoopShaping() { return std::move(m_loopShaping); }

private:

    template <typename T>
    static T * take(T * & member)
    {
        T * claimed = member;
        member = nullptr;
        return claimed;
    }

    std::unique_ptr<LtiSystem> m_plant;
    std::optional<qftbx::SpecificationRecords> m_specifications;
    std::unique_ptr<Omega> m_omega;
    CloudSet m_templates;
    CloudSet m_contour;
    std::optional<QVector <double>> m_epsilon;
    std::optional<BoundaryData> m_boundaries;
    std::unique_ptr<LtiSystem> m_controller;
    std::unique_ptr<LoopShapingResult> m_loopShaping;
};

} // namespace qftbx

//Transitional: consumers still refer to the class unqualified.
using qftbx::ProjectReader;

#endif // QFTBX_PROJECT_READER_H
