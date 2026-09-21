/**
 * @file
 * @brief Algorithm MC1: the branch and bound of the paper's algorithm 5 with QS2.
 *
 * The two stages of QS2 cut every box inside the feasibility test, per
 * frequency with the latest updated box, and its third stage is attempted
 * once on the surviving box: a certified feasible gain subrange updates the
 * prune variable C of step 3bis, which discards nodes that cannot improve it
 * and caps the gain range of every new box. The certified solution stands in
 * when the search exhausts the space. Nominal stability of a feasible box is
 * checked when it is popped, an ambiguous box whose members are all unstable
 * dies on insertion, and termination and cancellation are as in NT.
 */

#include <vector>
#include "src/core/math/constants.h"
#include <cstdint>
#include "src/core/common/exception.h"
#include "src/core/loopshaping/mc1/algorithm_mc1.h"

namespace qftbx {

void AlgorithmMc1::setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega,
                                          const BoundaryData * boundaries, double epsilon)
{
    this->plant = plant;
    this->controller = controller->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;
}

bool AlgorithmMc1::solve()
{
    liveList = std::make_unique<OrderedList>(false, m_settings.search.maxLiveNodes);
    conversion = std::make_unique<NaturalIntervalExtension>();
    detector = std::make_unique<BoundaryViolationDetector>(m_settings.algorithms.conservativeBoundaryColumns);
    stability = std::make_unique<NominalStabilityChecker>(plant, omega, m_settings.stability);

    bestCertifiedGain = std::numeric_limits<double>::infinity();
    bestCertifiedController = nullptr;

    nominalPlantValues.clear();

    for (double o : *omega) {
        std::complex<double> c = plant->evaluate(o);
        nominalPlantValues.push_back(c);
    }

    check_box_feasibility(std::move(controller));

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

        std::unique_ptr<SearchNode> node = liveList->takeFirstAs<SearchNode>();

        if (node->flag() == feasible && !stability->isNominallyStable(cornerOf(node->system(), true))) {
            continue;
        }

        if (node->system()->gain().range().min >= bestCertifiedGain) {
            continue;
        }

        if (node->flag() == feasible) {
            designedController = pointFromBox(node->system(), true);
            return true;
        }

        if (isEpsilonSmall(node->system(), this->epsilon, omega, conversion.get(), nominalPlantValues)) {
            const std::optional<PointController> corner = verifiedCorner(node->system(), omega,
                    conversion.get(), detector.get(), boundaries, nominalPlantValues);

            if (!corner || !stability->isNominallyStable(*corner)) {
                continue;
            }

            designedController = systemFromPoint(node->system(), *corner);
            return true;
        }
        BisectionResult halves = bisectWidestParameter(node->system());

        check_box_feasibility(std::move(halves.v1));
        check_box_feasibility(std::move(halves.v2));
    }
}

std::size_t AlgorithmMc1::peakLiveNodes() const
{
    return liveList != nullptr ? liveList->peakSize() : 0;
}

LoopShapingStatistics AlgorithmMc1::statistics() const
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
    if (stability != nullptr) {
        statistics.stabilityVerdicts = stability->statistics().verdicts;
        statistics.stabilityProfiles = stability->statistics().profilesComputed;
    }
    return statistics;
}

std::unique_ptr<LtiSystem> AlgorithmMc1::controllerStructure()
{
    return std::move(designedController);
}

void AlgorithmMc1::check_box_feasibility(std::unique_ptr<LtiSystem> box)
{
    BoxClassification classification;
    BoxFlag flag_final = feasible;

    box = capGain(std::move(box), bestCertifiedGain);

    std::size_t frequencyIndex = 0;
    NicholsBox projection;

    for (double o : *omega) {

        projection = conversion->nicholsBox(box.get(), o, nominalPlantValues.at(frequencyIndex));

        classification = detector->classifyBox(projection, boundaries, frequencyIndex);

        if (classification.flag() == infeasible) {
            return;
        }

        if (classification.flag() == ambiguous) {
            flag_final = ambiguous;

            box = quickSolution2(std::move(box), classification, projection, o,
                                 nominalPlantValues.at(frequencyIndex));
        }

        frequencyIndex++;
    }

    if (flag_final == ambiguous && stability->isBoxUnstable(box.get(), *conversion)) {
        return;
    }

    if (flag_final == ambiguous) {
        certifiedGainSearch(box.get());
    }

    const double gainInf = box->gain().range().min;

    liveList->insert(std::make_unique<SearchNode>(gainInf, std::move(box), flag_final));
}

