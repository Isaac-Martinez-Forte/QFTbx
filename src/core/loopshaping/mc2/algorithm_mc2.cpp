/**
 * @file
 * @brief Algorithm MC2: the thesis algorithm with the corrections documented in its class.
 *
 * The loop is that of section 5.4 without the execution stages: a live list
 * ordered by ascending gain infimum, the prune variable C with a strict
 * comparison, the cutting and bisection strategies of the pseudocode, and
 * the certified solution of MG returned when the space is exhausted. The
 * controller parameters are viewed as the thesis vector x, gain first. The
 * gain of a returned corner is whatever the anti-blocking rule chooses.
 * Termination, the verified corner and cancellation are as in NT.
 */

#include <vector>
#include "src/core/math/constants.h"
#include <cstdint>
#include "src/core/common/exception.h"
#include "src/core/loopshaping/mc2/algorithm_mc2.h"

#include "src/core/math/range_union.h"

namespace quick_solution = qftbx::quick_solution;

namespace qftbx {

namespace {

void cornerVectors(LtiSystem * box, bool zerosAtSup, bool polesAtSup,
                   std::vector<double> & zeros, std::vector<double> & poles)
{
    zeros.clear();
    poles.clear();

    for (Parameter & var : box->numerator()) {
        zeros.push_back(!var.isUncertain() ? var.nominal()
                        : (zerosAtSup ? var.range().max : var.range().min));
    }
    for (Parameter & var : box->denominator()) {
        poles.push_back(!var.isUncertain() ? var.nominal()
                        : (polesAtSup ? var.range().max : var.range().min));
    }
}

}

void AlgorithmMc2::setSettings(const qftbx::Settings & settings)
{
    m_settings = settings;
    const Settings::Algorithms::McStrategies & mc = settings.algorithms.mc;
    strategies.infeasibleMagnitude = mc.infeasibleMagnitude;
    strategies.infeasiblePhase = mc.infeasiblePhase;
    strategies.feasibleMagnitude = mc.feasibleMagnitude;
    strategies.feasiblePhase = mc.feasiblePhase;
    strategies.bestGain = mc.bestGain;
    strategies.treeBisection = mc.treeBisection;
}

void AlgorithmMc2::setStrategies(const Strategies & s)
{
    strategies = s;
}

void AlgorithmMc2::setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega,
                                  const BoundaryData * boundaries, double epsilon)
{
    this->plant = plant;
    this->controller = controller->clone();
    depthAccounting.start(*this->controller);
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;

    phaseSpanWidth = boundaries->phaseRange().width();
    phaseGridStep = phaseSpanWidth / (boundaries->phaseCount() - 1);

    hasUncertainZeros = false;
    for (Parameter & var : this->controller->numerator()) {
        hasUncertainZeros = hasUncertainZeros || var.isUncertain();
    }

    hasUncertainPoles = false;
    for (Parameter & var : this->controller->denominator()) {
        hasUncertainPoles = hasUncertainPoles || var.isUncertain();
    }
}

inline std::int32_t AlgorithmMc2::parameterCount(LtiSystem * box) const
{
    return static_cast<std::int32_t>(1 + box->numerator().size() + box->denominator().size());
}

Range AlgorithmMc2::parameterRange(LtiSystem * box, std::int32_t parameter) const
{
    Parameter & var = parameter == 0
            ? box->gain()
            : (parameter <= static_cast<std::int32_t>(box->numerator().size())
                   ? box->numerator()[static_cast<std::size_t>(parameter - 1)]
                   : box->denominator()[static_cast<std::size_t>(parameter - 1)
                                        - box->numerator().size()]);

    return var.isUncertain() ? var.range()
                             : Range(var.nominal(), var.nominal());
}

