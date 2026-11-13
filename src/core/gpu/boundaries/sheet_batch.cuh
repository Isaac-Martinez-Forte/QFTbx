/**
 * @file
 * @brief A batch of frequencies whose sheets are computed on the card.
 *
 * SheetRun holds what every batch shares: the stream, the flag the kernels
 * read to stop, the loop values of the grid and the guard. batchesOf() cuts
 * the frequencies into batches that half the card's free memory holds,
 * counting the bytes a reader of the sheets adds to each frequency. A
 * SheetBatch uploads the templates of its frequencies, unless they are the
 * whole clouds the card already holds, and launches the kernels of the
 * sheets in the run's stream without waiting for them; the sheets and
 * everything they were computed from stay on the card for whoever reads
 * them next.
 */

#ifndef QFTBX_GPU_SHEET_BATCH_CUH
#define QFTBX_GPU_SHEET_BATCH_CUH

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "src/core/gpu/boundaries/sheet_bounds.cuh"
#include "src/core/gpu/boundaries/sheet_sweep.h"
#include "src/core/gpu/common/cancel_flag.cuh"
#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/stream.cuh"

namespace qftbx::gpu {

struct BatchView
{
    std::int32_t phaseCount;
    const double * rowMagnitude;
    const double * cosine;
    const double * sine;
    const std::size_t * offset;
    const std::int32_t * count;
    const DeviceComplex * nominal;
    const double * nominalMagnitude;
    const std::int64_t * sheetOffset;
    const DeviceComplex * plants;
    DeviceComplex * q;
    double * radius;
    SampleFloats * single;
    float * scale;
    double * largest;
    SegmentFloats * segment;
    SegmentDoubles * segmentExact;
    double * spacing;
    double * sheets;
    int * cancel;
};

class SheetRun
{
public:
    explicit SheetRun(const SheetProblem & problem);

    const SheetProblem & problem;
    std::size_t rows = 0;
    std::size_t columns = 0;
    Guard guard = Guard::None;
    Stream stream;
    CancelFlag flag;
    DeviceBuffer<double> rowMagnitude;
    DeviceBuffer<double> cosine;
    DeviceBuffer<double> sine;
};

struct BatchRange
{
    std::size_t first = 0;
    std::size_t count = 0;
};

std::vector<BatchRange> batchesOf(const SheetRun & run, const std::function<std::size_t(std::size_t)> & extraBytes);

class SheetBatch
{
public:
    SheetBatch(SheetRun & run, BatchRange range);

    BatchRange range;
    std::vector<std::size_t> offset;
    std::vector<std::int32_t> sampleCount;
    std::vector<std::int64_t> sheetOffset;
    std::int64_t sheetValues = 0;
    BatchView view{};

private:
    DeviceBuffer<std::size_t> m_offset;
    DeviceBuffer<std::int32_t> m_count;
    DeviceBuffer<DeviceComplex> m_nominal;
    DeviceBuffer<double> m_nominalMagnitude;
    DeviceBuffer<std::int64_t> m_sheetOffset;
    DeviceBuffer<DeviceComplex> m_plants;
    DeviceBuffer<DeviceComplex> m_q;
    DeviceBuffer<double> m_radius;
    DeviceBuffer<SampleFloats> m_single;
    DeviceBuffer<float> m_scale;
    DeviceBuffer<double> m_largest;
    DeviceBuffer<SegmentFloats> m_segment;
    DeviceBuffer<SegmentDoubles> m_segmentExact;
    DeviceBuffer<double> m_spacing;
    DeviceBuffer<double> m_sheets;
    std::vector<DeviceBuffer<std::int32_t>> m_groups;
};

}

#endif
