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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "src/core/common/exception.h"
#include "src/core/loopshaping/mc2/algorithm_mc2.h"
#include "src/core/math/constants.h"

namespace qftbx {

namespace {

enum class CutKind { Magnitude, Phase };

Range magnitudeRangeOf(const NicholsBox & projection)
{
    return Range(projection.magnitudeDb.lower(), projection.magnitudeDb.upper());
}

Range phaseRangeOf(const NicholsBox & projection)
{
    return Range(projection.phaseDegrees.lower(), projection.phaseDegrees.upper());
}

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
    strategies = settings.research.mc;
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

    phaseGridStep = boundaries->phaseRange().width() / (boundaries->phaseCount() - 1);
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

std::unique_ptr<LtiSystem> AlgorithmMc2::replaceParameter(LtiSystem * box, std::int32_t parameter, Range range) const
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

void AlgorithmMc2::prepare()
{
    liveList = std::make_unique<OrderedList>(false, m_settings.search.maxLiveNodes);
    conversion = std::make_unique<NaturalIntervalExtension>();
    detector = std::make_unique<BoundaryViolationDetector>(m_settings.research.conservativeColumnsInForce());
    stability = std::make_unique<NominalStabilityChecker>(plant, omega, m_settings.stability);
    family = std::make_unique<FamilyStabilityChecker>(plant, controller.get(),
                                                     m_settings.research.familyGate ? m_sweep : ParameterGrids());

    exact.reset();
    certifier.reset();
    if (m_settings.research.mc2Reading == Settings::Research::PointReading::Exact
            && m_templates != nullptr && m_specifications != nullptr) {
        exact = std::make_unique<ExactPointCheck>(*plant, controller.get(), *omega, *m_templates, *m_specifications);
        if (exact->usable()) {
            certifier = std::make_unique<Certifier>(*exact, *stability, *family);
        } else {
            exact.reset();
        }
    }

    exactReading = certifier != nullptr;

    certificate = LoopShapingStatistics::Certificate();
    certificate.exactPoints = exactReading;
    residueGainInf = std::numeric_limits<double>::infinity();
    unprovenGainInf = std::numeric_limits<double>::infinity();
    gridBackedGainInf = std::numeric_limits<double>::infinity();
    resolvedGainInf = std::numeric_limits<double>::infinity();

    bestCertifiedGain = std::numeric_limits<double>::infinity();
    bestCertifiedController.reset();

    nominalPlantValues.clear();
    for (double o : *omega) {
        nominalPlantValues.push_back(plant->evaluate(o));
    }
}

bool AlgorithmMc2::solve()
{
    prepare();

    bool uncertain = controller->gain().isUncertain();
    for (Parameter & z : controller->numerator()) {
        uncertain = uncertain || z.isUncertain();
    }
    for (Parameter & p : controller->denominator()) {
        uncertain = uncertain || p.isUncertain();
    }
    if (!uncertain) {
        designedController = pointFromBox(controller.get(), true);
        return false;
    }

    initialGainRange = controller->gain().range();
    liveList->insert(std::make_unique<McSearchNode>(initialGainRange.min, std::move(controller), ambiguous));

    while (true) {

        if (qftbx::cancellationAsked(m_cancellation)) {
            throw qftbx::Cancelled();
        }

        if (liveList->isEmpty()) {
            return concludeEmptyList();
        }

        std::unique_ptr<McSearchNode> node = liveList->takeFirstAs<McSearchNode>();

        if (cannotImprove(node->system()->gain().range().min)) {
            continue;
        }

        node->setSystem(capGain(node->releaseSystem(), bestCertifiedGain));
        const double gainInf = node->system()->gain().range().min;

        if (exactReading && family->isBoxUnstable(node->system())) {
            continue;
        }

        Step step = resolveFeasibleHead(*node);
        if (step == Step::Designed) {
            return true;
        }

        NodeAnalysis analysis;
        if (!analyseOrDiscard(*node, analysis, gainInf)) {
            continue;
        }

        if (analysis.flag == feasible && !node->cornerVerdict().has_value()) {
            step = resolveFeasibleCorner(*node, gainInf);
            if (step == Step::Designed) {
                return true;
            }
            if (step == Step::Next) {
                continue;
            }
        }

        if (isEpsilonSmall(node.get(), analysis)) {
            if (resolveEpsilonBox(*node, gainInf) == Step::Designed) {
                return true;
            }
            continue;
        }

        if (pruneUnstableBox(*node, gainInf)) {
            continue;
        }

        expand(*node, analysis);
    }
}

bool AlgorithmMc2::concludeEmptyList()
{
    closeCertificate();

    if (bestCertifiedController != nullptr) {
        designedController = std::move(bestCertifiedController);
        return true;
    }

    if (certificate.residueNodes == 0 && certificate.unprovenDiscards == 0 && certificate.gridBackedPrunes == 0) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "No feasible solution exists in the given search box."));
    }

    throw qftbx::InvalidInput(QFTBX_TR("Core", "The search found no design and cannot prove that none exists: %1 boxes were discarded on the boundary columns or on the nominal stability of their enclosure alone, and %2 were left without a certified point. A design, if there is one, needs a gain of at least %3.")
                              .arg(certificate.unprovenDiscards + certificate.gridBackedPrunes)
                              .arg(certificate.residueNodes)
                              .arg(certificate.lowerBoundStrict));
}

