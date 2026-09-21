/**
 * @file
 * @brief Algorithm MC3, under development.
 *
 * The trial implementation of the search that keeps the gain out of the
 * tree; it runs only from the benchmark.
 */

#include "src/core/loopshaping/mc3/algorithm_mc3.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "src/core/common/exception.h"
#include "src/core/loopshaping/common/common_functions.h"
#include "src/core/math/interval.h"

namespace qftbx {

namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();

double toDb(double gain) { return 20.0 * std::log10(gain); }
double fromDb(double db) { return std::pow(10.0, db / 20.0); }

bool positive(const Range & r) { return r.min > 0.0 && r.max > r.min; }

}

void AlgorithmMc3::setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega,
                              const BoundaryData * boundaries, double epsilon)
{
    this->plant = plant;
    this->controller = controller->clone();
    depthAccounting.start(*this->controller);
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;
}

RangeUnion AlgorithmMc3::complementOf(const RangeUnion & set)
{
    std::vector<double> lower, upper;
    double previous = -kInfinity;
    for (const Range & part : set.components()) {
        lower.push_back(previous);
        upper.push_back(part.min);
        previous = part.max;
    }
    lower.push_back(previous);
    upper.push_back(kInfinity);
    return RangeUnion::of(lower.data(), upper.data(), lower.size());
}

RangeUnion AlgorithmMc3::unionOf(const RangeUnion & a, const RangeUnion & b)
{
    std::vector<double> lower, upper;
    for (const RangeUnion * set : {&a, &b}) {
        for (const Range & part : set->components()) {
            lower.push_back(part.min);
            upper.push_back(part.max);
        }
    }
    return RangeUnion::of(lower.data(), upper.data(), lower.size());
}

AlgorithmMc3::GainSets AlgorithmMc3::gainSetsOf(LtiSystem * box)
{
    GainSets sets;
    sets.certified = RangeUnion::whole();
    sets.forbidden = RangeUnion();
    const bool conservative = detector->conservative();

    for (std::size_t i = 0; i < omega->size(); ++i) {
        const NaturalIntervalExtension::Factors factors = conversion->factorsOf(box, omega->at(i));
        const NicholsBox unit = conversion->nicholsOf(Interval(1.0), factors, nominalPlantValues.at(i));
        ++m_projections;

        const double mLo = unit.magnitudeDb.lower(), mHi = unit.magnitudeDb.upper();
        const double pLo = unit.phaseDegrees.lower(), pHi = unit.phaseDegrees.upper();
        if (mHi - mLo >= epsilon || pHi - pLo >= epsilon) {
            sets.small = false;
        }

        const BoundaryColumns & columns = boundaries->columns(i);
        const std::int32_t first = conservative ? columns.firstColumnCovering(pLo) : columns.columnOf(pLo);
        const std::int32_t last = conservative ? columns.lastColumnCovering(pHi) : columns.columnOf(pHi);

        RangeUnion certifiedHere = RangeUnion::whole();
        RangeUnion forbiddenHere = RangeUnion::whole();
        std::vector<double> lower, upper, gapLower, gapUpper;

        for (std::int32_t c = first; c <= last; ++c) {
            const BoundaryColumns::Intervals spans = columns.intervals(c);
            lower.clear(); upper.clear(); gapLower.clear(); gapUpper.clear();

            double previousHi = -kInfinity;
            for (std::int32_t j = 0; j < spans.count; ++j) {
                lower.push_back(spans.lo[j] - mLo);
                upper.push_back(spans.hi[j] - mHi);
                if (spans.lo[j] > previousHi) {
                    gapLower.push_back(previousHi - mLo);
                    gapUpper.push_back(spans.lo[j] - mHi);
                }
                previousHi = spans.hi[j];
            }
            if (previousHi < kInfinity) {
                gapLower.push_back(previousHi - mLo);
                gapUpper.push_back(kInfinity);
            }

            certifiedHere.intersectWith(RangeUnion::of(lower.data(), upper.data(), lower.size()));
            forbiddenHere.intersectWith(spans.count == 0 ? RangeUnion::whole()
                                                         : RangeUnion::of(gapLower.data(), gapUpper.data(), gapLower.size()));
            if (certifiedHere.isEmpty() && forbiddenHere.isEmpty()) {
                break;
            }
        }

        sets.certified.intersectWith(certifiedHere);
        sets.forbidden = unionOf(sets.forbidden, forbiddenHere);
        if (certifiedHere.isEmpty()) {
            ++sets.ambiguousFrequencies;
            depthAccounting.ambiguousAt(i);
        }
    }
    return sets;
}

