/**
 * @file
 * @brief The kernel of the family's poles and its run.
 *
 * A block is four warps, and a warp takes one denominator after another.
 * Its threads share the work of each: two samples each, a butterfly each
 * per stage of the transform, a coefficient each and a root each, with the
 * largest values, the votes and the order of the frequencies on the axis
 * taken by shuffles. The twiddles and every warp's samples, coefficients
 * and roots are in shared memory. A warp keeps its own minima and bands in
 * registers, the frequency of rank p in thread p, and adds them with one
 * atomic each at the end; a frequency on the axis is positive, so its bits
 * order as the doubles do. The roots are a call of their own: inlined into
 * the free form's kernel, they made it spill registers.
 *
 * The run launches three times on one stream: the first denominator alone,
 * whose count the others are compared with, then all of them, and, only
 * when the first that stops the check is one whose poles cannot be placed,
 * the bands again over the combinations before it.
 */

#include "src/core/gpu/templates/family_poles.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstring>
#include <iterator>
#include <optional>

#include <cuda_runtime.h>

#include "src/core/gpu/common/check.cuh"
#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/device_probe.h"
#include "src/core/gpu/common/stream.cuh"
#include "src/core/gpu/templates/denominator_roots.cuh"
#include "src/core/gpu/templates/plant_evaluation.cuh"
#include "src/core/gpu/templates/plant_on_card.cuh"