std::unique_ptr<LtiSystem> AlgorithmMc2::replaceParameter(LtiSystem * box, std::int32_t parameter,
                                                       Range range) const
{
    std::vector<Parameter> numerator;
    numerator.reserve(box->numerator().size());
    for (std::size_t j = 0; j < box->numerator().size(); ++j) {
        Parameter & old = box->numerator()[j];
        numerator.push_back(static_cast<std::size_t>(parameter) == j + 1
                ? Parameter(old.name(), range, range.min)
                : old);
    }

    std::vector<Parameter> denominator;
    denominator.reserve(box->denominator().size());
    for (std::size_t j = 0; j < box->denominator().size(); ++j) {
        Parameter & old = box->denominator()[j];
        denominator.push_back(static_cast<std::size_t>(parameter) == j + 1 + box->numerator().size()
                ? Parameter(old.name(), range, range.min)
                : old);
    }

    Parameter gain = parameter == 0
            ? Parameter(box->gain().name(), range, range.min)
            : box->gain();

    return box->create(box->name(), std::move(numerator), std::move(denominator),
                       std::move(gain), box->delay());
}

bool AlgorithmMc2::solve()
{
    liveList = std::make_unique<OrderedList>(false, m_settings.search.maxLiveNodes);
    conversion = std::make_unique<NaturalIntervalExtension>();
    detector = std::make_unique<BoundaryViolationDetector>(m_settings.algorithms.conservativeBoundaryColumns);
    stability = std::make_unique<NominalStabilityChecker>(plant, omega, m_settings.stability);

    bestCertifiedGain = std::numeric_limits<double>::infinity();
    bestCertifiedController.reset();

    nominalPlantValues.clear();

    for (double o : *omega) {
        std::complex<double> c = plant->evaluate(o);
        nominalPlantValues.push_back(c);
    }

    if (!hasUncertainZeros && !hasUncertainPoles && !controller->gain().isUncertain()) {
        designedController = pointFromBox(controller.get(), true);
        return false;
    }

    const double initialGainInf = controller->gain().range().min;

    auto initial = std::make_unique<McSearchNode>(initialGainInf, std::move(controller),
                                                 ambiguous);
    initial->setCutsEnabled(true);
    liveList->insert(std::move(initial));

    while (true) {

        if (qftbx::cancellationAsked(m_cancellation)) {
            throw qftbx::Cancelled();
        }

        if (liveList->isEmpty()) {
            if (bestCertifiedController != nullptr) {
                designedController = std::move(bestCertifiedController);
                return true;
            }

            throw qftbx::InvalidInput(QFTBX_TR("Core", "No feasible solution exists in the given search box."));
        }

        std::unique_ptr<McSearchNode> node = liveList->takeFirstAs<McSearchNode>();

        if (bestCertifiedGain < node->system()->gain().range().min) {
            continue;
        }

        node->setSystem(capGain(node->releaseSystem(), bestCertifiedGain));

        if (node->flag() == feasible) {
            designedController = pointFromBox(node->system(), true);
            return true;
        }

        NodeAnalysis analysis;
        if (!analyse(node.get(), analysis)) {
            depthAccounting.record(*node->system(), infeasible);
            continue;
        }
        depthAccounting.record(*node->system(), analysis.flag == feasible ? feasible : ambiguous);
        for (std::size_t i = 0; i < analysis.classification.size(); ++i) {
            if (analysis.classification[i].has_value() && analysis.classification[i]->flag() == ambiguous) {
                depthAccounting.ambiguousAt(i);
            }
        }

        if (analysis.flag == feasible) {
            const PointController corner = cornerOf(node->system(), true);

            if (!stability->isNominallyStable(corner)) {
                continue;
            }

            designedController = systemFromPoint(node->system(), corner);
            return true;
        }

        if (isEpsilonSmall(node.get(), analysis)) {
            const std::optional<PointController> corner = verifiedCorner(node->system(), omega,
                    conversion.get(), detector.get(), boundaries, nominalPlantValues);

            if (!corner || !stability->isNominallyStable(*corner)) {
                continue;
            }

            PointController best = *corner;
            const RangeUnion gains = admissibleGains(best.zeros, best.poles,
                                                     node->system()->gain().range());

            if (!gains.isEmpty()) {
                const double contracted = std::pow(10.0, gains.minimum() / 20.0);

                if (contracted < best.gain) {
                    PointController candidate{contracted, best.zeros, best.poles};

                    if (pointIsFeasible(candidate) && stability->isNominallyStable(candidate)) {
                        best = std::move(candidate);
                    }
                }
            }

            designedController = systemFromPoint(node->system(), best);
            return true;
        }

        if (stability->isBoxUnstable(node->system(), *conversion)) {
            continue;
        }

        std::vector<FeasibleThreshold> thresholds;
        improveNode(node.get(), analysis, thresholds);

        if (bestCertifiedGain < node->system()->gain().range().min) {
            continue;
        }

        qftbx::McBisectionResult children = bisect(node.get(), analysis, thresholds);

        for (std::unique_ptr<McSearchNode> * slot : {&children.t1, &children.t2}) {
            std::unique_ptr<McSearchNode> child = std::move(*slot);

            if (child == nullptr) {
                continue;
            }

            if (bestCertifiedGain < child->system()->gain().range().min) {
                continue;
            }

            child->setIndex(child->system()->gain().range().min);
            liveList->insert(std::move(child));
        }
    }
}

