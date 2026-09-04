#include <vector>
#include "src/core/math/constants.h"
#include <cstdint>
#include "src/core/exception.h"
#include "src/core/loopshaping/algorithm_mc1.h"

using namespace cxsc;

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


//Main loop: the NT branch & bound (paper, algorithm 5) with QS2 inside
//the feasibility test of every box (steps 1(b-bis) and 4bis) and the
//prune variable C of step 3bis behind bestCertifiedGain.
bool AlgorithmMc1::solve()
{
    liveList = std::make_unique<OrderedList>(false, m_settings.search.maxLiveNodes);
    conversion = std::make_unique<NaturalIntervalExtension>();
    detector = std::make_unique<BoundaryViolationDetector>();
    stability = std::make_unique<NominalStabilityChecker>(plant, omega, m_settings.stability);

    bestCertifiedGain = std::numeric_limits<double>::infinity();
    bestCertifiedController = nullptr;

    nominalPlantValues.clear();
    nominalPlantValuesStd.clear();

    for (double o : *omega) {
        std::complex<double> c = plant->evaluate(o);
        nominalPlantValuesStd.push_back(c);
        nominalPlantValues.push_back(cxsc::complex(c.real(), c.imag()));
    }

    //Steps 1-2: QS2 and feasibility of the initial box happen inside
    //check_box_feasibility, which inserts it unless certainly infeasible.
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
            //The certified solution of QS2 stage 3 stands in when the
            //interval search exhausts the space (the paper keeps its box
            //z' in the list instead; same fallback).
            if (bestCertifiedController != nullptr) {
                designedController = std::move(bestCertifiedController);
                return true;
            }

            throw qftbx::InvalidInput(
                    "No feasible solution exists in the given search box.");
        }

        std::unique_ptr<SearchNode> node = liveList->takeFirstAs<SearchNode>();

        //Step 3bis.(a): a node whose gain infimum cannot improve the
        //certified solution is discarded.
        if (node->system()->gain().range().min >= bestCertifiedGain) {
            continue;
        }

        //Step 3 and Remark 3.1 termination, as reviewed for NT.
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

        //Step 4: bisect along the widest parameter direction.
        BisectionResult halves = bisectWidestParameter(node->system());

        //Steps 4bis-6: QS2 + feasibility + insertion.
        check_box_feasibility(std::move(halves.v1));
        check_box_feasibility(std::move(halves.v2));
    }
}


std::size_t AlgorithmMc1::peakLiveNodes() const
{
    return liveList != nullptr ? liveList->peakSize() : 0;
}


std::unique_ptr<LtiSystem> AlgorithmMc1::controllerStructure()
{
    return std::move(designedController);
}


//Feasibility test over every design frequency with the QS2 stages 1-2
//cutting applied per frequency with the latest updated box, and stage 3
//attempted once on the surviving box. Certainly infeasible boxes are
//destroyed; anything else enters the live list.
void AlgorithmMc1::check_box_feasibility(std::unique_ptr<LtiSystem> box)
{
    BoxClassification classification;
    BoxFlag flag_final = feasible;

    //Step 3bis.(b): the certified solution caps the useful gain range of
    //every new box.
    box = capGain(std::move(box), bestCertifiedGain);

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

            box = quickSolution2(std::move(box), classification, projection, o,
                                 nominalPlantValuesStd.at(frequencyIndex));
        }

        frequencyIndex++;
    }

    //Nominal closed-loop stability of bounds-feasible boxes, as reviewed
    //for NT/NK.
    if (flag_final == feasible) {
        const std::unique_ptr<LtiSystem> point = pointFromBox(box.get(), true);

        if (!stability->isNominallyStable(point.get())) {
            return;
        }
    }

    //QS2 stage 3 on the surviving ambiguous box: a certified feasible
    //gain subrange updates the prune variable C.
    if (flag_final == ambiguous) {
        certifiedGainSearch(box.get());
    }

    //The index is read BEFORE the box is handed over: as arguments of one
    //call their evaluation order is unspecified.
    const double gainInf = box->gain().range().min;

    liveList->insert(std::make_unique<SearchNode>(gainInf, std::move(box), flag_final));
}