std::unique_ptr<LtiSystem> AlgorithmMc3::withGains(LtiSystem * box, const RangeUnion & gains) const
{
    const double lo = fromDb(gains.minimum()), hi = fromDb(gains.maximum());
    return box->create(box->name(), box->numerator(), box->denominator(),
                       Parameter(box->gain().name(), Range(lo, hi), lo), box->delay());
}

std::pair<std::unique_ptr<LtiSystem>, std::unique_ptr<LtiSystem>> AlgorithmMc3::bisect(LtiSystem * box) const
{
    int widestIndex = -1;
    bool widestIsZero = true;
    double widest = -1.0;
    const auto consider = [&](const std::vector<Parameter> & list, bool zeros) {
        for (std::size_t i = 0; i < list.size(); ++i) {
            const Parameter & p = list[i];
            if (!p.isUncertain()) continue;
            const Range r = p.range();
            const double width = positive(r) ? std::log(r.max / r.min) : (r.max - r.min);
            if (width > widest) {
                widest = width; widestIndex = static_cast<int>(i); widestIsZero = zeros;
            }
        }
    };
    consider(box->numerator(), true);
    consider(box->denominator(), false);

    std::vector<Parameter> lowNum = box->numerator(), lowDen = box->denominator();
    std::vector<Parameter> highNum = lowNum, highDen = lowDen;
    if (widestIndex >= 0) {
        std::vector<Parameter> & low = widestIsZero ? lowNum : lowDen;
        std::vector<Parameter> & high = widestIsZero ? highNum : highDen;
        const Parameter & p = low[static_cast<std::size_t>(widestIndex)];
        const Range r = p.range();
        const double split = positive(r) ? std::sqrt(r.min * r.max) : 0.5 * (r.min + r.max);
        low[static_cast<std::size_t>(widestIndex)] = Parameter(p.name(), Range(r.min, split), r.min);
        high[static_cast<std::size_t>(widestIndex)] = Parameter(p.name(), Range(split, r.max), split);
    }
    return {box->create(box->name(), std::move(lowNum), std::move(lowDen), box->gain(), box->delay()),
            box->create(box->name(), std::move(highNum), std::move(highDen), box->gain(), box->delay())};
}

PointController AlgorithmMc3::centreOf(LtiSystem * box, double gainDb)
{
    PointController point;
    point.gain = fromDb(gainDb);
    for (Parameter & p : box->numerator()) {
        point.zeros.push_back(p.isUncertain() ? (positive(p.range()) ? std::sqrt(p.range().min * p.range().max) : p.range().middle()) : p.nominal());
    }
    for (Parameter & p : box->denominator()) {
        point.poles.push_back(p.isUncertain() ? (positive(p.range()) ? std::sqrt(p.range().min * p.range().max) : p.range().middle()) : p.nominal());
    }
    return point;
}

