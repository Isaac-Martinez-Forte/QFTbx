#include <vector>
#include <cstdint>
#include "src/core/exception.h"
#include "src/core/loopshaping/algorithm_mc1.h"

#include "src/core/loopshaping/quick_solution.h"

using namespace tools;
using namespace cxsc;
using namespace FC;

namespace quick_solution = qftbx::quick_solution;

namespace {

//Relative tolerance of the stage-3 gain bisection: 1% locates the
//certified gain closely enough for pruning without spending the run time
//it is meant to save.
const double kCertifiedGainTolerance = 1.01;

//Step 3bis.(b) of the paper: cap the gain range of a box at the prune
//variable C. Returns the capped replacement (and destroys the original)
//or the box itself when the cap does not apply.
std::unique_ptr<LtiSystem> capGain(std::unique_ptr<LtiSystem> box, double cap)
{
    if (!box->gain().isUncertain() ||
            cap <= box->gain().range().min || cap >= box->gain().range().max) {
        return box;
    }

    return box->create(box->name(), box->numerator(), box->denominator(),
            Parameter("kv", Range(box->gain().range().min, cap),
                          box->gain().range().min, "kv"),
            box->delay());
}

//Nominal plant phase on the (-2 pi, 0] branch the Nichols boxes use.
double nominalPhase(std::complex<double> p0)
{
    double phi0 = std::arg(p0);

    if (phi0 > 0.0) {
        phi0 -= 2.0 * M_PI;
    }

    return phi0;
}

} // namespace


AlgorithmMc1::AlgorithmMc1()
{
}

AlgorithmMc1::~AlgorithmMc1()
{
}


void AlgorithmMc1::setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega,
                                          const BoundaryData * boundaries, double epsilon)
{
    this->plant = plant;
    this->controller = controller->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;

    hasUncertainZeros = false;
    for (Parameter & var : this->controller->numerator()) {
        hasUncertainZeros = hasUncertainZeros || var.isUncertain();
    }

    hasUncertainPoles = false;
    for (Parameter & var : this->controller->denominator()) {
        hasUncertainPoles = hasUncertainPoles || var.isUncertain();
    }
}