std::size_t AlgorithmMc2::peakLiveNodes() const
{
    return liveList != nullptr ? liveList->peakSize() : 0;
}

LoopShapingStatistics AlgorithmMc2::statistics() const
{
    LoopShapingStatistics statistics;
    if (liveList != nullptr) {
        statistics.peakLiveNodes = liveList->peakSize();
        statistics.nodesProcessed = liveList->takenCount();
    }
    if (detector != nullptr) {
        statistics.boxesClassified = detector->classifications();
        statistics.boxesFeasible = detector->feasibleBoxes();
        statistics.boxesInfeasible = detector->infeasibleBoxes();
        statistics.boxesAmbiguous = detector->ambiguousBoxes();
    }
    depthAccounting.fill(statistics);
    if (stability != nullptr) {
        statistics.stabilityVerdicts = stability->statistics().verdicts;
        statistics.stabilityProfiles = stability->statistics().profilesComputed;
    }
    return statistics;
}

std::unique_ptr<LtiSystem> AlgorithmMc2::controllerStructure()
{
    return std::move(designedController);
}

bool AlgorithmMc2::analyse(McSearchNode * node, NodeAnalysis & out)
{
    out.flag = feasible;
    out.mainFrequency = 0;

    double largestArea = std::numeric_limits<double>::lowest();

    for (std::size_t i = 0; i < omega->size(); ++i) {

        if (node->isFrequencyFeasible(i)) {
            out.classification.push_back(std::nullopt);
            out.projection.push_back(std::nullopt);
            out.boxMag.push_back(Range());
            out.boxPhase.push_back(Range());
            continue;
        }

        const NicholsBox projection = conversion->nicholsBox(node->system(), omega->at(i),
                                                      nominalPlantValues.at(i));

        BoxClassification classification = detector->classifyBox(projection, boundaries, i);

        const BoxFlag verdict = classification.flag();

        if (verdict == infeasible) {
            return false;
        }

        out.classification.push_back(std::move(classification));
        out.projection.push_back(projection);
        out.boxMag.push_back(Range(projection.magnitudeDb.lower(), projection.magnitudeDb.upper()));
        out.boxPhase.push_back(Range(projection.phaseDegrees.lower(), projection.phaseDegrees.upper()));

        const double phaseWidth = projection.phaseDegrees.width();

        if (verdict == ambiguous) {
            out.flag = ambiguous;

            const double area = projection.magnitudeDb.width() * phaseWidth;
            if (area > largestArea) {
                largestArea = area;
                out.mainFrequency = i;
            }
        }
    }

    return true;
}

void AlgorithmMc2::improveNode(McSearchNode * node, NodeAnalysis & analysis,
                                           std::vector<FeasibleThreshold> & thresholds)
{
    if (!node->cutsEnabled()) {
        return;
    }

    bool improved = false;

    if (strategies.bestGain && bestGainSearch(node)) {
        improved = true;
    } else if (strategies.feasibleMagnitude || strategies.feasiblePhase) {
        feasibleCuts(node, analysis, thresholds, improved);
    }

    if (strategies.infeasibleMagnitude || strategies.infeasiblePhase) {
        infeasibleCuts(node, analysis, improved);
    }
}

bool AlgorithmMc2::boxIsFeasibleAt(LtiSystem * box, std::size_t freqIndex)
{
    const NicholsBox projection = conversion->nicholsBox(box, omega->at(freqIndex),
                                                  nominalPlantValues.at(freqIndex));
    return detector->classifyBox(projection, boundaries, freqIndex).flag() == feasible;
}

