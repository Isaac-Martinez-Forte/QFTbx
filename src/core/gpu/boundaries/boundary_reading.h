/**
 * @file
 * @brief The boundaries read off the sheets where they are, on the card.
 *
 * boundariesOnCard() computes the sheets of every frequency (sheet_sweep.h)
 * and reads them on the card, so that only what is read comes back. Each
 * reading asked of a frequency cuts one of its sheets at a height and gives
 * the columns of allowed magnitudes, the traces of the cut as nodes of the
 * grid, and for each trace whether the side below it is allowed, as
 * BoundaryEngine reads its own sheets (sheet_reading.cuh). It computes as
 * many frequencies at a time as half the card's free memory holds; a
 * failure on the card throws CudaError, and a cancelled computation stops
 * and answers cancelled. readingsOnProcessor() reads sheets given to it with
 * the threads' own code on the processor, which is how the tests check it
 * without a card.
 */

#ifndef QFTBX_GPU_BOUNDARY_READING_H
#define QFTBX_GPU_BOUNDARY_READING_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "src/core/boundaries/boundary_types.h"
#include "src/core/gpu/boundaries/sheet_sweep.h"
#include "src/core/math/range.h"
#include "src/core/pipeline/cancellation.h"

namespace qftbx::gpu {

struct ReadingRequest
{
    std::size_t sheet = 0;
    double heightDb = 0.0;
};

struct ReadingOnCard
{
    std::vector<std::int32_t> traceStart;
    std::vector<std::int32_t> x;
    std::vector<std::int32_t> y;
    std::vector<char> allowedBelow;
    std::vector<std::int32_t> columnBegin;
    std::vector<double> lo;
    std::vector<double> hi;
};

struct BoundariesOnCard
{
    std::vector<std::vector<ReadingOnCard>> readings;
    bool cancelled = false;
    double kernelMilliseconds = 0.0;
    double copyMilliseconds = 0.0;
    std::size_t batches = 0;
};

BoundariesOnCard boundariesOnCard(const SheetProblem & problem,
                                  const std::vector<std::vector<ReadingRequest>> & requests,
                                  Range phaseRange, Range magnitudeRange, const CancellationToken * cancellation);

std::vector<std::vector<ReadingOnCard>> readingsOnProcessor(const SheetProblem & problem,
                                                            const std::vector<BoundarySheets> & sheets,
                                                            const std::vector<std::vector<ReadingRequest>> & requests,
                                                            Range phaseRange, Range magnitudeRange);

}

#endif
