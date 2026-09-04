#include <vector>
#include <cstdint>
#include "src/core/exception.h"
#include "src/core/loopshaping/algorithm_nk.h"

using namespace cxsc;

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


//Main loop: the NT branch & bound (Tharewal 2005, sec. 3.3.3) with the
//NK additions wired at the paper's steps: local optimization on the
//leading box (steps 5-6 and 18-20) and Quick Solution inside the
//feasibility test of every box (steps 2 and 9).
bool AlgorithmNk::solve(){

    liveList = std::make_unique<OrderedList>(false, m_settings.search.maxLiveNodes);
    conversion = std::make_unique<NaturalIntervalExtension>();
    detector = std::make_unique<BoundaryViolationDetector>();
    stability = std::make_unique<NominalStabilityChecker>(plant, omega, m_settings.stability);

    bestLocalGain = std::numeric_limits<double>::infinity();
    bestLocalController.reset();
    launchGains.clear();

    //Stable prototype for building point controllers: the working box
    //pointer is replaced as Quick Solution rebuilds it.
    prototype = controller->clone();

    nominalPlantValues.clear();
    nominalPlantValuesStd.clear();

    for (double o : *omega) {
        std::complex<double> c = plant->evaluate(o);
        nominalPlantValuesStd.push_back(c);
        nominalPlantValues.push_back(cxsc::complex(c.real(), c.imag()));
    }

    //Steps 1-3: Quick Solution and feasibility of the initial box happen
    //inside check_box_feasibility, which inserts it unless certainly
    //infeasible.
    check_box_feasibility(std::move(controller));

    while (true) {

        //Once per node: the cheapest possible place to notice, and the only
        //one that bounds how long a cancellation takes to take effect. It
        //throws rather than returning false, because false already means
        //"searched everything and found nothing", which is a different
        //answer and one the caller reports differently.
        if (qftbx::cancellationAsked(m_cancellation)) {
            throw qftbx::Cancelled();
        }

        if (liveList->isEmpty()) {
            //Step 15. A certified feasible local solution stands in as
            //the answer when the interval search exhausts the space (the
            //local point was verified against bounds and stability).
            if (bestLocalController != nullptr) {
                designedController = std::move(bestLocalController);
                return true;
            }

            throw qftbx::InvalidInput(
                    "No feasible solution exists in the given search box.");
        }

        std::unique_ptr<SearchNode> node = liveList->takeFirstAs<SearchNode>();

        //Pruning by the local solution (step 4 of the paper's outline /
        //G-bis of the thesis): a node whose gain infimum cannot improve
        //the certified local solution is discarded.
        if (node->system()->gain().range().min >= bestLocalGain) {
            continue;
        }

        //Steps 17-20: local optimization launched from the leading box
        //under the 10% decision rule; a feasible result prunes the list
        //through bestLocalGain.
        localOptimization(node->system());

        if (node->system()->gain().range().min >= bestLocalGain) {
            continue;
        }

        //Step 21 and Remark 3.1 termination, as reviewed for NT.
        if (node->flag() == feasible || isEpsilonSmall(node->system(), this->epsilon, omega, conversion.get(), nominalPlantValues)) {
            if (node->flag() == ambiguous) {
                designedController = pointFromBox(node->system(), false);

                if (!stability->isNominallyStable(designedController.get())) {
                    designedController.reset();
                    continue;
                }
            } else {
                designedController = pointFromBox(node->system(), true);
            }

            return true;
        }

        //Step 8: bisect along the widest parameter direction.
        BisectionResult halves = bisectWidestParameter(node->system());

        //Steps 9-14: Quick Solution + feasibility + insertion.
        check_box_feasibility(std::move(halves.v1));
        check_box_feasibility(std::move(halves.v2));
    }
}


std::size_t AlgorithmNk::peakLiveNodes() const
{
    return liveList != nullptr ? liveList->peakSize() : 0;
}


std::unique_ptr<LtiSystem> AlgorithmNk::controllerStructure(){
    return std::move(designedController);
}


