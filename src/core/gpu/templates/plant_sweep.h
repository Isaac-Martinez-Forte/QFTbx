/**
 * @file
 * @brief The sweep of the plant family on the card.
 *
 * sweepOnCard() evaluates every combination of the grid at every
 * frequency, one thread per plant, and brings the clouds back. It uploads
 * the plant and the grid once and sweeps as many frequencies at a time as
 * half the card's free memory holds; when they all fit, the clouds stay on
 * the card for the phases after it (DeviceClouds), and otherwise they are
 * only on the processor. It marks the frequencies with a plant that is not
 * finite and stops between batches when the computation is cancelled.
 * Plain C++, so the glue includes it without the toolkit; a failure on the
 * card throws CudaError. plantOnProcessor() and digitsOnProcessor() run the
 * threads' own code on the processor, which is how the tests check it
 * without a card.
 */

#ifndef QFTBX_GPU_PLANT_SWEEP_H
#define QFTBX_GPU_PLANT_SWEEP_H

#include <complex>
#include <memory>
#include <vector>

#include "src/core/gpu/templates/plant_model.h"
#include "src/core/pipeline/cancellation.h"

namespace qftbx::gpu {

class DeviceClouds;

struct SweepOnCard
{
    std::vector<std::vector<std::complex<double>>> clouds;
    std::vector<char> nonFinite;
    bool cancelled = false;
    std::shared_ptr<const DeviceClouds> resident;
    double kernelMilliseconds = 0.0;
    double copyMilliseconds = 0.0;
    std::size_t batches = 0;
};

SweepOnCard sweepOnCard(const PlantModel & plant, const SweepGrid & grid, const std::vector<double> & frequencies,
                        const CancellationToken * cancellation);

std::complex<double> plantOnProcessor(const PlantModel & plant, const std::vector<double> & digits, double w);

std::vector<double> digitsOnProcessor(const SweepGrid & grid, std::size_t combination);

}

#endif
