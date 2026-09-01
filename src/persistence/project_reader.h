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
    LtiSystem * plant() const { return m_plant; }
    QVector <qftbx::SpecificationRecord *> * specifications() const { return m_specifications; }
    Omega * omega() const { return m_omega; }
    const CloudSet & templates() const { return m_templates; }
    const CloudSet & contour() const { return m_contour; }
    QVector <qreal> * epsilon() const { return m_epsilon; }
    const BoundaryData * boundaries() const
    {
        return m_boundaries.has_value() ? &m_boundaries.value() : nullptr;
    }
    LtiSystem * controller() const { return m_controller; }
    LoopShapingResult * loopShaping() const { return m_loopShaping; }

    //Claim: the caller becomes the owner and the reader forgets it. Used
    //by the facade, which hands everything to the project store; anything
    //left unclaimed dies with the reader (it used to leak).
    LtiSystem * takePlant() { return take(m_plant); }
    QVector <qftbx::SpecificationRecord *> * takeSpecifications() { return take(m_specifications); }
    Omega * takeOmega() { return take(m_omega); }
    /// By value, so "take" is now just a move: nothing to null out, nothing
    /// that could be freed twice.
    CloudSet takeTemplates() { return std::move(m_templates); }
    CloudSet takeContour() { return std::move(m_contour); }
    QVector <qreal> * takeEpsilon() { return take(m_epsilon); }
    /// The boundaries, or nothing when the file carried none. By value, so
    /// there is no owner to hand over.
    std::optional<BoundaryData> takeBoundaries()
    {
        std::optional<BoundaryData> taken = std::move(m_boundaries);
        m_boundaries.reset();

        return taken;
    }
    LtiSystem * takeController() { return take(m_controller); }
    LoopShapingResult * takeLoopShaping() { return take(m_loopShaping); }

private:

    template <typename T>
    static T * take(T * & member)
    {
        T * claimed = member;
        member = nullptr;
        return claimed;
    }

    LtiSystem * m_plant = nullptr;
    QVector <qftbx::SpecificationRecord *> * m_specifications = nullptr;
    Omega * m_omega = nullptr;
    CloudSet m_templates;
    CloudSet m_contour;
    QVector <qreal> * m_epsilon = nullptr;
    std::optional<BoundaryData> m_boundaries;
    LtiSystem * m_controller = nullptr;
    LoopShapingResult * m_loopShaping = nullptr;
};

} // namespace qftbx

//Transitional: consumers still refer to the class unqualified.
using qftbx::ProjectReader;

#endif // QFTBX_PROJECT_READER_H
