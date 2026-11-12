/**
 * @file
 * @brief The clouds prepared for the walk on the card.
 *
 * preparedOnCard() takes the clouds the sweep left on the card and
 * prepares each for the walk at its epsilon, as walk_cloud.h does on the
 * processor: in MATLAB's complex order without repeats, projected onto the
 * plane of the walk, and split into its epsilon-connected parts, rightmost
 * first. It goes as many frequencies at a time as half the card's free
 * memory holds, stops between batches when the computation is cancelled,
 * and brings back what the walk on the processor reads. cloudsOnCard()
 * puts clouds that came from elsewhere, a file, on the card, when they all
 * have the same number of points and fit in half its free memory, and is
 * null otherwise. Plain C++, so the glue includes it without the toolkit;
 * a failure on the card throws CudaError. preparedOnProcessor() runs the
 * threads' own code on the processor, which is how the tests check it
 * without a card.
 */

#ifndef QFTBX_GPU_WALK_PREPARATION_H
#define QFTBX_GPU_WALK_PREPARATION_H

#include <cstddef>
#include <memory>
#include <vector>

#include "src/core/pipeline/cancellation.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/templates/hull_metric.h"
#include "src/core/templates/walk_cloud.h"

namespace qftbx::gpu {

class DeviceClouds;

struct PreparedOnCard
{
    std::vector<PreparedCloud> clouds;
    bool cancelled = false;
    double kernelMilliseconds = 0.0;
    double copyMilliseconds = 0.0;
    std::size_t batches = 0;
};

std::shared_ptr<const DeviceClouds> cloudsOnCard(const CloudSet & clouds);

PreparedOnCard preparedOnCard(const DeviceClouds & clouds, const std::vector<double> & epsilon, HullMetric metric,
                              double dbPerDegree, const CancellationToken * cancellation);

std::vector<PreparedCloud> preparedOnProcessor(const CloudSet & clouds, const std::vector<double> & epsilon,
                                               HullMetric metric, double dbPerDegree);

}

#endif
