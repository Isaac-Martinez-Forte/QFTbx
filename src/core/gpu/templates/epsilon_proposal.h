/**
 * @file
 * @brief The epsilon proposal on the card.
 *
 * proposedOnCard() proposes for every cloud the card holds what
 * TemplateEngine::proposeEpsilon() does: the longest edge of the minimum
 * spanning tree and the diameter, a block of the card per frequency, and
 * the epsilons of epsilon_climb.h walked on the card in rounds until one
 * closes without passing a point twice. Plain C++, so the glue includes it
 * without the toolkit; a failure on the card throws CudaError.
 * proposedOnProcessor() runs the threads' own code on the processor, which
 * is how the tests check it without a card.
 */

#ifndef QFTBX_GPU_EPSILON_PROPOSAL_H
#define QFTBX_GPU_EPSILON_PROPOSAL_H

#include <cstddef>
#include <vector>

#include "src/core/pipeline/cancellation.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/templates/hull_metric.h"
#include "src/core/templates/template_computation.h"

namespace qftbx::gpu {

class DeviceClouds;

struct ProposedOnCard
{
    std::vector<EpsilonProposal> proposals;
    bool cancelled = false;
    double kernelMilliseconds = 0.0;
    double copyMilliseconds = 0.0;
    std::size_t batches = 0;
    std::size_t walks = 0;
    std::size_t rounds = 0;
};

ProposedOnCard proposedOnCard(const DeviceClouds & clouds, HullMetric metric, double dbPerDegree, bool walked,
                              const CancellationToken * cancellation);

std::vector<EpsilonProposal> proposedOnProcessor(const CloudSet & clouds, HullMetric metric, double dbPerDegree,
                                                 bool walked);

}

#endif