namespace qftbx::gpu {

namespace {

constexpr unsigned int kFullWarp = 0xffffffffu;
constexpr int kFamilyWarps = 4;
constexpr int kFamilyBlock = kFamilyWarps * 32;
constexpr unsigned long long kNone = ~0ull;
constexpr unsigned long long kInfinityBits = 0x7ff0000000000000ull;
constexpr int kCountBits = 8;
constexpr int kTableDoubles = 4 * kSamples;
constexpr int kScratchDoubles = 2 * kSamples + 5 * kMaxDenominator;

static_assert(sizeof(RootTables) == kTableDoubles * sizeof(double), "the tables are four rows of samples");
static_assert(kCombinationBits + kCountBits <= 64, "a combination and its count share a word");

struct CaseView
{
    GridView grid;
    const std::uint64_t * stride;
};

struct FamilyTotals
{
    int firstCount;
    unsigned long long unplaceableAt;
    unsigned long long otherKey;
    unsigned long long bandLow[kMaxDenominator];
    unsigned long long bandHigh[kMaxDenominator];
};

enum class FamilyStage : int { First, Scan, Bands };

struct WarpScratch
{
    double * samplesRe;
    double * samplesIm;
    double * first;
    double * coefficients;
    double * monic;
    double * zRe;
    double * zIm;
};

struct LaneRoot
{
    DeviceComplex value;
    bool valid;
};

struct CaseOutcome
{
    bool placeable;
    int count;
    int axisCount;
    double axis;
};

template <bool Border, typename Index>
QFTBX_HD std::uint64_t caseDigits(const CaseView & cases, Index c, double * digit)
{
    if constexpr (Border) {
        digitsOf<true, Index>(cases.grid, c, digit);
        return static_cast<std::uint64_t>(c);
    } else {
        Index rest = c;
        std::uint64_t first = 0;
        for (std::int32_t j = 0; j < cases.grid.digitCount; ++j) {
            const Index size = static_cast<Index>(cases.grid.size[j]);
            const Index index = rest % size;
            digit[j] = cases.grid.values[cases.grid.offset[j] + static_cast<std::int64_t>(index)];
            first += static_cast<std::uint64_t>(index) * cases.stride[j];
            rest /= size;
        }
        return first;
    }
}

__device__ double warpMax(double value)
{
    for (int offset = 16; offset > 0; offset >>= 1) {
        value = ::fmax(value, __shfl_xor_sync(kFullWarp, value, offset));
    }
    return value;
}

__device__ __noinline__ LaneRoot rootsOnWarp(const double * coefficients, int n, WarpScratch & s, int lane)
{
    LaneRoot root{DeviceComplex(0.0, 0.0), false};
    double c = lane < n ? coefficients[lane] : 0.0;
    const double largest = warpMax(::fabs(c));
    c = ::fabs(c) <= 1e-13 * largest ? 0.0 : c;
    const unsigned int nonzero = __ballot_sync(kFullWarp, c != 0.0);
    if (nonzero == 0u) {
        return root;
    }
    const int first = __ffs(static_cast<int>(nonzero)) - 1;
    const int last = 31 - __clz(static_cast<int>(nonzero));
    const int degree = last - first;
    if (degree == 0) {
        return root;
    }

    const double lead = __shfl_sync(kFullWarp, c, first);
    const double monic = c / lead;
    __syncwarp();
    if (lane >= first && lane <= last) {
        s.monic[lane - first] = lane == first ? 1.0 : monic;
    }
    __syncwarp();
    if (degree == 1) {
        root.value = DeviceComplex(-s.monic[1], 0.0);
        root.valid = lane == 0;
        return root;
    }

    const double bound = warpMax(lane > first && lane <= last ? ::fabs(monic) : 0.0) + 1.0;
    DeviceComplex z = aberthStart(bound, lane, degree);
    if (lane < degree) {
        s.zRe[lane] = z.real();
        s.zIm[lane] = z.imag();
    }
    __syncwarp();
    for (int step = 0; step < kAberthSteps; ++step) {
        double measure = 0.0;
        if (lane < degree) {
            z = aberthStep(s.monic, degree, s.zRe, s.zIm, lane, bound, measure);
        }
        __syncwarp();
        if (lane < degree) {
            s.zRe[lane] = z.real();
            s.zIm[lane] = z.imag();
        }
        __syncwarp();
        if (warpMax(measure) < 1e-15) {
            break;
        }
    }
    root.value = cleanedRoot(z);
    root.valid = lane < degree;
    return root;
}

__device__ CaseOutcome countOnWarp(LaneRoot root, int lane)
{
    const double magnitude = root.valid ? cuda::std::abs(root.value) : 0.0;
    const double largest = ::fmax(1.0, warpMax(magnitude));
    const int count = __popc(__ballot_sync(kFullWarp, root.valid && inTheRightHalfPlane(root.value, largest)));
    const unsigned int above = __ballot_sync(kFullWarp, root.valid && onTheAxisAbove(root.value, largest));
    const double frequency = root.value.imag();

    int rank = 0;
    for (int m = 0; m < 32; ++m) {
        const double other = __shfl_sync(kFullWarp, frequency, m);
        const bool before = other < frequency || (other == frequency && m < lane);
        rank += ((above >> m) & 1u) != 0u && before ? 1 : 0;
    }
    double sorted = 0.0;
    for (int m = 0; m < 32; ++m) {
        const int position = __shfl_sync(kFullWarp, rank, m);
        const double value = __shfl_sync(kFullWarp, frequency, m);
        sorted = ((above >> m) & 1u) != 0u && position == lane ? value : sorted;
    }
    return {true, count, __popc(above), sorted};
}

__device__ bool passOnWarp(const PlantView & plant, DeviceComplex * values, const RootTables & tables, double radius,
                           double * out, int & degree, WarpScratch & s, int lane)
{
    __syncwarp();
    bool finite = true;
    for (int half = 0; half < 2; ++half) {
        const int j = lane + 32 * half;
        values[0] = DeviceComplex(radius * tables.unitRe[j], radius * tables.unitIm[j]);
        const DeviceComplex sample = runProgram(plant.program, plant.programLength, values);
        finite = finite && isfinite(sample.real()) && isfinite(sample.imag());
        const int at = reversedSample(j);
        s.samplesRe[at] = sample.real();
        s.samplesIm[at] = sample.imag();
    }
    if (!__all_sync(kFullWarp, finite)) {
        return false;
    }
    __syncwarp();
    for (int stage = 0; stage < kSampleBits; ++stage) {
        butterfly(s.samplesRe, s.samplesIm, tables, stage, lane);
        __syncwarp();
    }

    const DeviceComplex low = DeviceComplex(s.samplesRe[lane], s.samplesIm[lane]) / static_cast<double>(kSamples);
    const DeviceComplex high = DeviceComplex(s.samplesRe[lane + 32], s.samplesIm[lane + 32]) / static_cast<double>(kSamples);
    const double largest = warpMax(::fmax(cuda::std::abs(low), cuda::std::abs(high)));
    if (largest == 0.0) {
        if (lane == 0) {
            out[0] = 0.0;
        }
        degree = 0;
        __syncwarp();
        return true;
    }

    const bool beyond = (cuda::std::abs(high) > 1e-7 * largest)
            | (lane > kRecoveredDegree && cuda::std::abs(low) > 1e-7 * largest)
            | (lane <= kRecoveredDegree && ::fabs(low.imag()) > 1e-7 * largest);
    if (__any_sync(kFullWarp, beyond)) {
        return false;
    }
    const unsigned int significant =
            __ballot_sync(kFullWarp, lane <= kRecoveredDegree && ::fabs(low.real()) > 1e-9 * largest);
    degree = significant == 0u ? 0 : 31 - __clz(static_cast<int>(significant));
    if (lane <= degree) {
        out[degree - lane] = recoveredCoefficient(low.real(), largest, radius, lane);
    }
    __syncwarp();
    return true;
}

__device__ LaneRoot freeFormOnWarp(const PlantView & plant, const double * digit, const RootTables & tables,
                                   WarpScratch & s, int lane, bool & placeable)
{
    const LaneRoot none{DeviceComplex(0.0, 0.0), false};
    DeviceComplex values[kMaxValues];
    bindValues(plant, digit, values);

    int firstDegree = 0;
    placeable = passOnWarp(plant, values, tables, 1.0, s.first, firstDegree, s, lane);
    if (!placeable) {
        return none;
    }
    const LaneRoot firstRoot = rootsOnWarp(s.first, firstDegree + 1, s, lane);
    const double largestRoot = warpMax(firstRoot.valid ? cuda::std::abs(firstRoot.value) : 0.0);
    const double radius = ::fmax(4.0, 4.0 * largestRoot);

    int degree = 0;
    placeable = passOnWarp(plant, values, tables, radius, s.coefficients, degree, s, lane) && degree == firstDegree;
    if (!placeable) {
        return none;
    }
    const double power = lane <= degree ? ::pow(radius, static_cast<double>(lane)) : 0.0;
    const double wider = lane <= degree ? s.coefficients[degree - lane] : 0.0;
    const double narrow = lane <= degree ? s.first[degree - lane] : 0.0;
    const double largestScaled = warpMax(::fabs(wider) * power);
    placeable = !__any_sync(kFullWarp, ::fabs(wider - narrow) * power > 1e-7 * largestScaled);
    if (!placeable) {
        return none;
    }
    return rootsOnWarp(s.coefficients, degree + 1, s, lane);
}

template <PlantForm Form>
__device__ CaseOutcome caseOnWarp(const PlantView & plant, const double * digit, const RootTables & tables,
                                  WarpScratch & s, int lane)
{
    bool placeable = true;
    LaneRoot root{DeviceComplex(0.0, 0.0), false};
    if constexpr (Form == PlantForm::FreeForm) {
        root = freeFormOnWarp(plant, digit, tables, s, lane, placeable);
    } else if constexpr (Form == PlantForm::Polynomial) {
        __syncwarp();
        if (lane < plant.denominatorCount) {
            s.coefficients[lane] = coefficient(plant.denominatorSlot[lane], plant.denominatorNominal[lane], digit);
        }
        __syncwarp();
        root = rootsOnWarp(s.coefficients, plant.denominatorCount, s, lane);
    } else {
        const bool factor = lane < plant.denominatorCount;
        const double value = factor ? coefficient(plant.denominatorSlot[lane], plant.denominatorNominal[lane], digit) : 0.0;
        if constexpr (Form == PlantForm::TimeConstantGain) {
            placeable = !__any_sync(kFullWarp, factor && value == 0.0);
        }
        root = {DeviceComplex(-value, 0.0), factor};
    }
    if (!placeable) {
        return {false, 0, 0, 0.0};
    }
    return countOnWarp(root, lane);
}

template <PlantForm Form, bool Border, typename Index>
__global__ void __launch_bounds__(kFamilyBlock) familyPoles(PlantView plant, CaseView cases, Index caseCount,
                                                             const double * tableValues, FamilyStage stage,
                                                             unsigned long long limit, FamilyTotals * totals)
{
    extern __shared__ double shared[];
    for (int t = static_cast<int>(threadIdx.x); t < kTableDoubles; t += static_cast<int>(blockDim.x)) {
        shared[t] = tableValues[t];
    }
    __syncthreads();
    const RootTables & tables = *reinterpret_cast<const RootTables *>(shared);

    const int warp = static_cast<int>(threadIdx.x) / 32;
    const int lane = static_cast<int>(threadIdx.x) % 32;
    double * mine = shared + kTableDoubles + warp * kScratchDoubles;
    WarpScratch scratch{mine, mine + kSamples, mine + 2 * kSamples, mine + 2 * kSamples + kMaxDenominator,
                        mine + 2 * kSamples + 2 * kMaxDenominator, mine + 2 * kSamples + 3 * kMaxDenominator,
                        mine + 2 * kSamples + 4 * kMaxDenominator};

    const int firstCount = stage == FamilyStage::Scan ? totals->firstCount : -1;
    unsigned long long unplaceable = kNone;
    unsigned long long other = kNone;
    unsigned long long low = kInfinityBits;
    unsigned long long high = 0ull;

    const Index warps = static_cast<Index>(gridDim.x) * static_cast<Index>(kFamilyWarps);
    for (Index c = static_cast<Index>(blockIdx.x) * static_cast<Index>(kFamilyWarps) + static_cast<Index>(warp);
         c < caseCount; c += warps) {
        double digit[kMaxDigits];
        const std::uint64_t at = caseDigits<Border, Index>(cases, c, digit);
        if (stage == FamilyStage::Bands && at >= limit) {
            continue;
        }
        const CaseOutcome outcome = caseOnWarp<Form>(plant, digit, tables, scratch, lane);
        if (stage == FamilyStage::First) {
            if (lane == 0) {
                totals->firstCount = outcome.placeable ? outcome.count : -1;
            }
            continue;
        }
        if (!outcome.placeable) {
            unplaceable = unplaceable < at ? unplaceable : at;
            continue;
        }
        if (stage == FamilyStage::Scan && outcome.count != firstCount) {
            const unsigned long long key = (at << kCountBits) | static_cast<unsigned long long>(outcome.count);
            other = other < key ? other : key;
        }
        if (lane < outcome.axisCount) {
            const auto bits = static_cast<unsigned long long>(__double_as_longlong(outcome.axis));
            low = low < bits ? low : bits;
            high = high > bits ? high : bits;
        }
    }

    if (lane == 0 && unplaceable != kNone) {
        atomicMin(&totals->unplaceableAt, unplaceable);
    }
    if (lane == 0 && other != kNone) {
        atomicMin(&totals->otherKey, other);
    }
    if (low != kInfinityBits) {
        atomicMin(&totals->bandLow[lane], low);
        atomicMax(&totals->bandHigh[lane], high);
    }
}

struct Cases
{
    SweepGrid grid;
    std::vector<std::uint64_t> stride;
    std::size_t count = 0;
};

Cases casesOf(const PlantModel & plant, const SweepGrid & grid)
{
    Cases cases;
    if (grid.combinations == 0) {
        return cases;
    }
    cases.grid.border = grid.border;
    if (!grid.border.empty()) {
        cases.grid.grids = grid.grids;
        cases.grid.combinations = grid.combinations;
        cases.stride.assign(grid.grids.size(), 0);
        cases.count = grid.combinations;
        return cases;
    }

    std::vector<char> read(grid.grids.size(), 0);
    for (const std::int32_t slot : plant.denominatorSlot) {
        if (slot >= 0) {
            read[static_cast<std::size_t>(slot)] = 1;
        }
    }
    cases.count = 1;
    std::uint64_t stride = 1;
    for (std::size_t j = 0; j < grid.grids.size(); ++j) {
        const std::vector<double> & values = grid.grids[j];
        cases.stride.push_back(stride);
        if (read[j] != 0) {
            cases.grid.grids.push_back(values);
            cases.count *= values.size();
        } else {
            cases.grid.grids.push_back({values.front()});
        }
        stride *= values.size();
    }
    cases.grid.combinations = cases.count;
    return cases;
}

RootTables rootTables()
{
    RootTables tables{};
    for (int j = 0; j < kSamples; ++j) {
        const std::complex<double> unit = std::polar(1.0, 2.0 * qftbx::math::kPi * j / kSamples);
        tables.unitRe[j] = unit.real();
        tables.unitIm[j] = unit.imag();
        const std::complex<double> twiddle = std::polar(1.0, -2.0 * qftbx::math::kPi * j / kSamples);
        tables.twiddleRe[j] = twiddle.real();
        tables.twiddleIm[j] = twiddle.imag();
    }
    return tables;
}

FamilyTotals emptyTotals()
{
    FamilyTotals totals{};
    totals.firstCount = -1;
    totals.unplaceableAt = kNone;
    totals.otherKey = kNone;
    std::fill(std::begin(totals.bandLow), std::end(totals.bandLow), kInfinityBits);
    std::fill(std::begin(totals.bandHigh), std::end(totals.bandHigh), 0ull);
    return totals;
}

bool stopsAtUnplaceable(const FamilyTotals & totals)
{
    return totals.unplaceableAt != kNone && (totals.otherKey == kNone || (totals.otherKey >> kCountBits) > totals.unplaceableAt);
}

double asDouble(unsigned long long bits)
{
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}

FamilyOnCard summarized(const FamilyTotals & totals, std::size_t denominators)
{
    FamilyOnCard family;
    family.denominators = denominators;
    if (totals.firstCount >= 0) {
        family.firstCount = totals.firstCount;
    }
    if (stopsAtUnplaceable(totals)) {
        family.unplaceableAt = totals.unplaceableAt;
    } else if (totals.otherKey != kNone) {
        family.otherCountAt = totals.otherKey >> kCountBits;
        family.otherCount = static_cast<int>(totals.otherKey & ((1ull << kCountBits) - 1));
        return family;
    }
    if (family.unplaceableAt == std::uint64_t{0}) {
        return family;
    }
    for (int p = 0; p < kMaxDenominator && totals.bandLow[p] != kInfinityBits; ++p) {
        family.bands.push_back({asDouble(totals.bandLow[p]), asDouble(totals.bandHigh[p])});
    }
    return family;
}

template <typename Kernel>
unsigned int familyBlocks(Kernel kernel, std::size_t denominators, std::size_t sharedBytes)
{
    int perMultiprocessor = 0;
    check(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&perMultiprocessor, kernel, kFamilyBlock, sharedBytes),
          "cudaOccupancyMaxActiveBlocksPerMultiprocessor");
    const std::size_t resident = static_cast<std::size_t>(std::max(perMultiprocessor, 1))
            * static_cast<std::size_t>(std::max(probe().multiprocessors, 1));
    const std::size_t needed = (denominators + kFamilyWarps - 1) / kFamilyWarps;
    return static_cast<unsigned int>(std::max<std::size_t>(1, std::min(resident, needed)));
}

struct HostOutcome
{
    bool placeable = true;
    int count = 0;
    std::vector<double> axis;
};

std::vector<DeviceComplex> rootsOnProcessor(std::vector<double> a)
{
    double largest = 0.0;
    for (const double c : a) {
        largest = std::fmax(largest, std::fabs(c));
    }
    for (double & c : a) {
        c = std::fabs(c) <= 1e-13 * largest ? 0.0 : c;
    }
    const auto first = std::find_if(a.begin(), a.end(), [](double c) { return c != 0.0; });
    if (first == a.end()) {
        return {};
    }
    const auto last = std::find_if(a.rbegin(), a.rend(), [](double c) { return c != 0.0; });
    const int from = static_cast<int>(first - a.begin());
    const int degree = static_cast<int>(a.rend() - last) - 1 - from;
    if (degree == 0) {
        return {};
    }

    const double lead = a[static_cast<std::size_t>(from)];
    std::vector<double> monic(static_cast<std::size_t>(degree) + 1, 1.0);
    double bound = 0.0;
    for (int i = 1; i <= degree; ++i) {
        monic[static_cast<std::size_t>(i)] = a[static_cast<std::size_t>(from + i)] / lead;
        bound = std::fmax(bound, std::fabs(monic[static_cast<std::size_t>(i)]));
    }
    if (degree == 1) {
        return {DeviceComplex(-monic[1], 0.0)};
    }
    bound += 1.0;

    std::vector<double> zRe(static_cast<std::size_t>(degree));
    std::vector<double> zIm(static_cast<std::size_t>(degree));
    for (int j = 0; j < degree; ++j) {
        const DeviceComplex z = aberthStart(bound, j, degree);
        zRe[static_cast<std::size_t>(j)] = z.real();
        zIm[static_cast<std::size_t>(j)] = z.imag();
    }
    std::vector<DeviceComplex> moved(static_cast<std::size_t>(degree));
    for (int step = 0; step < kAberthSteps; ++step) {
        double largestStep = 0.0;
        for (int j = 0; j < degree; ++j) {
            double measure = 0.0;
            moved[static_cast<std::size_t>(j)] = aberthStep(monic.data(), degree, zRe.data(), zIm.data(), j, bound, measure);
            largestStep = std::fmax(largestStep, measure);
        }
        for (int j = 0; j < degree; ++j) {
            zRe[static_cast<std::size_t>(j)] = moved[static_cast<std::size_t>(j)].real();
            zIm[static_cast<std::size_t>(j)] = moved[static_cast<std::size_t>(j)].imag();
        }
        if (largestStep < 1e-15) {
            break;
        }
    }
    std::vector<DeviceComplex> roots;
    for (int j = 0; j < degree; ++j) {
        roots.push_back(cleanedRoot(DeviceComplex(zRe[static_cast<std::size_t>(j)], zIm[static_cast<std::size_t>(j)])));
    }
    return roots;
}

bool passOnProcessor(const PlantView & plant, std::vector<DeviceComplex> & values, const RootTables & tables,
                     double radius, std::vector<double> & out)
{
    double re[kSamples];
    double im[kSamples];
    for (int j = 0; j < kSamples; ++j) {
        values[0] = DeviceComplex(radius * tables.unitRe[j], radius * tables.unitIm[j]);
        const DeviceComplex sample = runProgram(plant.program, plant.programLength, values.data());
        if (!std::isfinite(sample.real()) || !std::isfinite(sample.imag())) {
            return false;
        }
        re[reversedSample(j)] = sample.real();
        im[reversedSample(j)] = sample.imag();
    }
    for (int stage = 0; stage < kSampleBits; ++stage) {
        for (int b = 0; b < kSamples / 2; ++b) {
            butterfly(re, im, tables, stage, b);
        }
    }

    DeviceComplex scaled[kSamples];
    double largest = 0.0;
    for (int k = 0; k < kSamples; ++k) {
        scaled[k] = DeviceComplex(re[k], im[k]) / static_cast<double>(kSamples);
        largest = std::fmax(largest, cuda::std::abs(scaled[k]));
    }
    if (largest == 0.0) {
        out.assign(1, 0.0);
        return true;
    }
    for (int k = 0; k < kSamples; ++k) {
        const bool beyond = k > kRecoveredDegree ? cuda::std::abs(scaled[k]) > 1e-7 * largest
                                                 : std::fabs(scaled[k].imag()) > 1e-7 * largest;
        if (beyond) {
            return false;
        }
    }
    int degree = 0;
    for (int k = kRecoveredDegree; k >= 0; --k) {
        if (std::fabs(scaled[k].real()) > 1e-9 * largest) {
            degree = k;
            break;
        }
    }
    out.assign(static_cast<std::size_t>(degree) + 1, 0.0);
    for (int k = 0; k <= degree; ++k) {
        out[static_cast<std::size_t>(degree - k)] = recoveredCoefficient(scaled[k].real(), largest, radius, k);
    }
    return true;
}

std::optional<std::vector<DeviceComplex>> freeFormOnProcessor(const PlantView & plant, const double * digit,
                                                              const RootTables & tables)
{
    std::vector<DeviceComplex> values(kMaxValues);
    bindValues(plant, digit, values.data());

    std::vector<double> narrow;
    if (!passOnProcessor(plant, values, tables, 1.0, narrow)) {
        return std::nullopt;
    }
    double largestRoot = 0.0;
    for (const DeviceComplex & root : rootsOnProcessor(narrow)) {
        largestRoot = std::fmax(largestRoot, cuda::std::abs(root));
    }
    const double radius = std::fmax(4.0, 4.0 * largestRoot);

    std::vector<double> wider;
    if (!passOnProcessor(plant, values, tables, radius, wider) || wider.size() != narrow.size()) {
        return std::nullopt;
    }
    const std::size_t degree = wider.size() - 1;
    double largestScaled = 0.0;
    for (std::size_t k = 0; k <= degree; ++k) {
        largestScaled = std::fmax(largestScaled, std::fabs(wider[degree - k]) * std::pow(radius, static_cast<double>(k)));
    }
    for (std::size_t k = 0; k <= degree; ++k) {
        if (std::fabs(wider[degree - k] - narrow[degree - k]) * std::pow(radius, static_cast<double>(k))
                > 1e-7 * largestScaled) {
            return std::nullopt;
        }
    }
    return rootsOnProcessor(wider);
}

template <PlantForm Form>
HostOutcome caseOnProcessor(const PlantView & plant, const double * digit, const RootTables & tables)
{
    HostOutcome outcome;
    std::vector<DeviceComplex> roots;
    if constexpr (Form == PlantForm::FreeForm) {
        const std::optional<std::vector<DeviceComplex>> found = freeFormOnProcessor(plant, digit, tables);
        if (!found.has_value()) {
            outcome.placeable = false;
            return outcome;
        }
        roots = *found;
    } else if constexpr (Form == PlantForm::Polynomial) {
        std::vector<double> coefficients;
        for (std::int32_t c = 0; c < plant.denominatorCount; ++c) {
            coefficients.push_back(coefficient(plant.denominatorSlot[c], plant.denominatorNominal[c], digit));
        }
        roots = rootsOnProcessor(coefficients);
    } else {
        for (std::int32_t c = 0; c < plant.denominatorCount; ++c) {
            const double value = coefficient(plant.denominatorSlot[c], plant.denominatorNominal[c], digit);
            if (Form == PlantForm::TimeConstantGain && value == 0.0) {
                outcome.placeable = false;
                return outcome;
            }
            roots.emplace_back(-value, 0.0);
        }
    }

    double largest = 1.0;
    for (const DeviceComplex & root : roots) {
        largest = std::fmax(largest, cuda::std::abs(root));
    }
    for (const DeviceComplex & root : roots) {
        outcome.count += inTheRightHalfPlane(root, largest) ? 1 : 0;
        if (onTheAxisAbove(root, largest)) {
            outcome.axis.push_back(root.imag());
        }
    }
    std::sort(outcome.axis.begin(), outcome.axis.end());
    return outcome;
}

std::uint64_t bitsOf(double value)
{
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

}

FamilyOnCard familyOnCard(const PlantModel & plant, const SweepGrid & grid)
{
    const Cases cases = casesOf(plant, grid);
    if (cases.count == 0) {
        return FamilyOnCard{};
    }

    useProbedDevice();
    Stream stream;
    const FlatGrid flat = flatten(cases.grid);
    ModelOnCard card;
    upload(card, plant, cases.grid, flat, {}, stream);
    DeviceBuffer<std::uint64_t> stride = uploaded(cases.stride, stream);
    const RootTables tables = rootTables();
    DeviceBuffer<RootTables> tablesOnCard(1, stream);
    tablesOnCard.upload(&tables);
    const FamilyTotals empty = emptyTotals();
    DeviceBuffer<FamilyTotals> totals(1, stream);
    totals.upload(&empty);
    const CaseView view{card.grid, stride.data()};
    const double * tableValues = reinterpret_cast<const double *>(tablesOnCard.data());
    const std::size_t sharedBytes = (kTableDoubles + kFamilyWarps * kScratchDoubles) * sizeof(double);

    const auto computing = std::chrono::steady_clock::now();
    FamilyTotals reduced = empty;
    dispatch(plant.form, !cases.grid.border.empty(), cases.count >= (std::size_t{1} << 31),
             [&](auto formTag, auto borderTag, auto index) {
        using Index = decltype(index);
        const auto kernel = familyPoles<decltype(formTag)::value, decltype(borderTag)::value, Index>;
        const unsigned int blocks = familyBlocks(kernel, cases.count, sharedBytes);
        kernel<<<1, kFamilyBlock, sharedBytes, stream.get()>>>(card.denominator, view, Index{1}, tableValues,
                                                              FamilyStage::First, kNone, totals.data());
        checkLaunch("the kernel of the family's first denominator");
        kernel<<<blocks, kFamilyBlock, sharedBytes, stream.get()>>>(card.denominator, view, static_cast<Index>(cases.count),
                                                                   tableValues, FamilyStage::Scan, kNone, totals.data());
        checkLaunch("the kernel of the family's denominators");
        totals.download(&reduced);
        stream.synchronize();

        if (stopsAtUnplaceable(reduced) && reduced.unplaceableAt > 0) {
            totals.upload(&empty);
            kernel<<<blocks, kFamilyBlock, sharedBytes, stream.get()>>>(card.denominator, view,
                                                                       static_cast<Index>(cases.count), tableValues,
                                                                       FamilyStage::Bands, reduced.unplaceableAt,
                                                                       totals.data());
            checkLaunch("the kernel of the family's bands");
            FamilyTotals before = empty;
            totals.download(&before);
            stream.synchronize();
            std::copy(std::begin(before.bandLow), std::end(before.bandLow), std::begin(reduced.bandLow));
            std::copy(std::begin(before.bandHigh), std::end(before.bandHigh), std::begin(reduced.bandHigh));
        }
    });

    FamilyOnCard family = summarized(reduced, cases.count);
    family.kernelMilliseconds =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - computing).count();
    return family;
}

