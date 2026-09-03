#include <cstdint>
#include <algorithm>

#include "boundary_union_1d.h"

namespace qftbx {

using std::abs;

BoundaryUnion1D::BoundaryUnion1D()
{

}

BoundaryUnion1D::~BoundaryUnion1D()
{

}

std::int32_t BoundaryUnion1D::bucketIndex(double x, double totalPhase)
{
    double res = totalPhase-(abs(x)*(totalPhase/(double)kPhaseDegrees));
    if(res<0) res=0;
    return (std::int32_t) res;
}

void BoundaryUnion1D::insertSorted(TraceSet & layerBucketsRow, std::int32_t index, qftbx::NicholsPoint point, double totalPhase)
{
    //The iterator used to be passed in from the caller, computed BEFORE this
    //function might have grown the same bucket; taking the index alone and
    //resolving the iterator here says the same thing without that trap.
    Trace & bucket = layerBucketsRow.at(static_cast<std::size_t>(bucketIndex(point.phase, totalPhase)));

    for (const qftbx::NicholsPoint & bucketPoint : bucket) {
        if (point.magnitude == bucketPoint.magnitude) {
            return;   //duplicate magnitude at this phase
        }
    }

    bucket.insert(bucket.begin() + index, point);
}

std::vector<TraceSet> BoundaryUnion1D::buildLayerBuckets(const TraceSet & chosenCurves, double totalPhase, bool open, bool upper)
{
    std::vector<TraceSet> layerBuckets (kLayerCount);

    for (std::int32_t i = 0; i < kLayerCount; i++)
    {
        TraceSet & row = layerBuckets[static_cast<std::size_t>(i)];
        row.resize(static_cast<std::size_t>(totalPhase) + 1);

        for (const qftbx::NicholsPoint & point : chosenCurves.at(static_cast<std::size_t>(i)))
        {
            Trace & bucket = row.at(static_cast<std::size_t>(bucketIndex(point.phase, totalPhase)));

            std::int32_t index = 0;
            for (const qftbx::NicholsPoint & placed : bucket)
            {
                if (upper)
                {
                    if (point.magnitude < placed.magnitude) index++;
                }
                else if (point.magnitude > placed.magnitude) index++;
            }

            //An open boundary keeps only ONE point per phase.
            if (!open || bucket.empty()) insertSorted(row, index, point, totalPhase);
        }
    }

    return layerBuckets;
}

//Layer 1: the points of the SECOND chosen curve that survive the first
//curve's buckets. The rewrite is by hand rather than mechanical: with the
//buckets held by value the whole function reads off `bucket`, a reference
//resolved once per point, instead of recomputing
//layerBuckets->at(0)->at(bucketIndex(...)) four times per branch.
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
            const std::int32_t bucketSize = static_cast<std::int32_t>(bucket.size());

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
                for (std::int32_t j = 0; j < bucketSize; j += 2)
                {
                    if (curvePoint.magnitude >= bucket[0].magnitude) layer1.push_back(curvePoint);

                    //The guard has to cover the FURTHEST index read, j+2:
                    //`bucketSize-j>1` only promised j+1, so an even bucket of
                    //more than two points reached at(bucketSize) on its last
                    //pair - one past the end. Not reachable with the current
                    //fixtures (verified by instrumenting the branch); which
                    //verdict the final pair of an even bucket deserves is a
                    //question for the thesis, noted in the plan.
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
            const std::int32_t bucketSize = static_cast<std::int32_t>(bucket.size());

            if (bucketSize > 1 && bucketSize % 2 == 0)
            {
                bool outside = true;

                if (open2 && curvePoint.magnitude <= bucket[0].magnitude)
                {
                    outside = false;
                }
                else
                {
                    for (std::int32_t j = 0; j < bucketSize; j += 2)
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

//Layer 2: the mirror of drawFirstLayer - the FIRST chosen curve against the
//SECOND curve's buckets, gated by open2 instead of open1. Two differences
//from its twin are deliberate and preserved: the inner test here is STRICT
//(< and >) where layer 1 uses <= and >=, and the open flag consulted in the
//closed branch is open1.
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
            const std::int32_t bucketSize = static_cast<std::int32_t>(bucket.size());

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
                for (std::int32_t j = 0; j < bucketSize; j += 2)
                {
                    if (curvePoint.magnitude >= bucket[0].magnitude) layer2.push_back(curvePoint);

                    //The same off-by-one drawFirstLayer had, which was fixed
                    //there and NOT here: the guard was `bucketSize-j>1`,
                    //which only promises j+1, while the branch reads j+2. An
                    //even bucket of more than two points therefore indexed
                    //one past the end on its last pair. Two mirrored
                    //functions, one fix - which is the argument for reading
                    //both when either changes.
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
            const std::int32_t bucketSize = static_cast<std::int32_t>(bucket.size());

            if (bucketSize > 1 && bucketSize % 2 == 0)
            {
                bool outside = true;

                if (open1 && curvePoint.magnitude <= bucket[0].magnitude)
                {
                    outside = false;
                }
                else
                {
                    for (std::int32_t j = 0; j < bucketSize; j += 2)
                    {
                        //STRICT here, unlike layer 1.
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
    double res = (abs(x)*((double)phaseCount/totalPhase));
    if(res<0) res=0;
    //The synthetic border point (|x| == totalPhase) used to yield bucket
    //phaseCount out of phaseCount buckets: out of range.
    if(res > phaseCount - 1) res = phaseCount - 1;
    return (std::int32_t) res;
}

TraceSet BoundaryUnion1D::buildUnionBuckets(const Trace & unionPoints, double totalPhase, std::int32_t pointCount)
{
    TraceSet unionBucketsRow (static_cast<std::size_t>(pointCount));

    //From the first point on (it used to be skipped), inserted sorted by
    //magnitude and deduplicated, like the layer buckets (and like the
    //historical file format).
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

void BoundaryUnion1D::run(const BoundaryData * boundaries, const TraceMetadata & traceMetadata)
{
    m_unionBuckets.clear();
    m_unionVectors.clear();
    m_openFlags.clear();
    m_upperFlags.clear();

    //One map per design frequency; each map holds, per specification, a
    //parametric curve of (phase, magnitude) points spanning -360 to 0
    //degrees.
    const BoundarySet & boundariesPerFrequency = boundaries->boundaries();

    const double totalPhase = -boundaries->phaseRange().min;
    const std::int32_t phasePointCount = boundaries->phaseCount();

    for (std::size_t i = 0; i < boundariesPerFrequency.size(); i++)
    {
        const std::map<QString, TraceSet> & map = boundariesPerFrequency[i];
        const std::map<QString, TraceLabels> & metadataMap = traceMetadata.at(i);

        Trace unionPoints;

        //Declared per FREQUENCY and deliberately not reset inside the
        //specification loop: once a boundary of this frequency has been seen
        //to be open, it stays open for the ones that follow.
        bool open1 = false, upper = false, open2 = false;

        m_openFlags.push_back(false);
        m_upperFlags.push_back(false);

        bool firstSpecification = true;

        //std::map iterates in key order, as QMap did.
        for (const auto & entry : map)
        {
            const TraceSet & specificationTraces = entry.second;

            //Metadata of THE SAME specification (a fresh iterator used to
            //be opened here, always reading the first key of the map).
            const auto foundMetadata = metadataMap.find(entry.first);
            if (foundMetadata != metadataMap.end() && !foundMetadata->second.empty())
            {
                //An x of 0 marks the allowed side as above, 1 as below; the
                //first point suffices, the whole trace shares one label.
                upper = !foundMetadata->second.front();
            }

            if (firstSpecification)
            {
                //The first specification is joined as an intersection.
                for (const Trace & trace : specificationTraces)
                {
                    unionPoints.insert(unionPoints.end(), trace.begin(), trace.end());
                }
                firstSpecification = false;
                continue;
            }

            //Every further specification is flattened into one curve.
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

            //The running union becomes the two layers merged. Every scratch
            //container here used to be heap-allocated and hand-freed, and an
            //extra specification abandoned the layer buckets whole.
            unionPoints = mergeLayers(layer1, layer2);

            if ((open1 || open2) && !m_openFlags.at(i))
            {
                m_openFlags[i] = open1 || open2;
                m_upperFlags[i] = upper;
            }
        }

        m_unionBuckets.push_back(buildUnionBuckets(unionPoints, totalPhase, phasePointCount));

        //Ordered by proximity only where the boundary is open: a closed one
        //is already a cycle.
        if (m_openFlags.at(i)) {
            m_unionVectors.push_back(sortByProximity(unionPoints));
        } else {
            m_unionVectors.push_back(std::move(unionPoints));
        }
    }
}

//take*: the caller becomes the owner by moving, which is what the engine
//always did with these - it read them and the union object was then deleted.
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


//Greedy nearest-neighbour ordering, starting from the leftmost point. The
//caller's vector used to be consumed (removeOne per step, then clear()),
//which is why it was taken by non-const pointer; it works on its own copy
//now and the caller keeps what it passed.
Trace BoundaryUnion1D::sortByProximity(const Trace & points) {

    Trace remaining = points;
    Trace ordered;
    ordered.reserve(remaining.size());

    qftbx::NicholsPoint tmp = qftbx::NicholsPoint(10000, 1);

    for (const qftbx::NicholsPoint & p : remaining) {
        if (p.phase < tmp.phase){
            tmp = qftbx::NicholsPoint (p);
        }
    }

    const auto removeOne = [&remaining](const qftbx::NicholsPoint & value) {
        const auto found = std::find(remaining.begin(), remaining.end(), value);
        if (found != remaining.end()) {
            remaining.erase(found);
        }
    };

    ordered.push_back(tmp);
    removeOne(tmp);

    const std::size_t lon = remaining.size();

    for (std::size_t i = 0; i < lon; i++){
        const qftbx::NicholsPoint uno(tmp);
        double dis = 10000;

        for (const qftbx::NicholsPoint & dos : remaining){
            const double dis2 = sqrt(pow(uno.phase - dos.phase, 2) + pow(uno.magnitude - dos.magnitude, 2));
            if (dis2 < dis){
                dis = dis2;
                tmp = qftbx::NicholsPoint (dos);
            }
        }

        ordered.push_back(tmp);
        removeOne(tmp);
    }

    return ordered;
}

} // namespace qftbx