RangeUnion AlgorithmMc3::pointGainSet(const PointController & point)
{
    RangeUnion admissible = RangeUnion::whole();
    const bool conservative = detector->conservative();
    std::vector<double> lower, upper;
    for (std::size_t i = 0; i < omega->size(); ++i) {
        PointController unit = point;
        unit.gain = 1.0;
        const NicholsBox at = conversion->nicholsPoint(unit, omega->at(i), nominalPlantValues.at(i));
        const double m = at.magnitudeDb.upper(), phi = at.phaseDegrees.lower();
        const BoundaryColumns & columns = boundaries->columns(i);
        const std::int32_t first = conservative ? columns.firstColumnCovering(phi) : columns.columnOf(phi);
        const std::int32_t last = conservative ? columns.lastColumnCovering(phi) : columns.columnOf(phi);
        for (std::int32_t c = first; c <= last; ++c) {
            const BoundaryColumns::Intervals spans = columns.intervals(c);
            lower.clear(); upper.clear();
            for (std::int32_t j = 0; j < spans.count; ++j) {
                lower.push_back(spans.lo[j] - m);
                upper.push_back(spans.hi[j] - m);
            }
            admissible.intersectWith(RangeUnion::of(lower.data(), upper.data(), lower.size()));
            if (admissible.isEmpty()) return admissible;
        }
    }
    return admissible;
}

void AlgorithmMc3::certify(LtiSystem * box, double gainDb)
{
    if (gainDb >= bestGainDb) {
        return;
    }
    const PointController point = centreOf(box, gainDb);
    if (!stability->isNominallyStable(point)) {
        return;
    }
    bestGainDb = gainDb;
    bestController = systemFromPoint(box, point);
    ++m_certificates;
}

