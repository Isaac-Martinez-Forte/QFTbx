/**
 * @file
 * @brief Algorithm NT: the branch and bound of Tharewal 2005, sections 3.3.3 and 5.2.1.
 *
 * The main loop follows steps 1 to 7 of section 3.3.3: the initial box
 * enters the live list unless certainly infeasible, an empty list proves
 * there is no feasible solution, and the leading box terminates the search
 * when feasible or smaller than epsilon at every frequency, in which case a
 * corner verified against the boundaries and the nominal stability criterion
 * is returned. Cancellation is checked once per node and reported as an
 * exception, because a false return already means the space was exhausted.
 * The gain cuts of section 5.2.1 are applied inside the feasibility test.
 */

#include <vector>
#include <cstdint>
#include "src/core/common/exception.h"
#include "src/core/loopshaping/nt/algorithm_nt.h"

namespace qftbx {

void AlgorithmNt::setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> *omega, const BoundaryData * boundaries,
                                 double epsilon) {

    this->plant = plant;
    this->controller = controller->clone();
    depthAccounting.start(*this->controller);
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;
}

bool AlgorithmNt::solve() {

    liveList = std::make_unique<OrderedList>(false, m_settings.search.maxLiveNodes);

    conversion = std::make_unique<NaturalIntervalExtension>();
    detector = std::make_unique<BoundaryViolationDetector>(m_settings.algorithms.conservativeBoundaryColumns);
    stability = std::make_unique<NominalStabilityChecker>(plant, omega, m_settings.stability);
    family = std::make_unique<FamilyStabilityChecker>(plant, controller.get(),
                                                     m_settings.algorithms.familyStabilityGate ? m_sweep : ParameterGrids());

    nominalPlantValues.clear();

    for (double o : *omega) {
        std::complex <double> c = plant->evaluate(o);
        nominalPlantValues.push_back(c);
    }

    check_box_feasibility(std::move(controller));

    while (true) {

        if (qftbx::cancellationAsked(m_cancellation)) {
            throw qftbx::Cancelled();
        }

        if (liveList->isEmpty()) {
            throw qftbx::InvalidInput(QFTBX_TR("Core", "No feasible solution exists in the given search box."));
        }

        std::unique_ptr<SearchNode> node = liveList->takeFirstAs<SearchNode>();

        if (node->flag() == feasible && !stability->isNominallyStable(cornerOf(node->system(), true))) {
            continue;
        }

        if (node->flag() == feasible) {
            if (family->isStable(cornerOf(node->system(), true))) {
                designedController = pointFromBox(node->system(), true);

                return true;
            }
            if (isEpsilonSmall(node->system(), this->epsilon, omega, conversion.get(), nominalPlantValues)) {
                continue;
            }
            BisectionResult halves = bisectWidestParameter(node->system());
            check_box_feasibility(std::move(halves.v1));
            check_box_feasibility(std::move(halves.v2));
            continue;
        }

        if (isEpsilonSmall(node->system(), this->epsilon, omega, conversion.get(), nominalPlantValues)) {
            const std::optional<PointController> corner = verifiedCorner(node->system(), omega,
                    conversion.get(), detector.get(), boundaries, nominalPlantValues);

            if (!corner || !stability->isNominallyStable(*corner) || !family->isStable(*corner)) {
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

std::size_t AlgorithmNt::peakLiveNodes() const
{
    return liveList != nullptr ? liveList->peakSize() : 0;
}

LoopShapingStatistics AlgorithmNt::statistics() const
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

std::unique_ptr<LtiSystem> AlgorithmNt::controllerStructure() {
    return std::move(designedController);
}

void AlgorithmNt::check_box_feasibility(std::unique_ptr<LtiSystem> box) {

    BoxClassification classification;

    BoxFlag flag_final = feasible;

    std::size_t frequencyIndex = 0;
    NicholsBox projection;

    double feasibleFrom = 0;
    bool feasibleCertified = true;

    for (double o : *omega) {

        const NaturalIntervalExtension::Factors factors = conversion->factorsOf(box.get(), o);
        const std::complex<double> p0 = nominalPlantValues.at(frequencyIndex);

        projection = conversion->nicholsOf(Interval(box->gain().range().min, box->gain().range().max),
                                           factors, p0);

        classification = detector->classifyBox(projection, boundaries, frequencyIndex);

        if (classification.flag() == infeasible) {
            depthAccounting.record(*box, infeasible);
            return;
        }

        if (classification.flag() == ambiguous) {
            flag_final = ambiguous;
            depthAccounting.ambiguousAt(frequencyIndex);

            const double minBoundary = classification.extremes()[0];
            const double maxBoundary = classification.extremes()[1];

            box = accelerated(std::move(box), minBoundary, factors, frequencyIndex,
                             !classification.isBottomLeftForbidden());

            if (feasibleCertified) {
                double from;
                if (feasibleGainFrom(box.get(), maxBoundary, projection, factors, frequencyIndex, from)) {
                    feasibleFrom = std::max(feasibleFrom, from);
                } else {
                    feasibleCertified = false;
                }
            }
        }

        frequencyIndex++;
    }

    const double kInf = box->gain().range().min;
    const double kSup = box->gain().range().max;

    if (flag_final == ambiguous && stability->isBoxUnstable(box.get(), *conversion)) {
        return;
    }

    if (flag_final == ambiguous && feasibleCertified &&
            feasibleFrom > kInf * 1.01 && feasibleFrom < kSup * 0.99) {

        const std::unique_ptr<LtiSystem> base = box->clone();

        check_box_feasibility(base->create(base->name(), base->numerator(),
                base->denominator(),
                Parameter(base->gain().name(), Range(feasibleFrom, kSup), feasibleFrom),
                base->delay()));

        box = box->create(box->name(), box->numerator(), box->denominator(),
                Parameter(box->gain().name(), Range(kInf, feasibleFrom), kInf),
                box->delay());
    }

    const double gainInf = box->gain().range().min;

    depthAccounting.record(*box, flag_final);
    liveList->insert(std::make_unique<SearchNode>(gainInf, std::move(box), flag_final));

}

std::unique_ptr<LtiSystem> AlgorithmNt::accelerated(std::unique_ptr<LtiSystem> v,
        double minBoundary, const NaturalIntervalExtension::Factors & factors,
        std::size_t frequencyIndex, bool above) {

    if (!above){

        const double minGainLinear = v->gain().range().min;
        const double minGainDb = 20 * log10(minGainLinear);

        const double magnitudeAtMinGainDb = conversion->nicholsOf(Interval(minGainLinear), factors,
                nominalPlantValues.at(frequencyIndex)).magnitudeDb.upper();

        if (magnitudeAtMinGainDb < minBoundary) {

            double cutGainDb = minGainDb + (minBoundary - magnitudeAtMinGainDb);

            double cutGainLinear = pow(10, cutGainDb / 20);

            v = v->create(v->name(), v->numerator(), v->denominator(),
                    Parameter(v->gain().name(), Range(cutGainLinear, v->gain().range().max), cutGainLinear),
                    v->delay());
        }
    }

    return v;
}

bool AlgorithmNt::feasibleGainFrom(LtiSystem * v, double maxBoundary,
                                   NicholsBox projection, const NaturalIntervalExtension::Factors & factors,
                                   std::size_t frequencyIndex, double & from) {

    const double phaseCentre = (projection.phaseDegrees.lower() + projection.phaseDegrees.upper()) / 2.0;

    if (detector->classifyPoint(qftbx::NicholsPoint(phaseCentre, maxBoundary + 1.0),
                                   boundaries, frequencyIndex) != feasible) {
        return false;
    }

    const double maxGainLinear = v->gain().range().max;
    const double maxGainDb = 20 * log10(maxGainLinear);

    const double magnitudeAtMaxGainDb = conversion->nicholsOf(Interval(maxGainLinear), factors,
            nominalPlantValues.at(frequencyIndex)).magnitudeDb.lower();

    if (magnitudeAtMaxGainDb <= maxBoundary) {
        return false;
    }

    const double feasibleGainDb = maxGainDb - (magnitudeAtMaxGainDb - maxBoundary);

    from = pow(10, feasibleGainDb / 20);

    return true;
}

}
