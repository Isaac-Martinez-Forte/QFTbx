#include <vector>
#include <cstdint>
#include "src/core/exception.h"
#include "src/core/loopshaping/algorithm_nt.h"

using namespace tools;
using namespace cxsc;
using namespace FC;

/*
 * Algorithm NT (Nataraj-Tharewal): interval branch & bound QFT loop
 * shaping, faithful to Tharewal 2005 ("Automated Synthesis of QFT
 * Controllers and Prefilters using Interval Global Optimization
 * Techniques", IIT Bombay):
 *
 * - chapter 3 (sec. 3.3.3): the branch & bound over the controller
 *   parameter box with the live-node list NL ordered by ascending
 *   inf(k), so the first solution is the global optimum;
 * - chapter 5 (sec. 5.2.1): the constraint-propagation acceleration on
 *   the gain, using the monotonicity of |L0| with respect to k and the
 *   extreme boundary magnitudes B_min / B_max over the box's phase
 *   interval: the certainly infeasible gain subrange is cut off (C_g-)
 *   and the certainly feasible one is split into its own NL triple
 *   (C_g+).
 *
 * Termination follows ch. 3 (p. 29 and Remark 3.1): a feasible leading
 * box, or a leading box whose Nichols projection is smaller than epsilon
 * at every design frequency. In the second case, when the box is still
 * ambiguous, the returned point is the corner of the box that the
 * monotonicity of the projection makes feasible (the anti-blocking rule
 * of the QFTbx thesis, sec. 3.1).
 *
 * The feasibility test is completed with the nominal closed-loop
 * stability check of sec. 3.3.5, implemented on the Nichols chart by the
 * Cohen-Chait-Yaniv criterion (NominalStabilityChecker): the QFT bounds
 * alone do not exclude loops that encircle the critical point. The
 * historical code approximated this with a hard-coded ordering penalty
 * at 2 rad/s, retired by this review.
 */

void AlgorithmNt::setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> *omega, const BoundaryData * boundaries,
                                 double epsilon) {


    this->plant = plant;
    this->controller = controller->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;
}


//Main loop: Tharewal 2005, sec. 3.3.3 (steps 1-7).

bool AlgorithmNt::solve() {

    liveList = std::make_unique<OrderedList>(false, m_settings.search.maxLiveNodes);

    conversion = std::make_unique<NaturalIntervalExtension>();
    detector = std::make_unique<BoundaryViolationDetector>();
    stability = std::make_unique<NominalStabilityChecker>(plant, omega, m_settings.stability);

    nominalPlantValues.clear();

    for (double o : *omega) {
        std::complex <double> c = plant->evaluate(o);
        nominalPlantValues.push_back(cxsc::complex(c.real(), c.imag()));
    }

    //Step 1: feasibility of the initial search box (inserts it into NL
    //unless certainly infeasible).
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

        //Steps 2/6c: an empty list proves there is no feasible solution.
        if (liveList->isEmpty()) {
            throw qftbx::InvalidInput(
                    "No feasible solution exists in the given search box.");
        }

        std::unique_ptr<SearchNode> node = liveList->takeFirstAs<SearchNode>();


        //Step 3, termination: a feasible leading box (ch. 3, p. 29; its
        //lower gain corner realises the optimum), or a leading box below
        //the epsilon accuracy at every frequency (Remark 3.1; if still
        //ambiguous, the feasible corner is extracted).
        if (node->flag() == feasible || isEpsilonSmall(node->system(), this->epsilon, omega, conversion.get(), nominalPlantValues)) {
            if (node->flag() == ambiguous) {
                designedController = pointFromBox(node->system(), false);

                //The anti-blocking corner is a fresh point: it must pass
                //the nominal stability criterion too. If it does not,
                //this node yields no solution and the search continues.
                if (!stability->isNominallyStable(designedController.get())) {
                    designedController.reset();
                    continue;
                }
            } else {
                //The lower corner of a feasible box was already certified
                //when the box entered the list.
                designedController = pointFromBox(node->system(), true);
            }

            //Everything else dies with the algorithm (see the destructor).
            return true;
        }

        //Step 4: bisect along the widest parameter direction.
        BisectionResult halves = bisectWidestParameter(node->system());

        //Steps 5-6: classify the subboxes and insert them in NL.
        check_box_feasibility(std::move(halves.v1));
        check_box_feasibility(std::move(halves.v2));
    }
}


