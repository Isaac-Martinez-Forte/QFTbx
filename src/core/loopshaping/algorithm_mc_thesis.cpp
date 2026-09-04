#include <vector>
#include "src/core/math/constants.h"
#include <cstdint>
#include "src/core/common/exception.h"
#include "src/core/loopshaping/algorithm_mc_thesis.h"

using namespace cxsc;

namespace quick_solution = qftbx::quick_solution;

namespace qftbx {

namespace {

//Corner value vectors of a box (uncertain parameters at the requested
//extreme, fixed ones at their nominal).
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

} // namespace


void AlgorithmMcThesis::setStrategies(const Strategies & s)
{
    strategies = s;
}


void AlgorithmMcThesis::setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega,
                                  const BoundaryData * boundaries, double epsilon)
{
    this->plant = plant;
    this->controller = controller->clone();
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


//--------------------------------------------------------- parameter access
//Uniform view of the controller parameters: 0 is the gain, then the
//zeros, then the poles (the thesis' x vector).

inline std::int32_t AlgorithmMcThesis::parameterCount(LtiSystem * box) const
{
    return static_cast<std::int32_t>(1 + box->numerator().size() + box->denominator().size());
}

Range AlgorithmMcThesis::parameterRange(LtiSystem * box, std::int32_t parameter) const
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

//New box with one parameter's range replaced (deep copy, the original is
//left untouched).
std::unique_ptr<LtiSystem> AlgorithmMcThesis::replaceParameter(LtiSystem * box, std::int32_t parameter,
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
            ? Parameter("kv", range, range.min, "kv")
            : box->gain();

    return box->create(box->name(), std::move(numerator), std::move(denominator),
                       std::move(gain), box->delay());
}


//------------------------------------------------------------ main loop
//Thesis 5.4, algorithm MC: branch & bound over the live list ordered by
//ascending gain infimum, with the prune variable C, the execution stages
//and the cutting/bisection strategies wired per the pseudocode.
bool AlgorithmMcThesis::solve()
{
    liveList = std::make_unique<OrderedList>(false, m_settings.search.maxLiveNodes);
    conversion = std::make_unique<NaturalIntervalExtension>();
    detector = std::make_unique<BoundaryViolationDetector>();
    stability = std::make_unique<NominalStabilityChecker>(plant, omega, m_settings.stability);

    bestCertifiedGain = std::numeric_limits<double>::infinity();
    bestCertifiedController.reset();

    nominalPlantValues.clear();
    nominalPlantValuesStd.clear();

    for (double o : *omega) {
        std::complex<double> c = plant->evaluate(o);
        nominalPlantValuesStd.push_back(c);
        nominalPlantValues.push_back(cxsc::complex(c.real(), c.imag()));
    }

    //A controller with no uncertain parameter offers nothing to search.
    if (!hasUncertainZeros && !hasUncertainPoles && !controller->gain().isUncertain()) {
        designedController = pointFromBox(controller.get(), true);
        return false;
    }

    //Step A/B: the initial box enters the list; its feasibility test
    //happens when it is popped (step D).
    //The index is read BEFORE the box is handed over: as arguments of one
    //call their evaluation order is unspecified.
    const double initialGainInf = controller->gain().range().min;

    auto initial = std::make_unique<McSearchNode>(initialGainInf, std::move(controller),
                                                 ambiguous);
    initial->setStage(strategies.stages ? Stage::Initial : Stage::Intermediate);
    initial->setCutsEnabled(true);
    liveList->insert(std::move(initial));

    while (true) {

        //Once per node: the cheapest possible place to notice, and the only
        //one that bounds how long a cancellation takes to take effect. It
        //throws rather than returning false, because false already means
        //"searched everything and found nothing", which is a different
        //answer and one the caller reports differently.
        if (qftbx::cancellationAsked(m_cancellation)) {
            throw qftbx::Cancelled();
        }

        //Step C: pop, prune and cap with C.
        if (liveList->isEmpty()) {
            //The certified solution of MG stands in when the search
            //exhausts the space (the thesis pseudocode reports "no
            //solution" here even when C holds one; returning it is the
            //sound completion).
            if (bestCertifiedController != nullptr) {
                designedController = std::move(bestCertifiedController);
                return true;
            }

            throw qftbx::InvalidInput(
                    "No feasible solution exists in the given search box.");
        }

        std::unique_ptr<McSearchNode> node = liveList->takeFirstAs<McSearchNode>();

        //Strict comparison: a node whose infimum EQUALS C still realises
        //the certified optimum (thesis 5.4.3 prescribes < over <=).
        if (bestCertifiedGain < node->system()->gain().range().min) {
            continue;
        }

        node->setSystem(capGain(node->releaseSystem(), bestCertifiedGain));

        //A feasible node is a solution: its gain infimum corner realises
        //the optimum of the box (stability was certified at insertion).
        if (node->flag() == feasible) {
            designedController = pointFromBox(node->system(), true);
            return true;
        }

        //Step D: feasibility test of the current box.
        NodeAnalysis analysis;
        if (!analyse(node.get(), analysis)) {
            continue;   //certainly infeasible: the node dies with the scope
        }

        if (analysis.flag == feasible) {
            const PointController corner = cornerOf(node->system(), true);

            if (!stability->isNominallyStable(corner)) {
                continue;
            }

            designedController = systemFromPoint(node->system(), corner);
            return true;
        }

        //Termination on the epsilon-small leading box (thesis 3.3, the
        //solution function): the returned point is unverified, so it must
        //pass the stability criterion, as reviewed for NT.
        if (isEpsilonSmall(node->system(), epsilon, omega, conversion.get(), nominalPlantValues)) {
            const PointController corner = cornerOf(node->system(), false);

            if (!stability->isNominallyStable(corner)) {
                continue;
            }

            designedController = systemFromPoint(node->system(), corner);
            return true;
        }

        //Steps E-F: stage bookkeeping, MG, QSFact, QSInv.
        std::vector<FeasibleThreshold> thresholds;
        improveNode(node.get(), analysis, thresholds);

        //C may have improved inside F.
        if (bestCertifiedGain < node->system()->gain().range().min) {
            continue;
        }

        //Steps G-H: bisect and insert the children.
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


std::size_t AlgorithmMcThesis::peakLiveNodes() const
{
    return liveList != nullptr ? liveList->peakSize() : 0;
}


std::unique_ptr<LtiSystem> AlgorithmMcThesis::controllerStructure()
{
    return std::move(designedController);
}


//------------------------------------------------------- feasibility test
//Step D: one detection per design frequency (skipping the frequencies
//the node history already certifies as feasible), collecting the data
//the cutting stages and the bisection need. Returns false when some
//frequency is certainly infeasible; the node is the caller's to drop.
bool AlgorithmMcThesis::analyse(McSearchNode * node, NodeAnalysis & out)
{
    out.flag = feasible;
    out.mainFrequency = 0;
    out.anyFullPhaseWidth = false;

    double largestArea = std::numeric_limits<double>::lowest();

    for (std::size_t i = 0; i < omega->size(); ++i) {

        if (node->isFrequencyFeasible(i)) {
            out.classification.push_back(std::nullopt);
            out.boxMag.push_back(Range());
            out.boxPhase.push_back(Range());
            continue;
        }

        const cinterval projection = conversion->nicholsBox(node->system(), omega->at(i),
                                                      nominalPlantValues.at(i));

        BoxClassification classification = detector->classifyBox(projection, boundaries, i);

        //Read BEFORE the move: the verdict is needed again below, and a
        //moved-from object is only guaranteed to be valid, not to still
        //hold anything. It does today because every member of a
        //BoxClassification is trivially copyable, so the move is a copy -
        //which is exactly the kind of thing that stops being true the day
        //someone adds a container to it.
        const BoxFlag verdict = classification.flag();

        if (verdict == infeasible) {
            return false;
        }

        out.classification.push_back(std::move(classification));
        out.boxMag.push_back(Range(_double(Inf(Re(projection))), _double(Sup(Re(projection)))));
        out.boxPhase.push_back(Range(_double(Inf(Im(projection))), _double(Sup(Im(projection)))));

        const double phaseWidth = _double(diam(Im(projection)));

        if (phaseWidth >= phaseSpanWidth - phaseGridStep) {
            out.anyFullPhaseWidth = true;
        }

        if (verdict == ambiguous) {
            out.flag = ambiguous;

            const double area = _double(diam(Re(projection))) * phaseWidth;
            if (area > largestArea) {
                largestArea = area;
                out.mainFrequency = i;
            }
        }
    }

    return true;
}


//--------------------------------------------------------------- steps E-F
void AlgorithmMcThesis::improveNode(McSearchNode * node, NodeAnalysis & analysis,
                                           std::vector<FeasibleThreshold> & thresholds)
{
    //Step E (thesis 4.4): the initial stage ends when no projected box
    //spans the full phase width of the Nichols plane any more.
    if (strategies.stages &&
            node->stage() == Stage::Initial && !analysis.anyFullPhaseWidth) {
        node->setStage(Stage::Intermediate);
    }

    if (!node->cutsEnabled()) {
        return;
    }

    //Step F: MG first; QSFact only when MG finds nothing (thesis 5.1.2:
    //they overlap in purpose); QSInv always.
    bool improved = false;

    if (strategies.bestGain && bestGainSearch(node, analysis)) {
        improved = true;
    } else if (strategies.feasibleMagnitude || strategies.feasiblePhase) {
        feasibleCuts(node, analysis, thresholds, improved);
    }

    if (strategies.infeasibleMagnitude || strategies.infeasiblePhase) {
        infeasibleCuts(node, analysis, improved);
    }

    //The final stage begins when a full pass yields nothing (thesis 4.4):
    //the cuts are disabled from here on for this node and its children.
    if (strategies.stages && !improved && node->stage() == Stage::Intermediate) {
        node->setStage(Stage::Final);
        node->setCutsEnabled(false);
    }
}


//Feasibility of a box (or point) at one design frequency, and at all of
//them: the defensive verification of everything the closed-form
//certificates produce (MG candidates, UM/UF boxes, tree-bisection
//marks). An equation slip then costs a missed acceleration, never a
//wrong verdict.
bool AlgorithmMcThesis::boxIsFeasibleAt(LtiSystem * box, std::size_t freqIndex)
{
    const cinterval projection = conversion->nicholsBox(box, omega->at(freqIndex),
                                                  nominalPlantValues.at(freqIndex));
    return detector->classifyBox(projection, boundaries, freqIndex).flag() == feasible;
}

bool AlgorithmMcThesis::boxIsFeasible(LtiSystem * box)
{
    for (std::size_t i = 0; i < omega->size(); ++i) {
        if (!boxIsFeasibleAt(box, i)) {
            return false;
        }
    }

    return true;
}

bool AlgorithmMcThesis::pointIsFeasible(const PointController & point)
{
    for (std::size_t i = 0; i < omega->size(); ++i) {
        const cinterval projection = conversion->nicholsPoint(point, omega->at(i),
                                                              nominalPlantValues.at(i));

        if (detector->classifyBox(projection, boundaries, i).flag() != feasible) {
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------- MG
//Best-gain search (thesis 4.3 and 5.2): with the other parameters fixed
//at the corner that maximises the controller magnitude (zeros sup, poles
//inf), each ambiguous frequency yields a closed-form gain threshold on
//its feasible side; their intersection is the best certified gain of the
//box. The candidate is verified against the feasibility test and the
//stability criterion before it may prune through C (the thesis relies on
//the strip geometry alone; the extra checks cost |Omega| detections).
bool AlgorithmMcThesis::bestGainSearch(McSearchNode * node, const NodeAnalysis & analysis)
{
    LtiSystem * box = node->system();

    if (!box->gain().isUncertain()) {
        return false;
    }

    std::vector<double> zeroSups, poleInfs;
    cornerVectors(box, true, false, zeroSups, poleInfs);

    const double kInf = box->gain().range().min;
    const double kSup = box->gain().range().max;

    double lowNeeded = kInf;    //k must be >= (top-side feasible strips)
    double highAllowed = kSup;  //k must be <= (bottom-side feasible strips)

    for (std::size_t i = 0; i < omega->size(); ++i) {

        const std::optional<BoxClassification> & classification =
                analysis.classification.at(i);

        if (!classification.has_value() || classification->flag() != ambiguous) {
            continue;   //the whole box, corner included, is feasible here
        }

        const double w = omega->at(i);
        const std::complex<double> p0 = nominalPlantValuesStd.at(i);
        const double boundMin = std::pow(10.0, classification->extremes()[0] / 20.0);
        const double boundMax = std::pow(10.0, classification->extremes()[1] / 20.0);

        //Preferring the bottom strip serves the objective (it allows the
        //gain infimum); the top strip is the fallback.
        bool constrained = false;

        if (!classification->isBottomLeftForbidden()) {   //strip under B_min certainly feasible
            const double t = quick_solution::gainCut(boundMin, zeroSups, poleInfs, w, p0);

            if (t >= kInf) {
                highAllowed = std::min(highAllowed, t);
                constrained = true;
            }
        }

        if (!constrained && !classification->isTopRightForbidden()) {   //strip over B_max feasible
            const double t = quick_solution::gainCut(boundMax, zeroSups, poleInfs, w, p0);

            if (t <= kSup && t > 0.0) {
                lowNeeded = std::max(lowNeeded, t);
                constrained = true;
            }
        }

        if (!constrained) {
            return false;   //this frequency cannot be certified at the corner
        }
    }

    if (lowNeeded > highAllowed || lowNeeded >= bestCertifiedGain) {
        return false;
    }

    //The certified point: gain at the intersection infimum, the other
    //parameters at the corner (thesis 4.3: the solution is a POINT; its
    //pseudocode substitutes into the whole box, an erratum).
    const PointController point{lowNeeded, std::move(zeroSups), std::move(poleInfs)};

    if (!pointIsFeasible(point) || !stability->isNominallyStable(point)) {
        return false;
    }

    bestCertifiedGain = lowNeeded;
    bestCertifiedController = systemFromPoint(box, point);

    return true;
}


//------------------------------------------------------------------ QSFact
//Insertion of a certainly feasible box into the live list, guarded by the
//prune variable and the stability criterion.
void AlgorithmMcThesis::insertFeasibleBox(std::unique_ptr<LtiSystem> box,
                                                McSearchNode * parent)
{
    const double gainInf = box->gain().range().min;

    if (gainInf > bestCertifiedGain) {
        return;
    }

    const PointController point = cornerOf(box.get(), true);

    if (!stability->isNominallyStable(point)) {
        return;
    }

    //A feasible box also certifies its own gain infimum: it feeds C like
    //an MG solution (the thesis keeps both mechanisms; folding them keeps
    //one prune variable).
    if (gainInf < bestCertifiedGain) {
        bestCertifiedGain = gainInf;
        bestCertifiedController = systemFromPoint(box.get(), point);
    }

    auto t = std::make_unique<McSearchNode>(gainInf, std::move(box), feasible);
    t->setStage(parent->stage());
    t->setCutsEnabled(false);
    liveList->insert(std::move(t));
}


//QSFact (thesis 5.1.2): certainly feasible subranges of every parameter.
//For each parameter, each family (magnitude/phase) and each side of the
//range, every ambiguous frequency must certify a threshold with the
//closed-form equations at the corner that puts the loop CLOSEST to the
//boundary (the quantification runs over all values of the other
//parameters); frequencies where the box is feasible impose nothing. The
//intersection across frequencies (UM/UF) is split off into the live list
//and every valid per-frequency threshold is recorded for the tree
//bisection (MM/MF).
void AlgorithmMcThesis::feasibleCuts(McSearchNode * node, const NodeAnalysis & analysis,
                                            std::vector<FeasibleThreshold> & thresholds, bool & improved)
{
    LtiSystem * box = node->system();
    const std::int32_t total = parameterCount(box);

    //family 0 = magnitude, 1 = phase; side true = upper subrange.
    for (std::int32_t parameter = 0; parameter < total; ++parameter) {

        const Range range = parameterRange(box, parameter);

        if (range.min >= range.max) {
            continue;   //fixed parameter
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

                //Corner vectors are refreshed per attempt: earlier
                //extractions may have shrunk the box.
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
                        continue;   //feasible here for the whole range
                    }

                    const double w = omega->at(i);
                    const std::complex<double> p0 = nominalPlantValuesStd.at(i);

                    double t = -1.0;

                    if (family == 0) {
                        const double boundMin = std::pow(10.0, classification->extremes()[0] / 20.0);
                        const double boundMax = std::pow(10.0, classification->extremes()[1] / 20.0);

                        //Which boundary side must be feasible follows the
                        //parameter's monotonicity: gain and zeros raise
                        //the loop, poles lower it (the upper subrange of
                        //a pole lives on the bottom strip).
                        const bool topStrip = (isGain || isZero) ? upperSide : !upperSide;

                        if (topStrip) {
                            if (classification->isTopRightForbidden()) {   //top strip forbidden
                                allCertified = false;
                                break;
                            }
                            //Others at the loop-minimising corner.
                            if (isGain) {
                                t = quick_solution::gainCut(boundMax, zeroInfs, poleSups, w, p0);
                            } else if (isZero) {
                                t = quick_solution::zeroCut(boundMax, kInf, zeroInfs, poleSups, termIndex, w, p0);
                            } else {
                                t = quick_solution::poleCut(boundMax, kInf, zeroInfs, poleSups, termIndex, w, p0);
                            }
                        } else {
                            if (classification->isBottomLeftForbidden()) {     //bottom strip forbidden
                                allCertified = false;
                                break;
                            }
                            //Others at the loop-maximising corner.
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

                        //Zeros lower the phase as they grow, poles raise
                        //it: the upper subrange of a zero lives on the
                        //LEFT strip, of a pole on the RIGHT strip.
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

                    //A threshold beyond the range on the certifying side
                    //imposes nothing at this frequency; one beyond the
                    //other side leaves no feasible subrange.
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

                //Strictly interior intersection: split the feasible
                //subrange off into the live list (UM/UF) and keep the
                //ambiguous remainder in the node.
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

                //Defensive verification with the real detection before
                //trusting the closed-form certificate.
                if (!boxIsFeasible(um.get())) {
                    continue;
                }

                insertFeasibleBox(std::move(um), node);

                std::unique_ptr<LtiSystem> remainder = replaceParameter(box, parameter,
                                                                       ambiguousPart);
                box = remainder.get();
                node->setSystem(std::move(remainder));

                improved = true;
            }
        }
    }
}


//------------------------------------------------------------------- QSInv
//QSInv (thesis 5.1.1): certainly infeasible subranges cut away, on every
//side of the projected box the corner classification certifies as
//forbidden: the magnitude cuts of NK's Quick Solution (bottom strip, plus
//their mirror on the top strip) and the phase cuts of thesis 4.1.2. All
//cuts run sequentially on the latest updated values.
void AlgorithmMcThesis::infeasibleCuts(McSearchNode * node, const NodeAnalysis & analysis,
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
        const std::complex<double> p0 = nominalPlantValuesStd.at(i);
        const double boundMin = std::pow(10.0, classification->extremes()[0] / 20.0);
        const double boundMax = std::pow(10.0, classification->extremes()[1] / 20.0);

        //Bottom strip certainly forbidden: cuts from below (NK's QS).
        if (strategies.infeasibleMagnitude && classification->isBottomLeftForbidden()) {
            cut = cutBelowBoundary(bounds, boundMin, w, p0) || cut;
        }

        //Top strip certainly forbidden: the mirror cuts from above, with
        //the loop-minimising corner and B_max.
        if (strategies.infeasibleMagnitude && classification->isTopRightForbidden()) {
            cut = cutAboveBoundary(bounds, boundMax, w, p0) || cut;
        }

        //Phase strips (thesis 4.1.2), when wider than one grid step.
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


//-------------------------------------------------------------- bisection
//Split one parameter at 'point'; both children inherit the node's stage,
//cut switch and feasible-frequency history.
qftbx::McBisectionResult AlgorithmMcThesis::bisectAt(McSearchNode * node, std::int32_t parameter,
                                                         double point)
{
    LtiSystem * box = node->system();
    const Range range = parameterRange(box, parameter);

    std::unique_ptr<LtiSystem> lower = replaceParameter(box, parameter,
                                                       Range(range.min, point));
    std::unique_ptr<LtiSystem> upper = replaceParameter(box, parameter,
                                                       Range(point, range.max));

    const auto makeChild = [&](std::unique_ptr<LtiSystem> system) {
        //The index is read BEFORE the box is handed over: as arguments of
        //one call their evaluation order is unspecified.
        const double gainInf = system->gain().range().min;

        auto t = std::make_unique<McSearchNode>(gainInf, std::move(system), ambiguous);
        t->setStage(node->stage());
        t->setCutsEnabled(node->cutsEnabled());
        t->setFeasibleFrequencies(node->feasibleFrequencies());
        return t;
    };

    qftbx::McBisectionResult children;
    children.t1 = makeChild(std::move(lower));
    children.t2 = makeChild(std::move(upper));

    //The bisected node is the caller's: it dies with the loop iteration.
    return children;
}


//The parameter whose Nichols term box at the main frequency contributes
//most by the requested measure (0 = area, 1 = magnitude, 2 = phase).
//The gain has no phase component, so measure 2 skips it and measures 0/1
//use its magnitude width (its phase width is zero).
inline std::int32_t AlgorithmMcThesis::widestByMeasure(McSearchNode * node, std::size_t mainFrequency, int measure)
{
    LtiSystem * box = node->system();
    const double w = omega->at(mainFrequency);
    const cxsc::complex p0 = nominalPlantValues.at(mainFrequency);

    std::int32_t best = -1;
    double bestValue = -1.0;

    const auto consider = [&](std::int32_t parameter, const cinterval & term, bool gainTerm) {
        double value;

        if (measure == 2) {
            if (gainTerm) {
                return;
            }
            value = _double(diam(Im(term)));
        } else if (measure == 1 || gainTerm) {
            value = _double(diam(Re(term)));
        } else {
            value = _double(diam(Re(term))) * _double(diam(Im(term)));
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


//Step G (thesis 5.4.6): the bisection strategy follows the node's stage.
qftbx::McBisectionResult AlgorithmMcThesis::bisect(McSearchNode * node, const NodeAnalysis & analysis,
                                                       const std::vector<FeasibleThreshold> & thresholds)
{
    //Tree bisection (thesis 5.3): split at the stored feasible threshold
    //covering the largest fraction of its parameter's current range, and
    //mark the feasible child for that frequency.
    if (strategies.treeBisection &&
            node->stage() == Stage::Intermediate && !thresholds.empty()) {

        const FeasibleThreshold * bestThreshold = nullptr;
        double bestFraction = 0.0;

        for (const FeasibleThreshold & t : thresholds) {
            const Range range = parameterRange(node->system(), t.parameter);

            if (t.threshold <= range.min || t.threshold >= range.max) {
                continue;   //the range moved since the threshold was recorded
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

            //Defensive verification of the mark with the real detection:
            //an unverified mark would silently skip this frequency's
            //feasibility test in the whole subtree.
            if (boxIsFeasibleAt(feasibleChild->system(), freq)) {
                feasibleChild->markFrequencyFeasible(freq, omega->at(freq));
            }

            return children;
        }
    }

    //Stage-driven measure: area in the initial stage (and as the general
    //fallback), the wider of magnitude/phase in the final stage.
    int measure = 0;

    if (node->stage() == Stage::Final) {
        const Range magnitude = analysis.boxMag.at(analysis.mainFrequency);
        const Range phase = analysis.boxPhase.at(analysis.mainFrequency);
        measure = phase.width() > magnitude.width() ? 2 : 1;
    }

    std::int32_t parameter = widestByMeasure(node, analysis.mainFrequency, measure);

    if (parameter < 0) {
        parameter = widestByMeasure(node, analysis.mainFrequency, 0);
    }

    const Range range = parameterRange(node->system(), parameter);

    return bisectAt(node, parameter, range.middle());
}

} // namespace qftbx