bool AlgorithmMc2::isEpsilonSmall(McSearchNode * node, const NodeAnalysis & analysis)
{
    for (std::size_t i = 0; i < omega->size(); ++i) {
        const NicholsBox box = analysis.projection.at(i).has_value()
                ? *analysis.projection.at(i)
                : conversion->nicholsBox(node->system(), omega->at(i), nominalPlantValues.at(i));

        if ((box.magnitudeDb.width() >= epsilon) || (box.phaseDegrees.width() >= epsilon)) {
            return false;
        }
    }

    return true;
}

bool AlgorithmMc2::boxIsFeasible(LtiSystem * box)
{
    for (std::size_t i = 0; i < omega->size(); ++i) {
        if (!boxIsFeasibleAt(box, i)) {
            return false;
        }
    }

    return true;
}

bool AlgorithmMc2::pointIsFeasible(const PointController & point)
{
    for (std::size_t i = 0; i < omega->size(); ++i) {
        const NicholsBox projection = conversion->nicholsPoint(point, omega->at(i),
                                                              nominalPlantValues.at(i));

        if (detector->classifyBox(projection, boundaries, i).flag() != feasible) {
            return false;
        }
    }

    return true;
}

RangeUnion AlgorithmMc2::admissibleGains(const std::vector<double> & zeros,
                                        const std::vector<double> & poles, Range gainRange)
{
    RangeUnion gains = RangeUnion::of(20.0 * std::log10(gainRange.min),
                                      20.0 * std::log10(gainRange.max));

    for (std::size_t i = 0; i < omega->size() && !gains.isEmpty(); ++i) {

        const NicholsBox at = conversion->nicholsPoint(1.0, zeros, poles,
                                                       omega->at(i), nominalPlantValues.at(i));
        const double mu = at.magnitudeDb.lower();

        const BoundaryColumns & columns = boundaries->columns(i);
        const double phase = at.phaseDegrees.lower();
        const BoundaryColumns::Intervals below = columns.intervals(
                    detector->conservative() ? columns.firstColumnCovering(phase) : columns.columnOf(phase));

        RangeUnion column = RangeUnion::of(below.lo, below.hi, static_cast<std::size_t>(below.count));
        if (detector->conservative()) {
            const BoundaryColumns::Intervals above = columns.intervals(columns.lastColumnCovering(phase));
            column.intersectWith(RangeUnion::of(above.lo, above.hi, static_cast<std::size_t>(above.count)));
        }
        column.shiftBy(-mu);
        gains.intersectWith(column);
    }

    return gains;
}

bool AlgorithmMc2::bestGainSearch(McSearchNode * node)
{
    LtiSystem * box = node->system();

    if (!box->gain().isUncertain()) {
        return false;
    }

    std::vector<double> zeroSups, poleInfs;
    cornerVectors(box, true, false, zeroSups, poleInfs);

    const RangeUnion gains = admissibleGains(zeroSups, poleInfs, box->gain().range());

    if (gains.isEmpty()) {
        return false;
    }

    const double gain = std::pow(10.0, gains.minimum() / 20.0);

    if (gain >= bestCertifiedGain) {
        return false;
    }

    const PointController point{gain, std::move(zeroSups), std::move(poleInfs)};

    if (!pointIsFeasible(point) || !stability->isNominallyStable(point)) {
        return false;
    }

    bestCertifiedGain = gain;
    bestCertifiedController = systemFromPoint(box, point);

    return true;
}

void AlgorithmMc2::insertFeasibleBox(std::unique_ptr<LtiSystem> box)
{
    const double gainInf = box->gain().range().min;

    if (gainInf > bestCertifiedGain) {
        return;
    }

    const PointController point = cornerOf(box.get(), true);

    if (!stability->isNominallyStable(point)) {
        return;
    }

    if (gainInf < bestCertifiedGain) {
        bestCertifiedGain = gainInf;
        bestCertifiedController = systemFromPoint(box.get(), point);
    }

    auto t = std::make_unique<McSearchNode>(gainInf, std::move(box), feasible);
    t->setCutsEnabled(false);
    liveList->insert(std::move(t));
}

