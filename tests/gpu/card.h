/**
 * @file
 * @brief What the GPU tests need to know about the card: whether there is
 * one to run on, and why not.
 */

#ifndef QFTBX_TESTS_GPU_CARD_H
#define QFTBX_TESTS_GPU_CARD_H

#include <string>

#include "src/core/pipeline/gpu_provider.h"

namespace qftbx::gputest {

inline bool cardUsable()
{
    return gpuProvider() != nullptr && gpuProvider()->status().usable();
}

inline std::string whyNoCard()
{
    return gpuProvider() == nullptr ? std::string("no GPU support") : gpuProvider()->status().reason().rendered();
}

}

#define QFTBX_REQUIRE_CARD() \
    if (!::qftbx::gputest::cardUsable()) { \
        GTEST_SKIP() << ::qftbx::gputest::whyNoCard(); \
    }

#endif