AlgorithmMc2::Step AlgorithmMc2::returnDesign(std::unique_ptr<LtiSystem> design)
{
    designedController = std::move(design);
    closeCertificate();
    return Step::Designed;
}

AlgorithmMc2::Step AlgorithmMc2::resolveFeasibleHead(McSearchNode & node)
{
    if (exactReading || node.flag() != feasible) {
        return Step::Carry;
    }

    if (!node.cornerVerdict().has_value()) {
        node.setCornerVerdict(family->isStable(cornerOf(node.system(), true)));
    }
    if (*node.cornerVerdict()) {
        return returnDesign(pointFromBox(node.system(), true));
    }
    return Step::Carry;
}

bool AlgorithmMc2::analyseOrDiscard(McSearchNode & node, NodeAnalysis & analysis, double gainInf)
{
    if (!analyse(&node, analysis)) {
        depthAccounting.record(*node.system(), infeasible);
        if (exactReading) {
            ++certificate.provenInfeasible;
        } else {
            discardUnproven(gainInf);
        }
        return false;
    }

    depthAccounting.record(*node.system(), analysis.flag == feasible ? feasible : ambiguous);
    for (std::size_t i = 0; i < analysis.classification.size(); ++i) {
        if (analysis.classification[i].has_value() && analysis.classification[i]->flag() == ambiguous) {
            depthAccounting.ambiguousAt(i);
        }
    }
    return true;
}

AlgorithmMc2::Step AlgorithmMc2::resolveFeasibleCorner(McSearchNode & node, double gainInf)
{
    const PointController corner = cornerOf(node.system(), true);

    if (exactReading) {
        if (certifier->certify(corner)) {
            adoptIncumbent(lowestGainOnRay(corner), node.system());
            return Step::Next;
        }
        return Step::Carry;
    }

    if (!stability->isNominallyStable(corner)) {
        dropToResidue(gainInf);
        return Step::Next;
    }
    if (family->isStable(corner)) {
        return returnDesign(systemFromPoint(node.system(), corner));
    }
    return Step::Carry;
}

AlgorithmMc2::Step AlgorithmMc2::resolveEpsilonBox(McSearchNode & node, double gainInf)
{
    if (exactReading) {
        const std::optional<PointController> candidate = bestEpsilonCandidate(node.system());
        if (!candidate) {
            dropToResidue(gainInf);
            return Step::Next;
        }
        adoptIncumbent(*candidate, node.system());
        resolvedAtEpsilon(gainInf);
        return Step::Next;
    }

    std::optional<PointController> corner = verifiedCorner(node.system(), omega, conversion.get(),
                                                           detector.get(), boundaries, nominalPlantValues);
    if (corner && (!stability->isNominallyStable(*corner) || !family->isStable(*corner))) {
        corner.reset();
    }
    if (!corner) {
        dropToResidue(gainInf);
        return Step::Next;
    }

    PointController best = *corner;
    const std::optional<double> contracted = lowestGain(best.zeros, best.poles, node.system()->gain().range());
    if (contracted.has_value() && *contracted < best.gain) {
        PointController candidate{*contracted, best.zeros, best.poles};
        if (accepts(candidate)) {
            best = std::move(candidate);
        }
    }

    return returnDesign(systemFromPoint(node.system(), best));
}

bool AlgorithmMc2::pruneUnstableBox(McSearchNode & node, double gainInf)
{
    if (!stability->isBoxUnstable(node.system(), *conversion)) {
        return false;
    }

    if (!exactReading || !family->isBoxUnstableAtNominal(node.system())) {
        discardGridBacked(gainInf);
    }
    return true;
}