//Feasibility test over every design frequency with the NK Quick Solution
//cutting applied per frequency with the latest updated box (paper,
//sec. 3.3: "one always uses the latest updated values"). Certainly
//infeasible boxes are destroyed; anything else enters the live list.
void AlgorithmNk::check_box_feasibility(std::unique_ptr<LtiSystem> box){

    BoxClassification classification;
    BoxFlag flag_final = feasible;

    //Step 20 of the paper: the certified local solution caps the useful
    //gain range of every new box.
    if (bestLocalGain < box->gain().range().max &&
            bestLocalGain > box->gain().range().min) {
        box = box->create(box->name(), box->numerator(), box->denominator(),
                Parameter("kv", Range(box->gain().range().min, bestLocalGain),
                              box->gain().range().min, "kv"),
                box->delay());
    }

    std::size_t frequencyIndex = 0;
    cinterval projection;

    for (double o : *omega) {

        projection = conversion->nicholsBox(box.get(), o, nominalPlantValues.at(frequencyIndex));

        classification = detector->classifyBox(projection, boundaries, frequencyIndex);

        if (classification.flag() == infeasible) {
            return;
        }

        if (classification.flag() == ambiguous) {
            flag_final = ambiguous;

            //Quick Solution at this frequency: sound only when the zone
            //under every boundary point is certainly forbidden, certified
            //by the parity classification of the box's lower corner.
            if (classification.isBottomLeftForbidden()) {
                box = quickSolution(std::move(box), classification.extremes()[0],
                                    o, nominalPlantValuesStd.at(frequencyIndex));
            }
        }

        frequencyIndex++;
    }

    //Nominal closed-loop stability of bounds-feasible boxes (the paper
    //demands the zeros of 1 + L0 in the left half-plane; checked on the
    //Nichols chart).
    if (flag_final == feasible) {
        const std::unique_ptr<LtiSystem> point = pointFromBox(box.get(), true);

        if (!stability->isNominallyStable(point.get())) {
            return;
        }
    }

    //The index is read BEFORE the box is handed over: as arguments of one
    //call their evaluation order is unspecified.
    const double gainInf = box->gain().range().min;

    liveList->insert(std::make_unique<SearchNode>(gainInf, std::move(box), flag_final));
}


//Quick Solution (paper sec. 3.3, algorithm QS): cut the certainly
//infeasible subranges of the gain, every zero and every pole with the
//closed-form monotonicity equations, sequentially, using the latest
//updated values. boundMinDb is |B_i|min over the box's phase interval.
std::unique_ptr<LtiSystem> AlgorithmNk::quickSolution(std::unique_ptr<LtiSystem> v, double boundMinDb,
                                                       double w, std::complex<double> p0){

    const double boundMin = std::pow(10.0, boundMinDb / 20.0);

    ParameterBounds bounds = boundsOf(v.get());

    if (!cutBelowBoundary(bounds, boundMin, w, p0)) {
        return v;
    }

    return boxFromBounds(v.get(), bounds);
}


//Local optimization (paper sec. 3.2; the paper only says "call any
//nonlinear constrained local optimization routine", so the routine is
//ours): a lean two-level pattern search. The objective is the gain alone,
//so the inner level finds the minimal feasible gain for fixed zeros/poles
//by logarithmic bisection (the predicate is the point bounds test; local
//crossing only, as a local method promises), and the outer level moves
//the zeros/poles with a Hooke-Jeeves style coordinate pattern in LOG
//space with an adaptive, coarsening step. A hard evaluation budget keeps
//the search cheaper than the pruning it buys, and the candidate must pass
//the nominal stability criterion once, at the end, before it may prune
//the global search. Launched under the paper's 10% decision rule.

double AlgorithmNk::minimalFeasibleGain(const std::vector<double> & zeros,
                                                       const std::vector<double> & poles,
                                                       LtiSystem * box, std::int32_t & budget){

    double high = box->gain().range().max;
    double low = box->gain().range().min;

    budget--;
    if (!pointIsFeasible(zeros, poles, high)) {
        return std::numeric_limits<double>::infinity();
    }

    budget--;
    if (pointIsFeasible(zeros, poles, low)) {
        return low;
    }

    while (high / low > m_settings.algorithms.gainTolerance && budget > 0) {
        const double mid = std::sqrt(low * high);

        budget--;
        if (pointIsFeasible(zeros, poles, mid)) {
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

    //Coordinate pattern over zeros/poles in log space, coarse to fine.
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
        std::unique_ptr<LtiSystem> candidate = pointSystem(bestZeros, bestPoles, bestGain);

        if (stability->isNominallyStable(candidate.get())) {
            bestLocalGain = bestGain;
            bestLocalController = std::move(candidate);
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


//Point feasibility against the bounds at every design frequency, with the
//same projection + detection the interval test uses (the historical local
//search passed the GAIN as the frequency index of the detection).
bool AlgorithmNk::pointIsFeasible(const std::vector<double> & zeros,
                                                  const std::vector<double> & poles, double gain){

    if (gain <= 0.0 || std::isinf(gain)) {
        return false;
    }

    const std::unique_ptr<LtiSystem> point = pointSystem(zeros, poles, gain);

    for (std::size_t i = 0; i < omega->size(); ++i) {
        const cinterval projection = conversion->nicholsBox(point.get(), omega->at(i),
                                                      nominalPlantValues.at(i));
        const BoxFlag flag = detector->classifyBox(projection, boundaries, i).flag();

        if (flag != feasible) {
            return false;
        }
    }

    return true;
}


//Starting point of the local search, per the GUI choice: the box centre
//or the |L0|-maximal corner.
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

} // namespace qftbx