std::size_t AlgorithmNt::peakLiveNodes() const
{
    return liveList != nullptr ? liveList->peakSize() : 0;
}


std::unique_ptr<LtiSystem> AlgorithmNt::controllerStructure() {
    return std::move(designedController);
}


//Feasibility test of one box over every design frequency (Tharewal 2005,
//sec. 3.3.4-3.3.5) plus the ch. 5 gain acceleration. Certainly infeasible
//boxes are destroyed; anything else is inserted into NL ordered by
//inf(k). When the certainly feasible gain subrange [feasibleFrom, sup(k)]
//can be split off (C_g+), it is re-certified by this same test and
//enters NL as its own triple.

void AlgorithmNt::check_box_feasibility(std::unique_ptr<LtiSystem> box) {

    BoxClassification classification;

    BoxFlag flag_final = feasible;

    std::size_t frequencyIndex = 0;
    cinterval projection;

    //C_g+ : the certainly feasible gain subrange must satisfy EVERY
    //frequency (intersection), so the candidate is the maximum of the
    //per-frequency lower limits and fails if any ambiguous frequency
    //cannot certify one.
    double feasibleFrom = 0;
    bool feasibleCertified = true;

    for (double o : *omega) {

        projection = conversion->nicholsBox(box.get(), o, nominalPlantValues.at(frequencyIndex));

        classification = detector->classifyBox(projection, boundaries, frequencyIndex);

        if (classification.flag() == infeasible) {
            return;
        }

        if (classification.flag() == ambiguous) {
            flag_final = ambiguous;

            const double minBoundary = classification.extremes()[0];
            const double maxBoundary = classification.extremes()[1];

            //C_g- : cut the certainly infeasible low-gain subrange.
            box = accelerated(std::move(box), minBoundary, o, frequencyIndex,
                             !classification.isBottomLeftForbidden());

            //C_g+ : candidate lower limit of the certainly feasible
            //high-gain subrange at this frequency.
            if (feasibleCertified) {
                double from;
                if (feasibleGainFrom(box.get(), maxBoundary, projection, o, frequencyIndex, from)) {
                    feasibleFrom = std::max(feasibleFrom, from);
                } else {
                    feasibleCertified = false;
                }
            }
        }

        frequencyIndex++;
    }

    //C_g+ split (Tharewal 2005, sec. 5.2.1-5.2.2): the candidate feasible
    //part becomes its own box and is re-certified by this same test, so
    //the split never depends on the heuristic gate for correctness. The
    //margins skip degenerate slivers that would only bloat the list.
    const double kInf = box->gain().range().min;
    const double kSup = box->gain().range().max;

    //Nominal closed-loop stability of bounds-feasible boxes (Tharewal
    //2005, sec. 3.3.5, by the Nichols-chart Nyquist criterion): satisfied
    //stability bounds plus one nominally stable point make the whole box
    //robustly stable; an unstable point discards it entirely.
    if (flag_final == feasible) {
        const std::unique_ptr<LtiSystem> point = pointFromBox(box.get(), true);

        if (!stability->isNominallyStable(point.get())) {
            return;
        }
    }

    if (flag_final == ambiguous && feasibleCertified &&
            feasibleFrom > kInf * 1.01 && feasibleFrom < kSup * 0.99) {

        //Deep copy for the feasible part, with its own gain interval.
        const std::unique_ptr<LtiSystem> base = box->clone();

        check_box_feasibility(base->create(base->name(), base->numerator(),
                base->denominator(),
                Parameter("kv", Range(feasibleFrom, kSup), feasibleFrom, "kv"),
                base->delay()));

        //The current box keeps the remaining ambiguous gain subrange.
        box = box->create(box->name(), box->numerator(), box->denominator(),
                Parameter("kv", Range(kInf, feasibleFrom), kInf, "kv"),
                box->delay());
    }

    //The index is read BEFORE the box is handed over: as arguments of one
    //call their evaluation order is unspecified.
    const double gainInf = box->gain().range().min;

    liveList->insert(std::make_unique<SearchNode>(gainInf, std::move(box), flag_final));

}


