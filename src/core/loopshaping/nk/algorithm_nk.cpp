/**
 * @file
 * @brief Algorithm NK: the NT branch and bound with Quick Solution and local search.
 *
 * The additions are wired at the paper's steps: Quick Solution inside the
 * feasibility test of every box, applied per frequency with the latest
 * updated box as section 3.3 asks; local optimisation launched from the
 * leading box under the ten percent rule, whose certified result prunes the
 * list and stands in as the answer when the search exhausts the space.
 * Nominal stability of a bounds-feasible box is checked when it reaches the
 * head of the list rather than on insertion, which gives the same search at
 * a fraction of the criterion's cost. Termination and cancellation are as in
 * algorithm NT.
 */

#include <vector>
#include <cstdint>
#include "src/core/common/exception.h"
#include "src/core/loopshaping/nk/algorithm_nk.h"

namespace quick_solution = qftbx::quick_solution;

namespace qftbx {

void AlgorithmNk::setProblem(LtiSystem *plant, LtiSystem *controller, std::vector<double> * omega, const BoundaryData *boundaries,
                                     double epsilon, std::int32_t initialisation){

    this->plant = plant;
    this->controller = controller->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;
    m_start = initialisation == 1 ? Extremes : Centre;
}

bool AlgorithmNk::solve(){

    liveList = std::make_unique<OrderedList>(false, m_settings.search.maxLiveNodes);
    conversion = std::make_unique<NaturalIntervalExtension>();
    detector = std::make_unique<BoundaryViolationDetector>(m_settings.algorithms.conservativeBoundaryColumns);
    stability = std::make_unique<NominalStabilityChecker>(plant, omega, m_settings.stability);

    bestLocalGain = std::numeric_limits<double>::infinity();
    bestLocalController.reset();
    launchGains.clear();

    prototype = controller->clone();

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
            if (bestLocalController != nullptr) {
                designedController = std::move(bestLocalController);
                return true;
            }

            throw qftbx::InvalidInput(QFTBX_TR("Core", "No feasible solution exists in the given search box."));
        }

        std::unique_ptr<SearchNode> node = liveList->takeFirstAs<SearchNode>();

        if (node->flag() == feasible && !stability->isNominallyStable(cornerOf(node->system(), true))) {
            continue;
        }

        if (node->system()->gain().range().min >= bestLocalGain) {
            continue;
        }

        localOptimization(node->system());

        if (node->system()->gain().range().min >= bestLocalGain) {
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

std::size_t AlgorithmNk::peakLiveNodes() const
{
    return liveList != nullptr ? liveList->peakSize() : 0;
}

LoopShapingStatistics AlgorithmNk::statistics() const
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

std::unique_ptr<LtiSystem> AlgorithmNk::controllerStructure(){
    return std::move(designedController);
}

void AlgorithmNk::check_box_feasibility(std::unique_ptr<LtiSystem> box){

    BoxClassification classification;
    BoxFlag flag_final = feasible;

    if (bestLocalGain < box->gain().range().max &&
            bestLocalGain > box->gain().range().min) {
        box = box->create(box->name(), box->numerator(), box->denominator(),
                Parameter(box->gain().name(), Range(box->gain().range().min, bestLocalGain),
                          box->gain().range().min),
                box->delay());
    }

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

            if (classification.isBottomLeftForbidden()) {
                box = quickSolution(std::move(box), classification.extremes()[0],
                                    o, nominalPlantValues.at(frequencyIndex));
            }
        }

        frequencyIndex++;
    }

    if (flag_final == ambiguous && stability->isBoxUnstable(box.get(), *conversion)) {
        return;
    }

    const double gainInf = box->gain().range().min;

    liveList->insert(std::make_unique<SearchNode>(gainInf, std::move(box), flag_final));
}

std::unique_ptr<LtiSystem> AlgorithmNk::quickSolution(std::unique_ptr<LtiSystem> v, double boundMinDb,
                                                       double w, std::complex<double> p0){

    const double boundMin = std::pow(10.0, boundMinDb / 20.0);

    ParameterBounds bounds = boundsOf(v.get());

    if (!cutBelowBoundary(bounds, boundMin, w, p0)) {
        return v;
    }

    return boxFromBounds(v.get(), bounds);
}

double AlgorithmNk::minimalFeasibleGain(const std::vector<double> & zeros,
                                                       const std::vector<double> & poles,
                                                       LtiSystem * box, std::int32_t & budget){

    double high = box->gain().range().max;
    double low = box->gain().range().min;

    std::vector<NaturalIntervalExtension::Factors> factors;
    factors.reserve(omega->size());
    for (double w : *omega) {
        factors.push_back(conversion->factorsOf(zeros, poles, w));
    }

    budget--;
    if (!pointIsFeasible(factors, high)) {
        return std::numeric_limits<double>::infinity();
    }

    budget--;
    if (pointIsFeasible(factors, low)) {
        return low;
    }

    while (high / low > m_settings.algorithms.gainTolerance && budget > 0) {
        const double mid = std::sqrt(low * high);

        budget--;
        if (pointIsFeasible(factors, mid)) {
            high = mid;
        } else {
            low = mid;
        }
    }

    return high;
}

