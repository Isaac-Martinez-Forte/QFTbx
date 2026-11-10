/**
 * @file
 * @brief The clouds of a sweep, kept on the card for the phases after it.
 *
 * One buffer, frequency after frequency, each the same number of plants,
 * with the stream it was reserved in, which lives as long as it does.
 */

#ifndef QFTBX_GPU_DEVICE_CLOUDS_CUH
#define QFTBX_GPU_DEVICE_CLOUDS_CUH

#include <cstddef>
#include <memory>

#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/stream.cuh"
#include "src/core/gpu/templates/plant_evaluation.cuh"

namespace qftbx::gpu {

class DeviceClouds
{
public:
    DeviceClouds(std::shared_ptr<Stream> stream, DeviceBuffer<DeviceComplex> values,
                 std::size_t frequencies, std::size_t plants)
        : m_stream(std::move(stream)), m_values(std::move(values)), m_frequencies(frequencies), m_plants(plants)
    {
    }

    const Stream & stream() const { return *m_stream; }
    const DeviceComplex * values() const { return m_values.data(); }
    std::size_t frequencies() const { return m_frequencies; }
    std::size_t plants() const { return m_plants; }

private:
    std::shared_ptr<Stream> m_stream;
    DeviceBuffer<DeviceComplex> m_values;
    std::size_t m_frequencies = 0;
    std::size_t m_plants = 0;
};

}

#endif