void AlgorithmMc2::feasibleCuts(McSearchNode * node, const NodeAnalysis & analysis,
                                            std::vector<FeasibleThreshold> & thresholds, bool & improved)
{
    LtiSystem * box = node->system();
    const std::int32_t total = parameterCount(box);

    for (std::int32_t parameter = 0; parameter < total; ++parameter) {

        const Range range = parameterRange(box, parameter);

        if (range.min >= range.max) {
            continue;
        }

        const bool isGain = parameter == 0;
        const bool isZero = !isGain && parameter <= static_cast<std::int32_t>(box->numerator().size());
            const std::int32_t termIndex = isGain
            ? -1
            : (isZero ? parameter - 1
                      : parameter - 1 - static_cast<std::int32_t>(box->numerator().size()));

        for (std::int32_t family = 0; family < 2; ++family) {

            if (family == 0 && !strategies.feasibleMagnitude) {
                continue;
            }

            if (family == 1 && (isGain || !strategies.feasiblePhase)) {
                continue;
            }

            for (bool upperSide : {false, true}) {

                std::vector<double> zeroInfs, zeroSups, poleInfs, poleSups;
                cornerVectors(box, false, true, zeroInfs, poleSups);
                cornerVectors(box, true, false, zeroSups, poleInfs);
                const double kInf = box->gain().range().min;
                const double kSup = box->gain().range().max;

                double intersection = upperSide
                        ? std::numeric_limits<double>::lowest()
                        : std::numeric_limits<double>::max();
                bool allCertified = true;

                for (std::size_t i = 0; i < omega->size() && allCertified; ++i) {

                    const std::optional<BoxClassification> & classification =
                analysis.classification.at(i);

                    if (!classification.has_value() || classification->flag() != ambiguous) {
                        continue;
                    }

                    const double w = omega->at(i);
                    const std::complex<double> p0 = nominalPlantValues.at(i);

                    double t = -1.0;

                    if (family == 0) {
                        const double boundMin = std::pow(10.0, classification->extremes()[0] / 20.0);
                        const double boundMax = std::pow(10.0, classification->extremes()[1] / 20.0);

                        const bool topStrip = (isGain || isZero) ? upperSide : !upperSide;

                        if (topStrip) {
                            if (classification->isTopRightForbidden()) {
                                allCertified = false;
                                break;
                            }
                            if (isGain) {
                                t = quick_solution::gainCut(boundMax, zeroInfs, poleSups, w, p0);
                            } else if (isZero) {
                                t = quick_solution::zeroCut(boundMax, kInf, zeroInfs, poleSups, termIndex, w, p0);
                            } else {
                                t = quick_solution::poleCut(boundMax, kInf, zeroInfs, poleSups, termIndex, w, p0);
                            }
                        } else {
                            if (classification->isBottomLeftForbidden()) {
                                allCertified = false;
                                break;
                            }
                            if (isGain) {
                                t = quick_solution::gainCut(boundMin, zeroSups, poleInfs, w, p0);
                            } else if (isZero) {
                                t = quick_solution::zeroCut(boundMin, kSup, zeroSups, poleInfs, termIndex, w, p0);
                            } else {
                                t = quick_solution::poleCut(boundMin, kSup, zeroSups, poleInfs, termIndex, w, p0);
                            }
                        }
                    } else {
                        const double phi0 = nominalPhase(p0);
                        const double thetaMin = classification->extremes()[2] * qftbx::math::kPi / 180.0;
                        const double thetaMax = classification->extremes()[3] * qftbx::math::kPi / 180.0;
                        const Range boxPhase = analysis.boxPhase.at(i);

                        const bool rightStrip = isZero ? !upperSide : upperSide;

                        if (rightStrip) {
                            if (classification->isTopRightForbidden() ||
                                    classification->extremes()[3] >= boxPhase.max - phaseGridStep) {
                                allCertified = false;
                                break;
                            }
                            t = isZero
                                ? quick_solution::zeroPhaseCutHigh(thetaMax, phi0, zeroSups, poleInfs, termIndex, w)
                                : quick_solution::polePhaseCutHigh(thetaMax, phi0, zeroSups, poleInfs, termIndex, w);
                        } else {
                            if (classification->isBottomLeftForbidden() ||
                                    classification->extremes()[2] <= boxPhase.min + phaseGridStep) {
                                allCertified = false;
                                break;
                            }
                            t = isZero
                                ? quick_solution::zeroPhaseCutLow(thetaMin, phi0, zeroInfs, poleSups, termIndex, w)
                                : quick_solution::polePhaseCutLow(thetaMin, phi0, zeroInfs, poleSups, termIndex, w);
                        }
                    }

                    if (t < 0.0) {
                        allCertified = false;
                        break;
                    }

                    if (upperSide) {
                        if (t >= range.max) {
                            allCertified = false;
                            break;
                        }
                        const double clamped = std::max(t, range.min);
                        intersection = std::max(intersection, clamped);

                        if (t > range.min) {
                            thresholds.push_back({parameter, i, t, true,
                                               (range.max - t) / range.width()});
                        }
                    } else {
                        if (t <= range.min) {
                            allCertified = false;
                            break;
                        }
                        const double clamped = std::min(t, range.max);
                        intersection = std::min(intersection, clamped);

                        if (t < range.max) {
                            thresholds.push_back({parameter, i, t, false,
                                               (t - range.min) / range.width()});
                        }
                    }
                }

                if (!allCertified) {
                    continue;
                }

                if (intersection <= range.min || intersection >= range.max) {
                    continue;
                }

                const Range feasiblePart = upperSide
                        ? Range(intersection, range.max)
                        : Range(range.min, intersection);
                const Range ambiguousPart = upperSide
                        ? Range(range.min, intersection)
                        : Range(intersection, range.max);

                std::unique_ptr<LtiSystem> um = replaceParameter(box, parameter, feasiblePart);

                if (!boxIsFeasible(um.get())) {
                    continue;
                }

                insertFeasibleBox(std::move(um));

                std::unique_ptr<LtiSystem> remainder = replaceParameter(box, parameter,
                                                                       ambiguousPart);
                box = remainder.get();
                node->setSystem(std::move(remainder));

                improved = true;
            }
        }
    }
}

