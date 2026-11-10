/**
 * @file
 * @brief Small computations on the card that exercise the GPU base: the
 * memory, the streams, the pool and the flag a kernel stops on.
 *
 * Plain C++, so the tests are ordinary gtest files; the kernels are in
 * test_kernels.cu.
 */

#ifndef QFTBX_TESTS_GPU_TEST_KERNELS_H
#define QFTBX_TESTS_GPU_TEST_KERNELS_H

#include <cstddef>
#include <vector>

#include "src/core/pipeline/cancellation.h"

namespace qftbx::gputest {

std::vector<double> doubledOnTheCard(const std::vector<double> & values);

bool reservationsFollowEachOther(std::size_t count, std::size_t elements);

struct SpinOutcome
{
    bool finished = false;
    bool stoppedOnTheFlag = false;
};

SpinOutcome spinUntilCancelled(CancellationToken & token);

}

#endif
