/**
 * @file
 * @brief What the program finds on the machine's NVIDIA card, read once.
 *
 * probe() looks for the driver and the cards, takes the one with most
 * multiprocessors, reads what the engines size their work by and launches
 * one kernel, which is how a build without code for the card finds out. It
 * runs once, from whichever thread asks first. CUDA keeps the current
 * device per thread, so every thread that computes on the card calls
 * useProbedDevice() first. Versions are CUDA's, 12040 for 12.4.
 */

#ifndef QFTBX_GPU_DEVICE_PROBE_H
#define QFTBX_GPU_DEVICE_PROBE_H

#include <cstddef>
#include <string>

namespace qftbx::gpu {

struct DeviceProbe
{
    enum class Outcome { Usable, NoDriver, NoDevice, OldDriver, NoCodeForCard, Failed };

    Outcome outcome = Outcome::NoDevice;
    int device = -1;
    std::string name;
    int computeMajor = 0;
    int computeMinor = 0;
    std::size_t memory = 0;
    int multiprocessors = 0;
    int doublePrecisionRatio = 0;
    bool memoryPools = false;
    int driverVersion = 0;
    int runtimeVersion = 0;
    std::string detail;
};

const DeviceProbe & probe();

void useProbedDevice();

}

#endif
