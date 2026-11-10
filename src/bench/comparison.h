/**
 * @file
 * @brief How far apart the phases come out on the processor and on the GPU.
 *
 * A comparison case computes the phases on both and gathers what each left
 * (PhaseOutputs). comparePhases() counts what differs, frequency by
 * frequency:
 * - the clouds, by size and by the largest distance between plants of the
 *   same index relative to the plant;
 * - the contours, as sets of points, by the points in one and not the
 *   other and by the Hausdorff distance over the frequency's epsilon;
 * - the contour reports and the epsilons;
 * - the columns of every specification, by the columns whose intervals
 *   differ in number and by the largest gap between finite ends that
 *   match, in dB;
 * - the traces of every specification and the union, by those whose
 *   shape differs (how many traces, how many points each) and by the
 *   largest gap between points that match, in degrees or dB.
 *
 * A distance that was never measured stays not-a-number. The search, when
 * the case runs one, is compared by the caller.
 */

#ifndef QFTBX_BENCH_COMPARISON_H
#define QFTBX_BENCH_COMPARISON_H

#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/templates/template_computation.h"

namespace qftbx::bench {

struct PhaseOutputs
{
    CloudSet clouds;
    CloudSet contours;
    std::vector<double> epsilon;
    std::vector<ContourReport> reports;
    std::optional<BoundaryData> boundaries;
};

struct Comparison
{
    bool made = false;
    std::size_t frequencies = 0;

    std::size_t cloudSizesDiffer = 0;
    double cloudLargestRelative = std::numeric_limits<double>::quiet_NaN();
    std::size_t contourPointsDiffer = 0;
    double contourLargestHausdorff = std::numeric_limits<double>::quiet_NaN();
    std::size_t reportsDiffer = 0;
    std::size_t epsilonsDiffer = 0;

    std::size_t columnsDiffer = 0;
    double columnLargestDb = std::numeric_limits<double>::quiet_NaN();
    std::size_t traceSetsDiffer = 0;
    double traceLargest = std::numeric_limits<double>::quiet_NaN();
    std::size_t unionsDiffer = 0;
    double unionLargest = std::numeric_limits<double>::quiet_NaN();

    std::string cpuStatus;
    std::string gpuStatus;
    double gainRelative = std::numeric_limits<double>::quiet_NaN();
    bool sameDesign = false;
};

void comparePhases(const PhaseOutputs & cpu, const PhaseOutputs & gpu, Comparison & into);

}

#endif
