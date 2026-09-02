#ifndef QFTBX_PROJECT_WRITER_H
#define QFTBX_PROJECT_WRITER_H

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
 * @brief The sections of a project to be written; a null pointer skips the
 * section. None of the pointers is owned.
 */
struct ProjectContent {
    LtiSystem * plant = nullptr;
    const qftbx::SpecificationRecords * specifications = nullptr;
    Omega * omega = nullptr;
    CloudSet templates;
    CloudSet contour;
    const QVector <double> * epsilon = nullptr;
    BoundaryData * boundaries = nullptr;
    LtiSystem * controller = nullptr;
    LoopShapingResult * loopShaping = nullptr;
};

/**
 * @brief Writes a .qft project file in the version-2 English dialect.
 *
 * Numbers are written with 17 significant digits, so a save/load round trip
 * is bit-exact (the historical writer kept 6 digits and silently degraded
 * every stored result). Throws qftbx::FileError when the file cannot be
 * written.
 */
class ProjectWriter
{
public:
    void save(const QString & filePath, const ProjectContent & content);
};

} // namespace qftbx

//Transitional: consumers still refer to the class unqualified.
using qftbx::ProjectWriter;
using qftbx::ProjectContent;

#endif // QFTBX_PROJECT_WRITER_H
