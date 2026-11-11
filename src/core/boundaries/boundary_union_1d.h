/**
 * @file
 * @brief The union of the per-specification boundaries of one frequency.
 *
 * Merges the curves of each design frequency into the single worst-case
 * boundary the chart shows and the search reads, working phase bucket by
 * phase bucket on the Nichols grid. The traces of the first specification
 * seed the running union; each further specification is merged against it
 * by bucketing both curves by phase and keeping, per phase, only the
 * points that remain binding: the most restrictive magnitude, honouring
 * whether the allowed region of each curve lies above or below it. The
 * result is a flat point set per frequency (unionVectors) and the same
 * points bucketed by phase, sorted ascending by magnitude and deduplicated
 * (unionBuckets), the layout the .qft file stores. The frequencies are
 * merged in parallel when OpenMP is there, each at its own index, and give
 * the same result as one after another.
 */

#ifndef QFTBX_BOUNDARY_UNION_1D_H
#define QFTBX_BOUNDARY_UNION_1D_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "src/core/boundaries/boundary_types.h"
#include "src/core/math/point.h"

#include "src/core/boundaries/boundary_data.h"

namespace qftbx {

class BoundaryUnion1D
{
public:
    void run(const BoundaryData & boundaries, const TraceMetadata & traceMetadata);

    UnionBuckets takeUnionBuckets();

    UnionTraces takeUnionVectors();

    std::vector<bool> takeOpenFlags();

    std::vector<bool> takeUpperFlags();

private:

    static constexpr std::int32_t kLayerCount = 2;
    static constexpr double kPhaseDegrees = 360.0;

    struct FrequencyUnion
    {
        Trace points;
        TraceSet buckets;
        bool open = false;
        bool upper = false;
    };

    UnionBuckets m_unionBuckets;
    UnionTraces m_unionVectors;

    std::vector<bool> m_openFlags;
    std::vector<bool> m_upperFlags;

    FrequencyUnion uniteFrequency(const std::map<std::string, TraceSet> & specifications,
                                  const std::map<std::string, TraceLabels> & labels, double totalPhase,
                                  std::int32_t phasePointCount) const;

    std::int32_t bucketIndex(double x, double totalPhase) const;

    void insertSorted(TraceSet & layerBuckets, std::size_t index, qftbx::NicholsPoint point, double totalPhase) const;

    std::vector<TraceSet> buildLayerBuckets(const TraceSet & chosenCurves, double totalPhase, bool open, bool upper) const;

    Trace drawFirstLayer(const TraceSet & chosenCurves, const std::vector<TraceSet> & layerBuckets, double totalPhase, bool open1, bool open2) const;

    Trace drawSecondLayer(const TraceSet & chosenCurves, const std::vector<TraceSet> & layerBuckets, double totalPhase, bool open1, bool open2) const;

    Trace mergeLayers(const Trace & layer1, const Trace & layer2) const;

    TraceSet buildUnionBuckets(const Trace & unionPoints, double totalPhase, std::size_t pointCount) const;

    std::int32_t bucketIndex(double x, double totalPhase, std::int32_t phaseCount) const;

    Trace sortByProximity(const Trace & points) const;

};

}

#endif
