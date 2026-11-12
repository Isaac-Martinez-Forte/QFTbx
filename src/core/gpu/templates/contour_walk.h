/**
 * @file
 * @brief The contours walked on the card.
 *
 * walkedOnCard() prepares the clouds the card holds, as preparedOnCard()
 * does, and walks every part of every frequency there: the epsilon-hull
 * walk of the template engine from the part's rightmost point and, when it
 * does not close within three steps a point, the relaxed walk from its
 * highest, over the part, or over the cloud as swept when the part is the
 * whole cloud. Only the contours come back, with what the engine needs to
 * finish them (contourWalked): whether a part fell back or ran out of
 * steps and where each starts. Plain C++, so the glue includes it without
 * the toolkit; a failure on the card throws CudaError. walkedOnProcessor()
 * runs the threads' own code on the processor, which is how the tests
 * check it without a card.
 */

#ifndef QFTBX_GPU_CONTOUR_WALK_H
#define QFTBX_GPU_CONTOUR_WALK_H

#include <cstddef>
#include <vector>

#include "src/core/pipeline/cancellation.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/templates/hull_metric.h"
#include "src/core/templates/walk_cloud.h"

namespace qftbx::gpu {

class DeviceClouds;

struct WalkedOnCard
{
    std::vector<WalkedContour> walks;
    bool cancelled = false;
    double kernelMilliseconds = 0.0;
    double copyMilliseconds = 0.0;
    std::size_t batches = 0;
};

WalkedOnCard walkedOnCard(const DeviceClouds & clouds, const std::vector<double> & epsilon, HullMetric metric,
                          double dbPerDegree, const CancellationToken * cancellation);

std::vector<WalkedContour> walkedOnProcessor(const CloudSet & clouds, const std::vector<double> & epsilon,
                                             HullMetric metric, double dbPerDegree);

}

#endif