//Geometric contractor C_g- (Tharewal 2005, ch. 5, Algorithm C_g-): using
//the monotonicity of |L0| w.r.t. the gain, remove the gain subrange
//[inf(k), k_B] whose boxes lie entirely below B_min, the minimum boundary
//magnitude over the box's phase interval. The cut only applies when the
//below-everything zone is certainly forbidden, certified by the parity
//classification of the box's lower corner (above == false).

std::unique_ptr<LtiSystem> AlgorithmNt::accelerated(std::unique_ptr<LtiSystem> v,
        double minBoundary, double o, std::size_t frequencyIndex, bool above) {

    if (!above){

        const double minGainLinear = v->gain().range().min;
        const double minGainDb = 20 * log10(minGainLinear);

        const std::unique_ptr<LtiSystem> lowGainBox = v->create(v->name(), v->numerator(),
                v->denominator(), Parameter(minGainLinear), v->delay());

        double magnitudeAtMinGainDb = _double(SupRe(conversion->nicholsBox(lowGainBox.get(), o,
                nominalPlantValues.at(frequencyIndex))));


        if (magnitudeAtMinGainDb < minBoundary) {

            //k_B = inf(k) + (B_min - sup|L0(inf(k))|), in dB.
            double cutGainDb = minGainDb + (minBoundary - magnitudeAtMinGainDb);

            double cutGainLinear = pow(10, cutGainDb / 20);

            v = v->create(v->name(), v->numerator(), v->denominator(),
                    Parameter("kv", Range(cutGainLinear, v->gain().range().max), cutGainLinear, "kv"),
                    v->delay());
        }
    }

    return v;
}


//Geometric contractor C_g+ (Tharewal 2005, ch. 5): lower limit of the
//gain subrange whose boxes lie entirely above B_max, the maximum boundary
//magnitude over the box's phase interval. Returns false when no part of
//the gain range can be certified feasible at this frequency. The zone
//above every boundary point must be an allowed zone, checked by the
//parity classification of a probe point just above B_max at the centre
//of the box's phase interval (a heuristic gate: the caller re-certifies
//the split box with the full feasibility test).

bool AlgorithmNt::feasibleGainFrom(LtiSystem * v, double maxBoundary,
                                   cinterval projection, double o, std::size_t frequencyIndex, double & from) {

    const double phaseCentre = (_double(InfIm(projection)) + _double(SupIm(projection))) / 2.0;

    if (detector->classifyPoint(qftbx::NicholsPoint(phaseCentre, maxBoundary + 1.0),
                                   boundaries, frequencyIndex) != feasible) {
        return false;
    }

    const double maxGainLinear = v->gain().range().max;
    const double maxGainDb = 20 * log10(maxGainLinear);

    const std::unique_ptr<LtiSystem> highGainBox = v->create(v->name(), v->numerator(),
            v->denominator(), Parameter(maxGainLinear), v->delay());

    double magnitudeAtMaxGainDb = _double(InfRe(conversion->nicholsBox(highGainBox.get(), o,
            nominalPlantValues.at(frequencyIndex))));

    if (magnitudeAtMaxGainDb <= maxBoundary) {
        return false;
    }

    //k_F = sup(k) - (inf|L0(sup(k))| - B_max), in dB.
    const double feasibleGainDb = maxGainDb - (magnitudeAtMaxGainDb - maxBoundary);

    from = pow(10, feasibleGainDb / 20);

    return true;
}
