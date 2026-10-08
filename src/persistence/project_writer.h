/**
 * @file
 * @brief Writes a project to a .qft file.
 *
 * The content to write is a set of unowned pointers, one per section, and a
 * null pointer skips its section; nothing is copied, since a set of
 * templates by value would copy every cloud to write it once. The file is
 * in the dialect this build reads. Numbers are written in the shortest form
 * that reads back to the same double, so a save and load round trip is exact
 * to the bit. A value that is not a finite number refuses the write with
 * qftbx::InvalidInput: the file never carries a NaN or an infinity, except
 * as the legitimate end of an open boundary column. A file that cannot be
 * written throws qftbx::FileError.
 */

#ifndef QFTBX_PROJECT_WRITER_H
#define QFTBX_PROJECT_WRITER_H

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

struct ProjectContent {
    std::string name;
    std::string description;
    std::string doi;
    LtiSystem * plant = nullptr;
    const qftbx::SpecificationRecords * specifications = nullptr;
    const Omega * omega = nullptr;
    const CloudSet * templates = nullptr;
    const CloudSet * contour = nullptr;
    const ParameterGrids * sweepGrids = nullptr;
    const std::vector <double> * epsilon = nullptr;
    EpsilonMetric epsilonMetric;
    const BoundaryData * boundaries = nullptr;
    LtiSystem * controller = nullptr;
    LoopShapingResult * loopShaping = nullptr;
};

class ProjectWriter
{
public:
    void save(const std::string & filePath, const ProjectContent & content);
};

}

#endif