void AlgorithmMc2::expand(McSearchNode & node, NodeAnalysis & analysis)
{
    std::vector<FeasibleThreshold> thresholds;
    improveNode(&node, analysis, thresholds);

    if (cannotImprove(node.system()->gain().range().min)) {
        return;
    }

    qftbx::McBisectionResult children = bisect(&node, analysis, thresholds);

    for (std::unique_ptr<McSearchNode> * slot : {&children.t1, &children.t2}) {
        std::unique_ptr<McSearchNode> child = std::move(*slot);

        if (cannotImprove(child->system()->gain().range().min)) {
            continue;
        }

        liveList->insert(std::move(child));
    }
}

void AlgorithmMc2::discardUnproven(double gainInf)
{
    ++certificate.unprovenDiscards;
    unprovenGainInf = std::min(unprovenGainInf, gainInf);
}

void AlgorithmMc2::discardGridBacked(double gainInf)
{
    ++certificate.gridBackedPrunes;
    gridBackedGainInf = std::min(gridBackedGainInf, gainInf);
}

void AlgorithmMc2::dropToResidue(double gainInf)
{
    ++certificate.residueNodes;
    residueGainInf = std::min(residueGainInf, gainInf);
}

void AlgorithmMc2::resolvedAtEpsilon(double gainInf)
{
    ++certificate.epsilonResolved;
    resolvedGainInf = std::min(resolvedGainInf, gainInf);
}

void AlgorithmMc2::adoptIncumbent(const PointController & design, LtiSystem * box)
{
    if (design.gain < bestCertifiedGain) {
        bestCertifiedGain = design.gain;
        bestCertifiedController = systemFromPoint(box, design);
        ++certificate.incumbentUpdates;
    }
}

void AlgorithmMc2::closeCertificate()
{
    const double head = liveList->isEmpty() ? std::numeric_limits<double>::infinity()
                                            : liveList->first()->getIndex();
    certificate.lowerBound = std::min({head, residueGainInf, unprovenGainInf, resolvedGainInf});
    certificate.lowerBoundStrict = std::min(certificate.lowerBound, gridBackedGainInf);
}

