/**
 * @file
 * @brief Entry point of the GPU test binary: the program's GPU support,
 * installed as the programs install it.
 */

#include <gtest/gtest.h>

#include <clocale>

#include "src/app/gpu_support.h"

int main(int argc, char ** argv)
{
    std::setlocale(LC_NUMERIC, "C");

    qftbx::installGpuSupport();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