std::unique_ptr<LtiSystem> AlgorithmMc1::quickSolution2(std::unique_ptr<LtiSystem> v,
                                                      const BoxClassification & classification,
                                                      const NicholsBox & projection,
                                                      double w, std::complex<double> p0)
{
    ParameterBounds bounds = boundsOf(v.get());

    bool cut = false;

    if (classification.isBottomLeftForbidden()) {
        const double boundMin = std::pow(10.0, classification.extremes()[0] / 20.0);
        cut = cutBelowBoundary(bounds, boundMin, w, p0) || cut;
    }

    const double phi0 = nominalPhase(p0);
    const double phaseStep = boundaries->phaseRange().width() /
                        (boundaries->phaseCount() - 1);

    const double boxPhaseMin = projection.phaseDegrees.lower();
    const double boxPhaseMax = projection.phaseDegrees.upper();

    const double boundPhaseMin = classification.extremes()[2];
    const double boundPhaseMax = classification.extremes()[3];

    if (classification.isTopRightForbidden() && boundPhaseMax < boxPhaseMax - phaseStep) {
        cut = cutRightOfPhase(bounds, boundPhaseMax * qftbx::math::kPi / 180.0, phi0, w) || cut;
    }

    if (classification.isBottomLeftForbidden() && boundPhaseMin > boxPhaseMin + phaseStep) {
        cut = cutLeftOfPhase(bounds, boundPhaseMin * qftbx::math::kPi / 180.0, phi0, w) || cut;
    }

    if (!cut) {
        return v;
    }

    return boxFromBounds(v.get(), bounds);
}

bool AlgorithmMc1::gainRangeIsFeasible(const std::vector<NaturalIntervalExtension::Factors> & factors,
                                       double gainInf, double gainSup)
{
    const Interval gain(gainInf, gainSup);

    bool feasibleEverywhere = true;

    for (std::size_t i = 0; i < omega->size() && feasibleEverywhere; ++i) {
        const NicholsBox projection = conversion->nicholsOf(gain, factors.at(i), nominalPlantValues.at(i));
        feasibleEverywhere = detector->classifyBox(projection, boundaries, i).flag() == feasible;
    }

    return feasibleEverywhere;
}

void AlgorithmMc1::certifiedGainSearch(LtiSystem * box)
{
    if (!box->gain().isUncertain()) {
        return;
    }

    const double low = box->gain().range().min;
    double high = box->gain().range().max;

    if (low <= 0.0) {
        return;
    }

    std::vector<NaturalIntervalExtension::Factors> factors;
    factors.reserve(omega->size());
    for (double w : *omega) {
        factors.push_back(conversion->factorsOf(box, w));
    }

    if (!gainRangeIsFeasible(factors, high, high)) {
        return;
    }

    double lo = low;

    if (gainRangeIsFeasible(factors, lo, high)) {
        high = lo;
    } else {
        double hi = high;

        while (hi / lo > m_settings.algorithms.certifiedGainTolerance) {
            const double mid = std::sqrt(lo * hi);

            if (gainRangeIsFeasible(factors, mid, high)) {
                hi = mid;
            } else {
                lo = mid;
            }
        }

        high = hi;
    }

    if (high >= bestCertifiedGain) {
        return;
    }

    PointController point = cornerOf(box, true);
    point.gain = high;

    if (stability->isNominallyStable(point)) {
        bestCertifiedGain = high;
        bestCertifiedController = systemFromPoint(box, point);
    }
}

}
