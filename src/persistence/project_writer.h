/**
 * @file
 * @brief Writes a project to a .qft file.
 *
 * The content to write is a set of unowned pointers, one per section, and a
 * null pointer skips its section. Numbers are written in the shortest form
 * that reads back to the same double, so a save and load round trip is exact
 * to the bit. A value that is not a finite number refuses the write: the file
 * never carries a NaN or an infinity, except as the legitimate end of an
 * open boundary column.
 */

#ifndef QFTBX_PROJECT_WRITER_H
#define QFTBX_PROJECT_WRITER_H

#include "src/core/templates/cloud_set.h"
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

/**
 * @brief The sections of a project to be written; a null pointer skips the
 * section. None of the pointers is owned, and nothing is copied: a set of
 * templates by value is a copy of every cloud, to write it once.
 */
struct ProjectContent {
    LtiSystem * plant = nullptr;
    const qftbx::SpecificationRecords * specifications = nullptr;
    const Omega * omega = nullptr;
    const CloudSet * templates = nullptr;
    const CloudSet * contour = nullptr;
    const std::vector <double> * epsilon = nullptr;
    EpsilonMetric epsilonMetric;
    const BoundaryData * boundaries = nullptr;
    LtiSystem * controller = nullptr;
    LoopShapingResult * loopShaping = nullptr;
};

/**
 * @brief Writes a .qft project file in the dialect this build reads.
 *
 * Numbers are written in the shortest form that reads back to the same
 * double (qftbx::text::number), so a save/load round trip is bit-exact and
 * a fixed number of digits cannot silently degrade a stored result.
 * Throws qftbx::FileError when the file cannot be written, and
 * qftbx::InvalidInput when a value to write is not a finite number: the file
 * never carries a NaN or an infinity.
 */
class ProjectWriter
{
public:
    void save(const std::string & filePath, const ProjectContent & content);
};

}

#endif