bool AlgorithmMc3::solve()
{
    liveList = std::make_unique<OrderedList>(false, m_settings.search.maxLiveNodes);
    conversion = std::make_unique<NaturalIntervalExtension>();
    detector = std::make_unique<BoundaryViolationDetector>(m_settings.algorithms.conservativeBoundaryColumns);
    stability = std::make_unique<NominalStabilityChecker>(plant, omega, m_settings.stability);

    bestGainDb = kInfinity;
    bestController.reset();
    nominalPlantValues.clear();
    for (double o : *omega) {
        nominalPlantValues.push_back(plant->evaluate(o));
    }

    const Range gainRange = controller->gain().isUncertain() ? controller->gain().range()
                                                             : Range(controller->gain().nominal(), controller->gain().nominal());
    if (gainRange.min <= 0.0) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "The gain search range must be positive."));
    }
    RangeUnion gains = RangeUnion::of(toDb(gainRange.min), toDb(gainRange.max));
    liveList->insert(std::make_unique<Node>(gains.minimum(), controller->clone(), gains));

    while (true) {
        if (qftbx::cancellationAsked(m_cancellation)) {
            throw qftbx::Cancelled();
        }
        if (liveList->isEmpty()) {
            if (bestController != nullptr) {
                designedController = std::move(bestController);
                return true;
            }
            throw qftbx::InvalidInput(QFTBX_TR("Core", "No feasible solution exists in the given search box."));
        }

        std::unique_ptr<Node> node = liveList->takeFirstAs<Node>();
        LtiSystem * box = node->system();

        if (node->getIndex() >= bestGainDb - epsilon) {
            ++m_prunedByBound;
            depthAccounting.record(*box, infeasible);
            continue;
        }

        if (!node->sets) {
            node->sets = gainSetsOf(box);
            node->gains.intersectWith(complementOf(node->sets->forbidden));
        }
        const GainSets & sets = *node->sets;
        RangeUnion & admissible = node->gains;

        constexpr double kSliceDb = 20.0;
        while (!admissible.isEmpty()) {
            const double lb = admissible.minimum();
            RangeUnion slice = admissible;
            slice.intersectWith(lb, lb + kSliceDb);
            std::unique_ptr<LtiSystem> gained = withGains(box, slice);
            const bool unstable = stability->isBoxUnstable(gained.get(), *conversion);
            if (!unstable) {
                break;
            }
            ++m_unstableBoxes;
            admissible.intersectWith(lb + kSliceDb, kInfinity);
        }
        if (admissible.isEmpty()) {
            ++m_emptyGainSets;
            depthAccounting.record(*box, infeasible);
            continue;
        }
        const double lowerBound = admissible.minimum();
        if (lowerBound >= bestGainDb - epsilon) {
            ++m_prunedByBound;
            depthAccounting.record(*box, infeasible);
            continue;
        }

        if (lowerBound > node->getIndex() && !liveList->isEmpty()
                && lowerBound > liveList->first()->getIndex()) {
            node->setIndex(lowerBound);
            liveList->insert(std::move(node));
            continue;
        }

        RangeUnion certifiable = sets.certified;
        certifiable.intersectWith(admissible);
        const bool certified = !certifiable.isEmpty();
        if (certified) {
            certify(box, certifiable.minimum());
        }
        depthAccounting.record(*box, certified ? feasible : ambiguous);
        if (!certified) {
            ++m_ambiguousVisits;
        }

        if (sets.small) {
            std::vector<PointController> points{centreOf(box, lowerBound)};
            {
                std::vector<std::size_t> variable;
                std::vector<Parameter *> params;
                for (Parameter & q : box->numerator()) params.push_back(&q);
                for (Parameter & q : box->denominator()) params.push_back(&q);
                for (std::size_t k = 0; k < params.size(); ++k) if (params[k]->isUncertain()) variable.push_back(k);
                if (variable.size() <= 4) {
                    for (std::size_t mask = 0; mask < (std::size_t(1) << variable.size()); ++mask) {
                        PointController corner = points.front();
                        for (std::size_t b = 0; b < variable.size(); ++b) {
                            const std::size_t k = variable[b];
                            const Range r = params[k]->range();
                            const double v = (mask >> b) & 1 ? r.max : r.min;
                            if (k < box->numerator().size()) corner.zeros[k] = v; else corner.poles[k - box->numerator().size()] = v;
                        }
                        points.push_back(corner);
                    }
                }
            }
            double bestCandidate = kInfinity;
            PointController bestPoint;
            for (const PointController & q : points) {
                RangeUnion admissibleHere = pointGainSet(q);
                admissibleHere.intersectWith(lowerBound, lowerBound + epsilon);
                if (admissibleHere.isEmpty()) continue;
                const double g = admissibleHere.minimum() + 1e-9;
                if (g >= bestCandidate) continue;
                PointController candidate = q;
                candidate.gain = fromDb(g);
                if (satisfiesBoundaries(candidate, omega, conversion.get(), detector.get(), boundaries, nominalPlantValues)
                        && stability->isNominallyStable(candidate)) {
                    bestCandidate = g;
                    bestPoint = candidate;
                }
            }
            if (bestCandidate < kInfinity) {
                designedController = systemFromPoint(box, bestPoint);
                return true;
            }
            ++m_smallDropped;
            continue;
        }

        if (certified && lowerBound >= certifiable.minimum() - epsilon) {
            continue;
        }

        auto halves = bisect(withGains(box, admissible).get());
        liveList->insert(std::make_unique<Node>(lowerBound, std::move(halves.first), admissible));
        liveList->insert(std::make_unique<Node>(lowerBound, std::move(halves.second), admissible));
    }
}

std::unique_ptr<LtiSystem> AlgorithmMc3::controllerStructure()
{
    return std::move(designedController);
}

LoopShapingStatistics AlgorithmMc3::statistics() const
{
    LoopShapingStatistics statistics;
    if (liveList) {
        statistics.peakLiveNodes = liveList->peakSize();
        statistics.nodesProcessed = liveList->takenCount();
    }
    statistics.boxesClassified = m_projections;
    statistics.boxesFeasible = m_certificates;
    statistics.boxesInfeasible = m_emptyGainSets + m_prunedByBound;
    statistics.boxesAmbiguous = m_ambiguousVisits + m_smallDropped;
    statistics.boxesAmbiguous = m_ambiguousVisits;
    if (stability) {
        statistics.stabilityVerdicts = stability->statistics().verdicts;
        statistics.stabilityProfiles = stability->statistics().profilesComputed;
    }
    depthAccounting.fill(statistics);
    return statistics;
}

}