bool AlgorithmMc2::cannotImprove(double gainInf) const
{
    return exactReading ? gainInf >= bestCertifiedGain : gainInf > bestCertifiedGain;
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
    statistics.certificate = certificate;
    if (family != nullptr) {
        statistics.certificate.familyPrunes = family->statistics().boxPrunes + family->statistics().nominalBoxPrunes;
    }
    if (certifier != nullptr) {
        const Certifier::Statistics & funnel = certifier->statistics();
        statistics.certificate.certifications = funnel.certifications;
        statistics.certificate.refusedByRouth = funnel.refusedByRouth;
        statistics.certificate.refusedByNominalStability = funnel.refusedByNominalStability;
        statistics.certificate.refusedBySpecifications = funnel.refusedBySpecifications;
        statistics.certificate.refusedByRoots = funnel.refusedByRoots;
    }
    if (exact != nullptr) {
        const ExactPointCheck::Statistics & reading = exact->statistics();
        statistics.certificate.sectorVerdicts = reading.sectorVerdicts;
        statistics.certificate.kernelPasses = reading.kernelPasses;
        statistics.certificate.gainSearches = reading.gainSearches;
        statistics.certificate.exchangeRounds = reading.exchangeRounds;
        statistics.certificate.ladderSteps = reading.ladderSteps;
        statistics.certificate.largestWorkingSet = reading.largestWorkingSet;
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
            continue;
        }

        const NicholsBox projection = conversion->nicholsBox(node->system(), omega->at(i),
                                                      nominalPlantValues.at(i));

        BoxClassification classification = detector->classifyBox(projection, boundaries, i);

        if (classification.flag() == infeasible) {
            if (!exactReading) {
                return false;
            }
            const ExactPointCheck::SectorVerdict sector = exact->sectorVerdict(
                        i, phaseRangeOf(projection), magnitudeRangeOf(projection));
            if (sector.provablyInfeasible) {
                return false;
            }
            ++certificate.columnsOverruled;
            classification.setFlag(ambiguous);
        }

        const BoxFlag verdict = classification.flag();

        out.classification.push_back(std::move(classification));
        out.projection.push_back(projection);

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

    const bool bestGainFound = strategies.bestGain && bestGainSearch(node);
    if (!bestGainFound && (strategies.feasibleMagnitude || strategies.feasiblePhase)) {
        feasibleCuts(node, analysis, thresholds);
    }

    if (strategies.infeasibleMagnitude || strategies.infeasiblePhase) {
        infeasibleCuts(node, analysis);
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

RangeUnion AlgorithmMc2::columnGainsDb(const std::vector<double> & zeros,
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

    const std::optional<double> gain = lowestGain(zeroSups, poleInfs, box->gain().range());
    if (!gain.has_value() || *gain >= bestCertifiedGain) {
        return false;
    }

    const PointController point{*gain, std::move(zeroSups), std::move(poleInfs)};
    if (!accepts(point)) {
        return false;
    }

    adoptIncumbent(point, box);
    return true;
}

std::optional<double> AlgorithmMc2::lowestGain(const std::vector<double> & zeros, const std::vector<double> & poles,
                                               Range gainRange)
{
    if (exactReading) {
        return exact->lowestAdmissibleGain(zeros, poles, gainRange).gain;
    }

    const RangeUnion gains = columnGainsDb(zeros, poles, gainRange);
    if (gains.isEmpty()) {
        return std::nullopt;
    }
    return std::pow(10.0, gains.minimum() / 20.0);
}

bool AlgorithmMc2::accepts(const PointController & point)
{
    if (exactReading) {
        return certifier->certifyAdmitted(point);
    }

    return satisfiesBoundaries(point, omega, conversion.get(), detector.get(), boundaries, nominalPlantValues)
           && stability->isNominallyStable(point) && family->isStable(point);
}

void AlgorithmMc2::insertFeasibleBox(std::unique_ptr<LtiSystem> box)
{
    const double gainInf = box->gain().range().min;

    if (cannotImprove(gainInf)) {
        return;
    }

    const PointController point = cornerOf(box.get(), true);
    std::optional<bool> verdict;

    if (exactReading) {
        verdict = certifier->certify(point);
        if (*verdict) {
            adoptIncumbent(lowestGainOnRay(point), box.get());
        }
    } else {
        if (!stability->isNominallyStable(point)) {
            dropToResidue(gainInf);
            return;
        }

        if (gainInf < bestCertifiedGain) {
            verdict = family->isStable(point);
            if (*verdict) {
                adoptIncumbent(point, box.get());
            }
        }
    }

    auto t = std::make_unique<McSearchNode>(gainInf, std::move(box), feasible);
    t->setCutsEnabled(false);
    if (verdict.has_value()) {
        t->setCornerVerdict(*verdict);
    }
    liveList->insert(std::move(t));
}

PointController AlgorithmMc2::lowestGainOnRay(const PointController & point)
{
    const std::optional<double> gain = lowestGain(point.zeros, point.poles, Range(initialGainRange.min, point.gain));
    if (gain.has_value() && *gain < point.gain) {
        const PointController lowered{*gain, point.zeros, point.poles};
        if (accepts(lowered)) {
            return lowered;
        }
    }
    return point;
}

std::optional<PointController> AlgorithmMc2::bestEpsilonCandidate(LtiSystem * box)
{
    std::optional<PointController> best;
    std::vector<std::pair<std::vector<double>, std::vector<double>>> tried;

    forEachCandidate(box, [&](const PointController & vertex) {
        for (const auto & seen : tried) {
            if (seen.first == vertex.zeros && seen.second == vertex.poles) {
                return false;
            }
        }
        tried.emplace_back(vertex.zeros, vertex.poles);

        const double ceiling = std::min(best.has_value() ? best->gain : bestCertifiedGain, initialGainRange.max);
        const std::optional<double> gain = lowestGain(vertex.zeros, vertex.poles, Range(initialGainRange.min, ceiling));
        if (gain.has_value() && (!best.has_value() || *gain < best->gain)) {
            const PointController candidate{*gain, vertex.zeros, vertex.poles};
            if (accepts(candidate)) {
                best = candidate;
            }
        }
        return false;
    });

    return best;
}

void AlgorithmMc2::feasibleCuts(McSearchNode * node, const NodeAnalysis & analysis,
                                std::vector<FeasibleThreshold> & thresholds)
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
        const std::int32_t termIndex = isGain ? -1
                                     : (isZero ? parameter - 1
                                               : parameter - 1 - static_cast<std::int32_t>(box->numerator().size()));

        for (const CutKind kind : {CutKind::Magnitude, CutKind::Phase}) {

            if (kind == CutKind::Magnitude && !strategies.feasibleMagnitude) {
                continue;
            }

            if (kind == CutKind::Phase && (isGain || !strategies.feasiblePhase)) {
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

                for (std::size_t i = 0; i < omega->size(); ++i) {

                    const std::optional<BoxClassification> & classification = analysis.classification.at(i);

                    if (!classification.has_value() || classification->flag() != ambiguous) {
                        continue;
                    }

                    const double w = omega->at(i);
                    const std::complex<double> p0 = nominalPlantValues.at(i);

                    double t = -1.0;

                    if (kind == CutKind::Magnitude) {
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
                        const Range boxPhase = phaseRangeOf(*analysis.projection.at(i));

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
                            thresholds.push_back({parameter, i, t, true});
                        }
                    } else {
                        if (t <= range.min) {
                            allCertified = false;
                            break;
                        }
                        const double clamped = std::min(t, range.max);
                        intersection = std::min(intersection, clamped);

                        if (t < range.max) {
                            thresholds.push_back({parameter, i, t, false});
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
            }
        }
    }
}

void AlgorithmMc2::infeasibleCuts(McSearchNode * node, const NodeAnalysis & analysis)
{
    LtiSystem * v = node->system();

    ParameterBounds bounds = boundsOf(v);

    bool cut = false;

    for (std::size_t i = 0; i < omega->size(); ++i) {

        const std::optional<BoxClassification> & classification = analysis.classification.at(i);

        if (!classification.has_value() || classification->flag() != ambiguous) {
            continue;
        }

        const double w = omega->at(i);
        const std::complex<double> p0 = nominalPlantValues.at(i);

        if (exactReading) {
            if (!strategies.infeasibleMagnitude) {
                continue;
            }
            const NicholsBox & projection = *analysis.projection.at(i);
            const Range boxMag = magnitudeRangeOf(projection);
            const ExactPointCheck::SectorVerdict sector = exact->sectorVerdict(i, phaseRangeOf(projection), boxMag);
            if (sector.forbiddenBelowDb > boxMag.min && std::isfinite(sector.forbiddenBelowDb)) {
                cut = cutBelowBoundary(bounds, std::pow(10.0, sector.forbiddenBelowDb / 20.0), w, p0) || cut;
            }
            if (sector.forbiddenAboveDb < boxMag.max && std::isfinite(sector.forbiddenAboveDb)) {
                cut = cutAboveBoundary(bounds, std::pow(10.0, sector.forbiddenAboveDb / 20.0), w, p0) || cut;
            }
            continue;
        }

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
            const Range boxPhase = phaseRangeOf(*analysis.projection.at(i));
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

    if (exactReading) {
        ++certificate.certifiedCuts;
    } else {
        discardUnproven(v->gain().range().min);
    }
    node->setSystem(boxFromBounds(v, bounds));
}

qftbx::McBisectionResult AlgorithmMc2::bisectAt(McSearchNode * node, std::int32_t parameter, double point)
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
        t->inheritHistoryFrom(*node);
        return t;
    };

    qftbx::McBisectionResult children;
    children.t1 = makeChild(std::move(lower));
    children.t2 = makeChild(std::move(upper));

    return children;
}

inline std::int32_t AlgorithmMc2::widestByMeasure(McSearchNode * node, std::size_t mainFrequency,
                                                  WidthMeasure measure)
{
    LtiSystem * box = node->system();
    const double w = omega->at(mainFrequency);
    const std::complex<double> p0 = nominalPlantValues.at(mainFrequency);

    std::int32_t best = -1;
    double bestValue = -1.0;

    const auto consider = [&](std::int32_t parameter, const NicholsBox & term, bool gainTerm) {
        double value;

        if (measure == WidthMeasure::Phase) {
            if (gainTerm) {
                return;
            }
            value = term.phaseDegrees.width();
        } else if (measure == WidthMeasure::Magnitude || gainTerm) {
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
                feasibleChild->markFrequencyFeasible(freq);
            }

            return children;
        }
    }

    const std::optional<NicholsBox> & main = analysis.projection.at(analysis.mainFrequency);
    const Range magnitude = main.has_value() ? magnitudeRangeOf(*main) : Range();
    const Range phase = main.has_value() ? phaseRangeOf(*main) : Range();
    const WidthMeasure measure = phase.width() > magnitude.width() ? WidthMeasure::Phase : WidthMeasure::Magnitude;

    std::int32_t parameter = widestByMeasure(node, analysis.mainFrequency, measure);

    if (parameter < 0) {
        parameter = widestByMeasure(node, analysis.mainFrequency, WidthMeasure::Area);
    }

    const Range range = parameterRange(node->system(), parameter);

    return bisectAt(node, parameter, range.middle());
}

}