//Main loop: the NT branch & bound (paper, algorithm 5) with QS2 inside
//the feasibility test of every box (steps 1(b-bis) and 4bis) and the
//prune variable C of step 3bis behind bestCertifiedGain.
bool AlgorithmMc1::solve()
{
    liveList = std::make_unique<OrderedList>();
    conversion = std::make_unique<NaturalIntervalExtension>();
    detector = std::make_unique<BoundaryViolationDetector>();
    stability = std::make_unique<NominalStabilityChecker>(plant, omega);

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
        struct BisectionResult retur = bisectWidestParameter(node->system());

        //Steps 4bis-6: QS2 + feasibility + insertion.
        check_box_feasibility(std::move(retur.v1));
        check_box_feasibility(std::move(retur.v2));
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
inline void AlgorithmMc1::check_box_feasibility(std::unique_ptr<LtiSystem> box)
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
inline std::unique_ptr<LtiSystem> AlgorithmMc1::quickSolution2(std::unique_ptr<LtiSystem> v,
                                                             const BoxClassification & classification,
                                                             const cxsc::cinterval & projection,
                                                             double w, std::complex<double> p0)
{
    std::vector<double> zeroInfs, zeroSups, poleInfs, poleSups;
    for (Parameter & var : v->numerator()) {
        zeroInfs.push_back(var.isUncertain() ? var.range().min : var.nominal());
        zeroSups.push_back(var.isUncertain() ? var.range().max : var.nominal());
    }
    for (Parameter & var : v->denominator()) {
        poleInfs.push_back(var.isUncertain() ? var.range().min : var.nominal());
        poleSups.push_back(var.isUncertain() ? var.range().max : var.nominal());
    }

    double gainInf = v->gain().range().min;
    const double gainSup = v->gain().range().max;

    bool cut = false;

    //-------------------------------------------------- stage 1, magnitude
    //Sound only when the zone under every boundary point is certainly
    //forbidden, certified by the parity classification of the box's lower
    //corner (same gate as NK).
    if (classification.isBottomLeftForbidden()) {

        const double boundMin = std::pow(10.0, classification.extremes()[0] / 20.0);

        if (v->gain().isUncertain()) {
            const double k = quick_solution::gainCut(boundMin, zeroSups, poleInfs, w, p0);

            if (k > gainInf && k < gainSup) {
                gainInf = k;
                cut = true;
            }
        }

        if (hasUncertainZeros) {
            for (std::size_t j = 0; j < zeroInfs.size(); ++j) {
                if (!v->numerator()[j].isUncertain()) {
                    continue;
                }

                const double z = quick_solution::zeroCut(boundMin, gainSup, zeroSups,
                                                        poleInfs, j, w, p0);

                if (z > zeroInfs[j] && z < zeroSups[j]) {
                    zeroInfs[j] = z;
                    cut = true;
                }
            }
        }

        if (hasUncertainPoles) {
            for (std::size_t j = 0; j < poleInfs.size(); ++j) {
                if (!v->denominator()[j].isUncertain()) {
                    continue;
                }

                const double p = quick_solution::poleCut(boundMin, gainSup, zeroSups,
                                                        poleInfs, j, w, p0);

                if (p > poleInfs[j] && p < poleSups[j]) {
                    poleSups[j] = p;
                    cut = true;
                }
            }
        }
    }

    //------------------------------------------------------ stage 2, phase
    if (hasUncertainZeros || hasUncertainPoles) {

        const double phi0 = nominalPhase(p0);
        const double phaseStep = boundaries->phaseRange().width() /
                            (boundaries->phaseCount() - 1);

        const double boxPhaseMin = _double(Inf(Im(projection)));
        const double boxPhaseMax = _double(Sup(Im(projection)));

        const double boundPhaseMin = classification.extremes()[2];
        const double boundPhaseMax = classification.extremes()[3];

        //Right strip (phases above the boundary maximum) certainly
        //forbidden, and wider than one grid step of the union.
        if (classification.isTopRightForbidden() && boundPhaseMax < boxPhaseMax - phaseStep) {

            const double thetaMax = boundPhaseMax * M_PI / 180.0;

            for (std::size_t j = 0; hasUncertainZeros && j < zeroInfs.size(); ++j) {
                if (!v->numerator()[j].isUncertain()) {
                    continue;
                }

                const double z = quick_solution::zeroPhaseCutHigh(thetaMax, phi0, zeroSups,
                                                                 poleInfs, j, w);

                if (z > zeroInfs[j] && z < zeroSups[j]) {
                    zeroInfs[j] = z;
                    cut = true;
                }
            }

            for (std::size_t j = 0; hasUncertainPoles && j < poleInfs.size(); ++j) {
                if (!v->denominator()[j].isUncertain()) {
                    continue;
                }

                const double p = quick_solution::polePhaseCutHigh(thetaMax, phi0, zeroSups,
                                                                 poleInfs, j, w);

                if (p > poleInfs[j] && p < poleSups[j]) {
                    poleSups[j] = p;
                    cut = true;
                }
            }
        }

        //Left strip (phases below the boundary minimum) certainly
        //forbidden.
        if (classification.isBottomLeftForbidden() && boundPhaseMin > boxPhaseMin + phaseStep) {

            const double thetaMin = boundPhaseMin * M_PI / 180.0;

            for (std::size_t j = 0; hasUncertainZeros && j < zeroInfs.size(); ++j) {
                if (!v->numerator()[j].isUncertain()) {
                    continue;
                }

                const double z = quick_solution::zeroPhaseCutLow(thetaMin, phi0, zeroInfs,
                                                                poleSups, j, w);

                if (z > zeroInfs[j] && z < zeroSups[j]) {
                    zeroSups[j] = z;
                    cut = true;
                }
            }

            for (std::size_t j = 0; hasUncertainPoles && j < poleInfs.size(); ++j) {
                if (!v->denominator()[j].isUncertain()) {
                    continue;
                }

                const double p = quick_solution::polePhaseCutLow(thetaMin, phi0, zeroInfs,
                                                                poleSups, j, w);

                if (p > poleInfs[j] && p < poleSups[j]) {
                    poleInfs[j] = p;
                    cut = true;
                }
            }
        }
    }

    if (!cut) {
        return v;
    }

    std::vector<Parameter> numerador;
    for (std::size_t j = 0; j < zeroInfs.size(); ++j) {
        Parameter & old = v->numerator()[j];
        numerador.push_back(old.isUncertain()
                ? Parameter(old.name(), Range(zeroInfs[j], zeroSups[j]), zeroInfs[j])
                : Parameter(old.nominal()));
    }

    std::vector<Parameter> denominador;
    for (std::size_t j = 0; j < poleInfs.size(); ++j) {
        Parameter & old = v->denominator()[j];
        denominador.push_back(old.isUncertain()
                ? Parameter(old.name(), Range(poleInfs[j], poleSups[j]), poleInfs[j])
                : Parameter(old.nominal()));
    }

    return v->create(v->name(), numerador, denominador,
            v->gain().isUncertain()
                ? Parameter("kv", Range(gainInf, gainSup), gainInf, "kv")
                : Parameter(v->gain().nominal()),
            v->delay());
}


//Feasibility of the box with its gain range replaced by
//[gainInf, gainSup] at every design frequency.
inline bool AlgorithmMc1::gainRangeIsFeasible(LtiSystem * box,
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
inline void AlgorithmMc1::certifiedGainSearch(LtiSystem * box)
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

        while (hi / lo > kCertifiedGainTolerance) {
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
