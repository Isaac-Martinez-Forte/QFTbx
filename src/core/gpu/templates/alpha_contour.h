/**
 * @file
 * @brief The alpha-shape contours on the card.
 *
 * alphaOnCard() prepares the clouds the card holds, as preparedOnCard()
 * does, and finds there the boundary edges of each, its components and the
 * face of largest area of each component, as alphaShape() does. Those
 * faces come back with the points that have no neighbour within epsilon,
 * and the processor finishes them into contours with finishedLoops() and
 * loopsContour(): what the template engine hands back for the alpha-shape
 * and for the border sweep, whose epsilon it gives. Plain C++, so the glue
 * includes it without the toolkit; a failure on the card throws CudaError.
 * alphaOnProcessor() runs the threads' own code on the processor, which is
 * how the tests check it without a card.
 */

#ifndef QFTBX_GPU_ALPHA_CONTOUR_H
#define QFTBX_GPU_ALPHA_CONTOUR_H

#include <cstddef>
#include <vector>

#include "src/core/pipeline/cancellation.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/templates/hull_metric.h"
#include "src/core/templates/walk_cloud.h"

namespace qftbx::gpu {

class DeviceClouds;

struct AlphaOnCard
{
    std::vector<WalkedContour> contours;
    bool cancelled = false;
    double kernelMilliseconds = 0.0;
    double copyMilliseconds = 0.0;
    std::size_t batches = 0;
};

AlphaOnCard alphaOnCard(const DeviceClouds & clouds, const std::vector<double> & epsilon, HullMetric metric,
                        double dbPerDegree, const CancellationToken * cancellation);

std::vector<WalkedContour> alphaOnProcessor(const CloudSet & clouds, const std::vector<double> & epsilon,
                                            HullMetric metric, double dbPerDegree);

}

#endif
