/**
 * @file
 * @brief The phase-bucketed merge of the boundaries of one frequency.
 *
 * The points of every curve are bucketed by phase, sorted by magnitude and
 * deduplicated. The first specification joins the union as an intersection
 * and every further one is flattened into one curve; an open boundary keeps
 * a single point per phase, and once a boundary of a frequency has been seen
 * open it stays open for the specifications that follow. The two layers of
 * the merge are mirrors of each other with one deliberate difference: the
 * inner test of the second is strict where the first is inclusive. Synthetic
 * border points close the union against the window frame.
 */

#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <limits>

#include "src/core/boundaries/boundary_union_1d.h"

namespace qftbx {

using std::abs;

std::int32_t BoundaryUnion1D::bucketIndex(double x, double totalPhase)
{
    double res = totalPhase-(abs(x)*(totalPhase/kPhaseDegrees));
    if(res<0) res=0;
    return static_cast<std::int32_t>(res);
}

void BoundaryUnion1D::insertSorted(TraceSet & layerBucketsRow, std::size_t index, qftbx::NicholsPoint point, double totalPhase)
{
    Trace & bucket = layerBucketsRow.at(static_cast<std::size_t>(bucketIndex(point.phase, totalPhase)));

    for (const qftbx::NicholsPoint & bucketPoint : bucket) {
        if (point.magnitude == bucketPoint.magnitude) {
            return;
        }
    }

    bucket.insert(bucket.begin() + static_cast<std::ptrdiff_t>(index), point);
}

std::vector<TraceSet> BoundaryUnion1D::buildLayerBuckets(const TraceSet & chosenCurves, double totalPhase, bool open, bool upper)
{
    std::vector<TraceSet> layerBuckets (kLayerCount);

    for (std::size_t i = 0; i < kLayerCount; i++)
    {
        TraceSet & row = layerBuckets[i];
        row.resize(static_cast<std::size_t>(totalPhase) + 1);

        for (const qftbx::NicholsPoint & point : chosenCurves.at(i))
        {
            Trace & bucket = row.at(static_cast<std::size_t>(bucketIndex(point.phase, totalPhase)));

            std::size_t index = 0;
            for (const qftbx::NicholsPoint & placed : bucket)
            {
                if (upper)
                {
                    if (point.magnitude < placed.magnitude) index++;
                }
                else if (point.magnitude > placed.magnitude) index++;
            }

            if (!open || bucket.empty()) insertSorted(row, index, point, totalPhase);
        }
    }

    return layerBuckets;
}

Trace BoundaryUnion1D::drawFirstLayer(const TraceSet & chosenCurves,
                                      const std::vector<TraceSet> & layerBuckets,
                                      double totalPhase, bool open1, bool open2)
{
    Trace layer1;

    const TraceSet & firstCurveBuckets = layerBuckets.at(0);
    const Trace & secondCurve = chosenCurves.at(1);

    if (open1)
    {
        for (const qftbx::NicholsPoint & curvePoint : secondCurve)
        {
            const Trace & bucket =
                    firstCurveBuckets.at(static_cast<std::size_t>(bucketIndex(curvePoint.phase, totalPhase)));
            const std::size_t bucketSize = bucket.size();

            if (bucketSize == 0) layer1.push_back(curvePoint);

            if (bucketSize == 1)
            {
                if (curvePoint.magnitude >= bucket[0].magnitude) layer1.push_back(curvePoint);
            }
            else if (bucketSize == 2)
            {
                if ((curvePoint.magnitude >= bucket[0].magnitude) ||
                        (curvePoint.magnitude <= bucket[1].magnitude)) layer1.push_back(curvePoint);
            }
            else
            {
                for (std::size_t j = 0; j < bucketSize; j += 2)
                {
                    if (curvePoint.magnitude >= bucket[0].magnitude) layer1.push_back(curvePoint);

                    if (j + 2 < bucketSize)
                    {
                        if ((curvePoint.magnitude <= bucket[j + 1].magnitude) &&
                                (curvePoint.magnitude >= bucket[j + 2].magnitude)) layer1.push_back(curvePoint);
                    }
                    else
                    {
                        if ((curvePoint.magnitude >= bucket[j].magnitude) &&
                                (curvePoint.magnitude <= bucket[j - 1].magnitude)) layer1.push_back(curvePoint);
                    }
                }
            }
        }
    }
    else
    {
        for (const qftbx::NicholsPoint & curvePoint : secondCurve)
        {
            const Trace & bucket =
                    firstCurveBuckets.at(static_cast<std::size_t>(bucketIndex(curvePoint.phase, totalPhase)));
            const std::size_t bucketSize = bucket.size();

            if (bucketSize > 1 && bucketSize % 2 == 0)
            {
                bool outside = true;

                if (open2 && curvePoint.magnitude <= bucket[0].magnitude)
                {
                    outside = false;
                }
                else
                {
                    for (std::size_t j = 0; j < bucketSize; j += 2)
                    {
                        if ((curvePoint.magnitude <= bucket[j].magnitude) &&
                                (curvePoint.magnitude >= bucket[j + 1].magnitude)) outside = false;
                    }
                }

                if (outside) layer1.push_back(curvePoint);
            }
            else if (bucketSize % 2 == 1 && curvePoint.magnitude > bucket[0].magnitude)
            {
                layer1.push_back(curvePoint);
            }
            else if (bucketSize == 0) layer1.push_back(curvePoint);
        }
    }

    return layer1;
}

Trace BoundaryUnion1D::drawSecondLayer(const TraceSet & chosenCurves,
                                       const std::vector<TraceSet> & layerBuckets,
                                       double totalPhase, bool open1, bool open2)
{
    Trace layer2;

    const TraceSet & secondCurveBuckets = layerBuckets.at(1);
    const Trace & firstCurve = chosenCurves.at(0);

    if (open2)
    {
        for (const qftbx::NicholsPoint & curvePoint : firstCurve)
        {
            const Trace & bucket =
                    secondCurveBuckets.at(static_cast<std::size_t>(bucketIndex(curvePoint.phase, totalPhase)));
            const std::size_t bucketSize = bucket.size();

            if (bucketSize == 0) layer2.push_back(curvePoint);

            if (bucketSize == 1)
            {
                if (curvePoint.magnitude >= bucket[0].magnitude) layer2.push_back(curvePoint);
            }
            else if (bucketSize == 2)
            {
                if ((curvePoint.magnitude >= bucket[0].magnitude) ||
                        (curvePoint.magnitude <= bucket[1].magnitude)) layer2.push_back(curvePoint);
            }
            else
            {
                for (std::size_t j = 0; j < bucketSize; j += 2)
                {
                    if (curvePoint.magnitude >= bucket[0].magnitude) layer2.push_back(curvePoint);

                    if (j + 2 < bucketSize)
                    {
                        if ((curvePoint.magnitude <= bucket[j + 1].magnitude) &&
                                (curvePoint.magnitude >= bucket[j + 2].magnitude)) layer2.push_back(curvePoint);
                    }
                    else
                    {
                        if ((curvePoint.magnitude >= bucket[j].magnitude) &&
                                (curvePoint.magnitude <= bucket[j - 1].magnitude)) layer2.push_back(curvePoint);
                    }
                }
            }
        }
    }
    else
    {
        for (const qftbx::NicholsPoint & curvePoint : firstCurve)
        {
            const Trace & bucket =
                    secondCurveBuckets.at(static_cast<std::size_t>(bucketIndex(curvePoint.phase, totalPhase)));
            const std::size_t bucketSize = bucket.size();

            if (bucketSize > 1 && bucketSize % 2 == 0)
            {
                bool outside = true;

                if (open1 && curvePoint.magnitude <= bucket[0].magnitude)
                {
                    outside = false;
                }
                else
                {
                    for (std::size_t j = 0; j < bucketSize; j += 2)
                    {
                        if ((curvePoint.magnitude < bucket[j].magnitude) &&
                                (curvePoint.magnitude > bucket[j + 1].magnitude)) outside = false;
                    }
                }

                if (outside) layer2.push_back(curvePoint);
            }
            else if (bucketSize % 2 == 1 && curvePoint.magnitude > bucket[0].magnitude)
            {
                layer2.push_back(curvePoint);
            }
            else if (bucketSize == 0) layer2.push_back(curvePoint);
        }
    }

    return layer2;
}

inline std::int32_t BoundaryUnion1D::bucketIndex(double x, double totalPhase, std::int32_t phaseCount)
{
    double res = (abs(x)*(static_cast<double>(phaseCount)/totalPhase));
    if(res<0) res=0;
    if(res > phaseCount - 1) res = phaseCount - 1;
    return static_cast<std::int32_t>(res);
}

TraceSet BoundaryUnion1D::buildUnionBuckets(const Trace & unionPoints, double totalPhase, std::size_t pointCount)
{
    TraceSet unionBucketsRow (pointCount);

    for (const qftbx::NicholsPoint & point : unionPoints) {
        Trace & bucket =
                unionBucketsRow.at(static_cast<std::size_t>(bucketIndex(point.phase, totalPhase, pointCount)));

        std::size_t pos = 0;
        bool duplicated = false;
        for (; pos < bucket.size(); pos++){
            if (bucket[pos].magnitude == point.magnitude){
                duplicated = true;
                break;
            }
            if (bucket[pos].magnitude > point.magnitude){
                break;
            }
        }
        if (!duplicated){
            bucket.insert(bucket.begin() + static_cast<std::ptrdiff_t>(pos), point);
        }
    }

    return unionBucketsRow;
}

Trace BoundaryUnion1D::mergeLayers(const Trace & layer1, const Trace & layer2)
{
    Trace merged = layer1;
    merged.insert(merged.end(), layer2.begin(), layer2.end());

    return merged;
}

void BoundaryUnion1D::run(const BoundaryData & boundaries, const TraceMetadata & traceMetadata)
{
    m_unionBuckets.clear();
    m_unionVectors.clear();
    m_openFlags.clear();
    m_upperFlags.clear();

    const BoundarySet & boundariesPerFrequency = boundaries.boundaries();

    const double totalPhase = -boundaries.phaseRange().min;
    const std::int32_t phasePointCount = boundaries.phaseCount();

    for (std::size_t i = 0; i < boundariesPerFrequency.size(); i++)
    {
        const std::map<std::string, TraceSet> & map = boundariesPerFrequency[i];
        const std::map<std::string, TraceLabels> & metadataMap = traceMetadata.at(i);

        Trace unionPoints;

        bool open1 = false, upper = false, open2 = false;

        m_openFlags.push_back(false);
        m_upperFlags.push_back(false);

        bool firstSpecification = true;

        for (const auto & entry : map)
        {
            const TraceSet & specificationTraces = entry.second;

            const auto foundMetadata = metadataMap.find(entry.first);
            if (foundMetadata != metadataMap.end() && !foundMetadata->second.empty())
            {
                upper = !foundMetadata->second.front();
            }

            if (firstSpecification)
            {
                for (const Trace & trace : specificationTraces)
                {
                    unionPoints.insert(unionPoints.end(), trace.begin(), trace.end());
                }
                firstSpecification = false;
                continue;
            }

            Trace currentCurve;
            for (const Trace & trace : specificationTraces)
            {
                currentCurve.insert(currentCurve.end(), trace.begin(), trace.end());
            }

            if (static_cast<double>(unionPoints.size()) >= totalPhase) open1 = true;
            if (static_cast<double>(currentCurve.size()) >= totalPhase) open2 = true;

            const TraceSet chosenCurves{unionPoints, currentCurve};
            const std::vector<TraceSet> layerBuckets =
                    buildLayerBuckets(chosenCurves, totalPhase, open1 || open2, upper);

            const Trace layer1 = drawFirstLayer(chosenCurves, layerBuckets, totalPhase, open1, open2);
            const Trace layer2 = drawSecondLayer(chosenCurves, layerBuckets, totalPhase, open1, open2);

            unionPoints = mergeLayers(layer1, layer2);

            if ((open1 || open2) && !m_openFlags.at(i))
            {
                m_openFlags[i] = open1 || open2;
                m_upperFlags[i] = upper;
            }
        }

        m_unionBuckets.push_back(buildUnionBuckets(unionPoints, totalPhase, static_cast<std::size_t>(phasePointCount)));

        if (m_openFlags.at(i)) {
            m_unionVectors.push_back(sortByProximity(unionPoints));
        } else {
            m_unionVectors.push_back(std::move(unionPoints));
        }
    }
}

UnionBuckets BoundaryUnion1D::takeUnionBuckets()
{
    return std::move(m_unionBuckets);
}

UnionTraces BoundaryUnion1D::takeUnionVectors()
{
    return std::move(m_unionVectors);
}

std::vector<bool> BoundaryUnion1D::takeOpenFlags()
{
    return std::move(m_openFlags);
}

std::vector<bool> BoundaryUnion1D::takeUpperFlags()
{
    return std::move(m_upperFlags);
}

Trace BoundaryUnion1D::sortByProximity(const Trace & points) {

    if (points.empty()) {
        return {};
    }

    Trace remaining = points;
    Trace ordered;
    ordered.reserve(remaining.size());

    qftbx::NicholsPoint current = *std::min_element(
                remaining.begin(), remaining.end(),
                [](const qftbx::NicholsPoint & a, const qftbx::NicholsPoint & b) {
                    return a.phase < b.phase;
                });

    const auto removeOne = [&remaining](const qftbx::NicholsPoint & value) {
        const auto found = std::find(remaining.begin(), remaining.end(), value);
        if (found != remaining.end()) {
            remaining.erase(found);
        }
    };

    ordered.push_back(current);
    removeOne(current);

    const std::size_t stepCount = remaining.size();

    for (std::size_t i = 0; i < stepCount; i++){
        const qftbx::NicholsPoint from(current);
        double nearest = std::numeric_limits<double>::infinity();

        for (const qftbx::NicholsPoint & candidate : remaining){
            const double distance = std::hypot(from.phase - candidate.phase, from.magnitude - candidate.magnitude);
            if (distance < nearest){
                nearest = distance;
                current = candidate;
            }
        }

        ordered.push_back(current);
        removeOne(current);
    }

    return ordered;
}

}