void AlgorithmMc2::infeasibleCuts(McSearchNode * node, const NodeAnalysis & analysis,
                                       bool & improved)
{
    LtiSystem * v = node->system();

    ParameterBounds bounds = boundsOf(v);

    bool cut = false;

    for (std::size_t i = 0; i < omega->size(); ++i) {

        const std::optional<BoxClassification> & classification =
                analysis.classification.at(i);

        if (!classification.has_value() || classification->flag() != ambiguous) {
            continue;
        }

        const double w = omega->at(i);
        const std::complex<double> p0 = nominalPlantValues.at(i);
        const double boundMin = std::pow(10.0, classification->extremes()[0] / 20.0);
        const double boundMax = std::pow(10.0, classification->extremes()[1] / 20.0);

        if (strategies.infeasibleMagnitude && classification->isBottomLeftForbidden()) {
            cut = cutBelowBoundary(bounds, boundMin, w, p0) || cut;
        }

        if (strategies.infeasibleMagnitude && classification->isTopRightForbidden()) {
            cut = cutAboveBoundary(bounds, boundMax, w, p0) || cut;
        }

        if (strategies.infeasiblePhase) {

            const double phi0 = nominalPhase(p0);
            const Range boxPhase = analysis.boxPhase.at(i);
            const double boundPhaseMin = classification->extremes()[2];
            const double boundPhaseMax = classification->extremes()[3];

            if (classification->isTopRightForbidden() && boundPhaseMax < boxPhase.max - phaseGridStep) {
                cut = cutRightOfPhase(bounds, boundPhaseMax * qftbx::math::kPi / 180.0, phi0, w) || cut;
            }

            if (classification->isBottomLeftForbidden() && boundPhaseMin > boxPhase.min + phaseGridStep) {
                cut = cutLeftOfPhase(bounds, boundPhaseMin * qftbx::math::kPi / 180.0, phi0, w) || cut;
            }
        }
    }

    if (!cut) {
        return;
    }

    node->setSystem(boxFromBounds(v, bounds));
    improved = true;
}