FamilyOnCard familyOnProcessor(const PlantModel & plant, const SweepGrid & grid)
{
    const Cases cases = casesOf(plant, grid);
    if (cases.count == 0) {
        return FamilyOnCard{};
    }

    const FlatGrid flat = flatten(cases.grid);
    PlantView view = viewOf(plant);
    view.program = plant.denominatorProgram.data();
    view.programLength = static_cast<std::int32_t>(plant.denominatorProgram.size());
    const CaseView caseView{viewOf(cases.grid, flat), cases.stride.data()};
    const RootTables tables = rootTables();

    std::vector<std::uint64_t> at(cases.count);
    std::vector<HostOutcome> outcomes(cases.count);
    dispatch(plant.form, !cases.grid.border.empty(), false, [&](auto formTag, auto borderTag, auto) {
        for (std::size_t c = 0; c < cases.count; ++c) {
            double digit[kMaxDigits] = {};
            at[c] = caseDigits<decltype(borderTag)::value, std::size_t>(caseView, c, digit);
            outcomes[c] = caseOnProcessor<decltype(formTag)::value>(view, digit, tables);
        }
    });

    FamilyTotals totals = emptyTotals();
    totals.firstCount = outcomes[0].placeable ? outcomes[0].count : -1;
    for (std::size_t c = 0; c < cases.count; ++c) {
        if (!outcomes[c].placeable) {
            totals.unplaceableAt = std::min<unsigned long long>(totals.unplaceableAt, at[c]);
        } else if (outcomes[c].count != totals.firstCount) {
            totals.otherKey = std::min<unsigned long long>(
                    totals.otherKey, (at[c] << kCountBits) | static_cast<unsigned long long>(outcomes[c].count));
        }
    }
    const unsigned long long limit = stopsAtUnplaceable(totals) ? totals.unplaceableAt : kNone;
    for (std::size_t c = 0; c < cases.count; ++c) {
        if (!outcomes[c].placeable || at[c] >= limit) {
            continue;
        }
        for (std::size_t p = 0; p < outcomes[c].axis.size(); ++p) {
            const unsigned long long bits = bitsOf(outcomes[c].axis[p]);
            totals.bandLow[p] = std::min<unsigned long long>(totals.bandLow[p], bits);
            totals.bandHigh[p] = std::max<unsigned long long>(totals.bandHigh[p], bits);
        }
    }
    return summarized(totals, cases.count);
}

}