void AlgorithmNk::localOptimization(LtiSystem * box){

    const double launch = box->gain().range().min;

    for (double previous : launchGains) {
        if (std::abs(launch - previous) <= 0.1 * std::max<double>(1.0, std::abs(previous))) {
            return;
        }
    }

    launchGains.push_back(launch);

    std::vector<double> zeros, poles;
    double gain;
    startingPoint(box, zeros, poles, gain);

    std::int32_t budget = m_settings.algorithms.localSearchBudget;

    double bestGain = minimalFeasibleGain(zeros, poles, box, budget);
    std::vector<double> bestZeros = zeros;
    std::vector<double> bestPoles = poles;

    const auto logRange = [](Parameter & var) {
        return std::log10(var.range().max) - std::log10(std::max<double>(var.range().min, 1e-12));
    };

    const auto tryMove = [&](bool isPole, std::size_t j, double stepDecades) -> bool {
        Parameter & var = isPole ? box->denominator()[j] : box->numerator()[j];
        std::vector<double> & values = isPole ? bestPoles : bestZeros;

        for (double direction : {stepDecades, -stepDecades}) {
            const double candidate = values.at(j) * std::pow(10.0, direction);

            if (candidate <= var.range().min || candidate >= var.range().max) {
                continue;
            }

            std::vector<double> trial = values;
            trial[j] = candidate;

            const double k = isPole ? minimalFeasibleGain(bestZeros, trial, box, budget)
                                   : minimalFeasibleGain(trial, bestPoles, box, budget);

            if (k < bestGain / m_settings.algorithms.gainTolerance) {
                values = trial;
                bestGain = k;
                return true;
            }
        }

        return false;
    };

    for (double divisor : {4.0, 8.0, 16.0}) {
        bool improved = true;

        while (improved && budget > 0) {
            improved = false;

            for (std::size_t j = 0; j < bestZeros.size() && budget > 0; ++j) {
                if (box->numerator()[j].isUncertain()) {
                    improved = tryMove(false, j, logRange(box->numerator()[j]) / divisor) || improved;
                }
            }

            for (std::size_t j = 0; j < bestPoles.size() && budget > 0; ++j) {
                if (box->denominator()[j].isUncertain()) {
                    improved = tryMove(true, j, logRange(box->denominator()[j]) / divisor) || improved;
                }
            }
        }
    }

    if (bestGain < bestLocalGain) {
        const PointController candidate{bestGain, bestZeros, bestPoles};

        if (stability->isNominallyStable(candidate)) {
            bestLocalGain = bestGain;
            bestLocalController = pointSystem(bestZeros, bestPoles, bestGain);
        }
    }
}

std::unique_ptr<LtiSystem> AlgorithmNk::pointSystem(const std::vector<double> & zeros,
                                                     const std::vector<double> & poles, double gain){
    std::vector<Parameter> numerator;
    numerator.reserve(zeros.size());
    for (double z : zeros) {
        numerator.emplace_back(z);
    }

    std::vector<Parameter> denominator;
    denominator.reserve(poles.size());
    for (double p : poles) {
        denominator.emplace_back(p);
    }

    return prototype->create(prototype->name(), std::move(numerator), std::move(denominator),
                             Parameter(gain), prototype->delay());
}

bool AlgorithmNk::pointIsFeasible(const std::vector<NaturalIntervalExtension::Factors> & factors,
                                  double gain){

    if (gain <= 0.0 || std::isinf(gain)) {
        return false;
    }

    for (std::size_t i = 0; i < omega->size(); ++i) {
        const NicholsBox projection = conversion->nicholsOf(Interval(gain), factors.at(i),
                                                           nominalPlantValues.at(i));
        const BoxFlag flag = detector->classifyBox(projection, boundaries, i).flag();

        if (flag != feasible) {
            return false;
        }
    }

    return true;
}

void AlgorithmNk::startingPoint(LtiSystem * box, std::vector<double> & zeros,
                                                std::vector<double> & poles, double & gain){

    const auto pick = [this](Parameter & var, bool isPole) -> double {
        if (!var.isUncertain()) {
            return var.nominal();
        }
        const Range r = var.range();
        return m_start == Centre ? r.middle()
                             : (isPole ? r.max : r.min);
    };

    zeros.clear();
    poles.clear();

    for (Parameter & var : box->numerator()) {
        zeros.push_back(pick(var, false));
    }
    for (Parameter & var : box->denominator()) {
        poles.push_back(pick(var, true));
    }

    Parameter & k = box->gain();
    if (!k.isUncertain()) {
        gain = k.nominal();
    } else if (m_start == Centre) {
        gain = (k.range().min + k.range().max) / 2.0;
    } else {
        gain = k.range().max;
    }
}

}