qftbx::McBisectionResult AlgorithmMc2::bisectAt(McSearchNode * node, std::int32_t parameter,
                                                         double point)
{
    LtiSystem * box = node->system();
    const Range range = parameterRange(box, parameter);

    std::unique_ptr<LtiSystem> lower = replaceParameter(box, parameter,
                                                       Range(range.min, point));
    std::unique_ptr<LtiSystem> upper = replaceParameter(box, parameter,
                                                       Range(point, range.max));

    const auto makeChild = [&](std::unique_ptr<LtiSystem> system) {
        const double gainInf = system->gain().range().min;

        auto t = std::make_unique<McSearchNode>(gainInf, std::move(system), ambiguous);
        t->setCutsEnabled(node->cutsEnabled());
        t->setFeasibleFrequencies(node->feasibleFrequencies());
        return t;
    };

    qftbx::McBisectionResult children;
    children.t1 = makeChild(std::move(lower));
    children.t2 = makeChild(std::move(upper));

    return children;
}

inline std::int32_t AlgorithmMc2::widestByMeasure(McSearchNode * node, std::size_t mainFrequency, int measure)
{
    LtiSystem * box = node->system();
    const double w = omega->at(mainFrequency);
    const std::complex<double> p0 = nominalPlantValues.at(mainFrequency);

    std::int32_t best = -1;
    double bestValue = -1.0;

    const auto consider = [&](std::int32_t parameter, const NicholsBox & term, bool gainTerm) {
        double value;

        if (measure == 2) {
            if (gainTerm) {
                return;
            }
            value = term.phaseDegrees.width();
        } else if (measure == 1 || gainTerm) {
            value = term.magnitudeDb.width();
        } else {
            value = term.magnitudeDb.width() * term.phaseDegrees.width();
        }

        if (value > bestValue) {
            bestValue = value;
            best = parameter;
        }
    };

    if (box->gain().isUncertain()) {
        consider(0, conversion->gainTermBox(box->gain(), p0), true);
    }

    for (std::size_t j = 0; j < box->numerator().size(); ++j) {
        if (box->numerator()[j].isUncertain()) {
            consider(j + 1, conversion->numeratorTermBox(box->numerator()[j], w, p0), false);
        }
    }

    for (std::size_t j = 0; j < box->denominator().size(); ++j) {
        if (box->denominator()[j].isUncertain()) {
            consider(j + 1 + box->numerator().size(),
                     conversion->denominatorTermBox(box->denominator()[j], w, p0), false);
        }
    }

    return best;
}

qftbx::McBisectionResult AlgorithmMc2::bisect(McSearchNode * node, const NodeAnalysis & analysis,
                                                       const std::vector<FeasibleThreshold> & thresholds)
{
    if (strategies.treeBisection && !thresholds.empty()) {

        const FeasibleThreshold * bestThreshold = nullptr;
        double bestFraction = 0.0;

        for (const FeasibleThreshold & t : thresholds) {
            const Range range = parameterRange(node->system(), t.parameter);

            if (t.threshold <= range.min || t.threshold >= range.max) {
                continue;
            }

            const double fraction = t.upperSide
                    ? (range.max - t.threshold) / range.width()
                    : (t.threshold - range.min) / range.width();

            if (fraction > bestFraction) {
                bestFraction = fraction;
                bestThreshold = &t;
            }
        }

        if (bestThreshold != nullptr) {
            const std::size_t freq = bestThreshold->freqIndex;
            qftbx::McBisectionResult children = bisectAt(node, bestThreshold->parameter,
                                                      bestThreshold->threshold);
            McSearchNode * feasibleChild = (bestThreshold->upperSide
                    ? children.t2 : children.t1).get();

            if (boxIsFeasibleAt(feasibleChild->system(), freq)) {
                feasibleChild->markFrequencyFeasible(freq, omega->at(freq));
            }

            return children;
        }
    }

    const Range magnitude = analysis.boxMag.at(analysis.mainFrequency);
    const Range phase = analysis.boxPhase.at(analysis.mainFrequency);
    const int measure = phase.width() > magnitude.width() ? 2 : 1;

    std::int32_t parameter = widestByMeasure(node, analysis.mainFrequency, measure);

    if (parameter < 0) {
        parameter = widestByMeasure(node, analysis.mainFrequency, 0);
    }

    const Range range = parameterRange(node->system(), parameter);

    return bisectAt(node, parameter, range.middle());
}

}