//QS2 stages 1 and 2 at one design frequency (paper, algorithm 4): the
//magnitude cuts of NK's Quick Solution when the strip under the boundary
//minimum is certainly forbidden, and the phase cuts when a vertical strip
//is. All cuts run sequentially on the latest updated values.
std::unique_ptr<LtiSystem> AlgorithmMc1::quickSolution2(std::unique_ptr<LtiSystem> v,
                                                      const BoxClassification & classification,
                                                      const cxsc::cinterval & projection,
                                                      double w, std::complex<double> p0)
{
    ParameterBounds bounds = boundsOf(v.get());

    bool cut = false;

    //-------------------------------------------------- stage 1, magnitude
    //Sound only when the zone under every boundary point is certainly
    //forbidden, certified by the parity classification of the box's lower
    //corner (same gate as NK).
    if (classification.isBottomLeftForbidden()) {
        const double boundMin = std::pow(10.0, classification.extremes()[0] / 20.0);
        cut = cutBelowBoundary(bounds, boundMin, w, p0) || cut;
    }

    //------------------------------------------------------ stage 2, phase
    const double phi0 = nominalPhase(p0);
    const double phaseStep = boundaries->phaseRange().width() /
                        (boundaries->phaseCount() - 1);

    const double boxPhaseMin = _double(Inf(Im(projection)));
    const double boxPhaseMax = _double(Sup(Im(projection)));

    const double boundPhaseMin = classification.extremes()[2];
    const double boundPhaseMax = classification.extremes()[3];

    //Right strip (phases above the boundary maximum) certainly forbidden,
    //and wider than one grid step of the union.
    if (classification.isTopRightForbidden() && boundPhaseMax < boxPhaseMax - phaseStep) {
        cut = cutRightOfPhase(bounds, boundPhaseMax * qftbx::math::kPi / 180.0, phi0, w) || cut;
    }

    //Left strip (phases below the boundary minimum) certainly forbidden.
    if (classification.isBottomLeftForbidden() && boundPhaseMin > boxPhaseMin + phaseStep) {
        cut = cutLeftOfPhase(bounds, boundPhaseMin * qftbx::math::kPi / 180.0, phi0, w) || cut;
    }

    if (!cut) {
        return v;
    }

    return boxFromBounds(v.get(), bounds);
}


//Feasibility of the box with its gain range replaced by
//[gainInf, gainSup] at every design frequency.
bool AlgorithmMc1::gainRangeIsFeasible(LtiSystem * box,
                                                           double gainInf, double gainSup)
{
    const std::unique_ptr<LtiSystem> candidate = box->create(box->name(),
            box->numerator(), box->denominator(),
            Parameter("kv", Range(gainInf, gainSup), gainInf, "kv"),
            box->delay());

    bool feasibleEverywhere = true;

    for (std::size_t i = 0; i < omega->size() && feasibleEverywhere; ++i) {
        const cinterval projection = conversion->nicholsBox(candidate.get(), omega->at(i),
                                                      nominalPlantValues.at(i));
        feasibleEverywhere = detector->classifyBox(projection, boundaries, i).flag() == feasible;
    }

    return feasibleEverywhere;
}


//QS2 stage 3 (paper, algorithm 4): the largest upper gain subrange
//[k_f, sup k] certainly feasible at every design frequency. k_f is
//located by logarithmic bisection over the interval feasibility test and
//the certified point must pass the nominal stability criterion before it
//may prune the search through C.
void AlgorithmMc1::certifiedGainSearch(LtiSystem * box)
{
    if (!box->gain().isUncertain()) {
        return;
    }

    const double low = box->gain().range().min;
    double high = box->gain().range().max;

    if (low <= 0.0 || !gainRangeIsFeasible(box, high, high)) {
        return;
    }

    double lo = low;

    if (gainRangeIsFeasible(box, lo, high)) {
        high = lo;
    } else {
        double hi = high;

        while (hi / lo > m_settings.algorithms.certifiedGainTolerance) {
            const double mid = std::sqrt(lo * hi);

            if (gainRangeIsFeasible(box, mid, high)) {
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

    //The z' box, and its minimal-gain point as certified solution.
    const std::unique_ptr<LtiSystem> zPrime = box->create(box->name(),
            box->numerator(), box->denominator(),
            Parameter("kv", Range(high, box->gain().range().max), high, "kv"),
            box->delay());

    std::unique_ptr<LtiSystem> point = pointFromBox(zPrime.get(), true);

    if (stability->isNominallyStable(point.get())) {
        bestCertifiedGain = high;
        bestCertifiedController = std::move(point);
    }
}

} // namespace qftbx
