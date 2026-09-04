#include <string>
#include <vector>
#include <cstdint>
#include "src/core/exception.h"
#include "src/core/loopshaping/algorithm_mr.h"

#include "src/core/text_tokens.h"

#include "src/core/specifications/specification_record.h"

#include <cmath>

using namespace tools;
using namespace cxsc;
using namespace FC;
using namespace alg;

namespace {

//Template representatives per frequency entering the constraint set (the
//paper uses 9 plants; the tracking constraints pair them quadratically).
//
//KNOWN AND ACCEPTED LIMIT, measured on the FDA-10 Example 5.1 fixture over a
//51x51 sweep of the uncertainty: the design this certifies exceeds the TRUE
//tracking bound by 12% to 19% at two of the five design frequencies
//(0.456 dB of spread against 0.408 allowed at w = 0.25; 1.81e-3 against
//1.52e-3 at w = 0.015). Nine points spread evenly along the contour, paired
//quadratically, is simply too coarse for this problem.
//
//What was ruled out, with numbers:
//  - Raising the count to 25 shrinks the excess (0.456 -> 0.431 dB) without
//    removing it, at twelve times the cost (5 s -> 64 s).
//  - The contour epsilon is NOT the cause: with 10 and with 2 the design
//    comes out identical, and with 0.5 there is no hull at all.
//
//The way OUT of it, and why it is not done here: the excess exists because
//the spread of |T| is SAMPLED. Bounding it instead - evaluating |T| as an
//interval over the template's contour, edge by edge, the way every other
//quantity in this file is bounded - would give a guaranteed enclosure and no
//discretisation gap at all, and it need not even be dearer: one interval
//evaluation per box against the 9x8 ordered pairs of today. But that changes
//what the algorithm COMPUTES, and it departs from the constraint set the
//article formulates. It is a modelling decision for the thesis, not a repair
//of this code, so this stays as the paper has it, with the gap written down.
const std::int32_t kTemplateRepresentatives = 9;

//Passes of the HC4 fixpoint loop per box (a bound protects against
//oscillating contractions; convergence is typically immediate).
const std::int32_t kMaxNarrowingPasses = 8;

std::string number(double value)
{
    //Through the shared primitive: it round-trips, so the lexer parses back
    //exactly this double, and it is far shorter than the 'g',17 this used
    //to ask for - these strings are built by the thousand.
    return (qftbx::text::number(value));
}

} // namespace

AlgorithmMr::AlgorithmMr()
{
}

AlgorithmMr::~AlgorithmMr()
{
}

void AlgorithmMr::setProblem(LtiSystem *plant, LtiSystem *controller, std::vector<double> * omega, const BoundaryData *boundaries,
                                  double epsilon, const qftbx::CloudSet & temp,
                                  const qftbx::SpecificationRecords * specificationRecords){
    this->plant = plant;
    this->controller = controller->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;
    this->temp = temp;
    this->specificationRecords = specificationRecords;
}


//Controller magnitude and phase as expression strings over the uncertain
//parameter names, one pair per design frequency. A zero-pole-gain factor
//(jw + x) contributes sqrt(x^2 + w^2) to the magnitude; a time-constant
//factor (1 + jw/x) contributes sqrt(1 + w^2/x^2); both contribute
//atan(w/x) to the phase in radians (the historical builder emitted
//atan(x/w), the complement of the true phase).
inline void AlgorithmMr::buildControllerExpressions(){

    const bool timeConstant =
            controller->type() == LtiSystem::SystemType::TimeConstantGain;

    if (!timeConstant && controller->type() != LtiSystem::SystemType::ZeroPoleGain) {
        throw qftbx::InvalidInput(
                "The ICSP loop-shaping algorithm needs a zero-pole-gain or "
                "time-constant controller structure.");
    }

    const auto term = [&](Parameter & var, double w) -> std::string {
        const std::string value = var.isUncertain() ? var.name() : number(var.nominal());
        if (timeConstant) {
            return "sqrt(1+(" + number(w * w) + "/(" + value + "^2)))";
        }
        return "sqrt((" + value + "^2)+" + number(w * w) + ")";
    };

    const auto phaseTerm = [&](Parameter & var, double w) -> std::string {
        const std::string value = var.isUncertain() ? var.name() : number(var.nominal());
        return "atan(" + number(w) + "/(" + value + "))";
    };

    const std::string gain = controller->gain().isUncertain()
            ? controller->gain().name()
            : number(controller->gain().nominal());

    magnitudeExpressions.clear();
    phaseExpressions.clear();

    for (double w : *omega) {

        std::string magnitude = "(" + gain + ")";
        std::string phase = "(0";

        for (Parameter & var : controller->numerator()) {
            magnitude += "*" + term(var, w);
            phase += "+" + phaseTerm(var, w);
        }

        for (Parameter & var : controller->denominator()) {
            magnitude += "/" + term(var, w);
            phase += "-" + phaseTerm(var, w);
        }

        phase += ")";

        magnitudeExpressions.push_back(magnitude);
        phaseExpressions.push_back(phase);
    }
}


//The constraint set of the ICSP (paper eqs. (10)-(11) plus the analogous
//QFTbx quadratics for the remaining specifications), one inequality
//"expression >= 0" per template representative (pairs for tracking) and
//design frequency where the specification band applies.
inline void AlgorithmMr::buildConstraints(){

    //The constraint set is rebuilt from scratch: the historical version
    //relied on the end-of-run cleanup to empty it, so a second run over
    //the same algorithm object would have doubled every constraint.
    constraints.clear();
    constraintTexts.clear();

    //The validated specification set, the same accessor the boundary
    //engine cuts at (the raw record heightDb evaluated NaN on some legacy
    //system specifications).
    const qftbx::SpecificationSet specifications = qftbx::toSpecificationSet(*specificationRecords);

    const auto applies = [&](std::int32_t slot, double w) {
        return specifications.at(static_cast<qftbx::SpecificationType>(slot)).appliesAt(w);
    };

    const auto boundDb = [&](std::int32_t slot, double w) {
        return specifications.at(static_cast<qftbx::SpecificationType>(slot)).boundDb(w);
    };

    const auto addConstraint = [&](const std::string & expression) {
        auto tree = std::make_unique<ExpressionTree>("1");
        tree->setFunc(expression, 0.0, alg::GREATER_EQUAL);
        constraints.push_back(std::move(tree));
        constraintTexts.push_back(expression);
    };

    for (std::size_t i = 0; i < omega->size(); ++i) {

        const double w = omega->at(i);
        const std::string & g = magnitudeExpressions.at(i);
        const std::string & phi = phaseExpressions.at(i);

        //Template representatives, evenly subsampled along the contour.
        //Non-finite or null points (artefacts of a degenerate contour)
        //would embed "nan" into the expression texts: they are skipped.
        std::vector<std::complex<double>> points;
        const qftbx::ComplexCloud & contour = temp.at(i);
        const std::size_t take = std::min<std::size_t>(kTemplateRepresentatives, contour.size());
        for (std::size_t j = 0; j < take; ++j) {
            const std::complex<double> value = contour.at(j * contour.size() / take);
            if (std::isfinite(value.real()) && std::isfinite(value.imag()) &&
                    std::abs(value) > 0.0) {
                points.push_back(value);
            }
        }

        for (const std::complex<double> & value : points) {

            const std::string p = number(std::abs(value));
            const std::string p2 = number(std::abs(value) * std::abs(value));
            const std::string theta = number(std::arg(value));

            //|1 + L|^2 expanded: g^2 p^2 + 2 g p cos(phi + theta) + 1.
            const std::string l2 = "((" + g + ")^2)*(" + p2 + ")+2*(" + g + ")*(" + p +
                    ")*cos((" + phi + ")+(" + theta + "))+1";

            //Stability margin |T| <= ws (paper eq. (10)); the sensor noise
            //specification shares the same transfer.
            for (std::int32_t slot : {2, 3}) {
                if (applies(slot, w)) {
                    const double ws = std::pow(10.0, boundDb(slot, w) / 20.0);
                    addConstraint("((" + g + ")^2)*(" + p2 + ")*(1-" +
                                  number(1.0 / (ws * ws)) + ")+2*(" + g + ")*(" + p +
                                  ")*cos((" + phi + ")+(" + theta + "))+1");
                }
            }

            //Output disturbance rejection |1/(1+L)| <= d:
            //|1+L|^2 - 1/d^2 >= 0.
            if (applies(4, w)) {
                const double d = std::pow(10.0, boundDb(4, w) / 20.0);
                addConstraint("(" + l2 + ")-" + number(1.0 / (d * d)));
            }

            //Input disturbance rejection |P/(1+L)| <= d:
            //|1+L|^2 - p^2/d^2 >= 0.
            if (applies(5, w)) {
                const double d = std::pow(10.0, boundDb(5, w) / 20.0);
                addConstraint("(" + l2 + ")-(" + p2 + ")*" + number(1.0 / (d * d)));
            }

            //Control effort |G/(1+L)| <= d: |1+L|^2 - g^2/d^2 >= 0 (the
            //historical rule dropped the g^2 factor).
            if (applies(6, w)) {
                const double d = std::pow(10.0, boundDb(6, w) / 20.0);
                addConstraint("(" + l2 + ")-((" + g + ")^2)*" + number(1.0 / (d * d)));
            }
        }

        //Tracking spread (paper eq. (11)) over ORDERED representative
        //pairs, with delta = |T_U/T_L| at this frequency.
        if (applies(0, w) && applies(1, w)) {

            const double deltaDb = boundDb(1, w) - boundDb(0, w);
            const double delta2 = std::pow(10.0, deltaDb / 10.0);
            const std::string invDelta2 = number(1.0 / delta2);

            for (std::size_t a = 0; a < points.size(); ++a) {
                for (std::size_t b = 0; b < points.size(); ++b) {
                    if (a == b) {
                        continue;
                    }

                    const double pi = std::abs(points.at(a));
                    const double thetaI = std::arg(points.at(a));
                    const double pk = std::abs(points.at(b));
                    const double thetaK = std::arg(points.at(b));

                    addConstraint("((" + g + ")^2)*" + number(pk * pk * pi * pi) +
                            "*(1-" + invDelta2 + ")+2*(" + g + ")*" + number(pk * pi) +
                            "*(" + number(pk) + "*cos((" + phi + ")+(" + number(thetaI) +
                            "))-" + number(pi) + "*" + invDelta2 + "*cos((" + phi +
                            ")+(" + number(thetaK) + ")))+" +
                            number(pk * pk) + "-" + number(pi * pi) + "*" + invDelta2);
                }
            }
        }
    }
}


bool AlgorithmMr::solve(){

    liveList = std::make_unique<OrderedList>(false, m_maxLiveNodes);
    stability = std::make_unique<NominalStabilityChecker>(plant, omega);

    buildControllerExpressions();
    buildConstraints();

    classifyAndInsert(std::move(controller));

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
            throw qftbx::InvalidInput(
                    "No feasible solution exists in the given search box.");
        }

        std::unique_ptr<SearchNode> node = liveList->takeFirstAs<SearchNode>();

        if (node->flag() == feasible || isParameterBoxSmall(node->system())) {

            const bool lowerCorner = node->flag() != ambiguous;

            //An epsilon-small box can still be AMBIGUOUS, and the point
            //taken from one is certified by nothing: the box was neither
            //proved feasible nor proved infeasible, and epsilon only says
            //it is small. The paper picks the minimum-gain controller out
            //of the FEASIBLE set, so a point that misses the constraint
            //set has to be dropped rather than reported. This is not
            //hypothetical: on the design example of the paper itself
            //(FDA-10 sec. 5, Example 5.1) the point of such a box missed
            //the robust stability margin by a factor of three, and nothing
            //said so. The check is the same constraint set the boxes are
            //judged by, evaluated on degenerate intervals, so it is
            //rigorous rather than a floating-point opinion. A feasible box
            //passes it by inclusion monotonicity.
            std::map<std::string, cxsc::interval> point;
            loadPointDomains(node->system(), lowerCorner, point);
            if (!certainlyFeasible(point)) {
                continue;
            }

            designedController = pointFromBox(node->system(), lowerCorner);

            //Every returned point must be nominally stabilising.
            if (!stability->isNominallyStable(designedController.get())) {
                designedController.reset();
                continue;
            }

            return true;
        }

        struct BisectionResult retur = bisectWidestParameter(node->system());

        classifyAndInsert(std::move(retur.v1));
        classifyAndInsert(std::move(retur.v2));
    }
}


std::size_t AlgorithmMr::peakLiveNodes() const
{
    return liveList != nullptr ? liveList->peakSize() : 0;
}


std::unique_ptr<LtiSystem> AlgorithmMr::controllerStructure(){
    return std::move(designedController);
}


//Branch & prune step for one box: narrow the parameter domains with the
//HC4 filter over the whole constraint set; an emptied domain proves the
//box infeasible, and non-negative interval evaluations of every
//constraint prove it feasible.
inline void AlgorithmMr::classifyAndInsert(std::unique_ptr<LtiSystem> box){

    std::map<std::string, cxsc::interval> domains;
    loadDomains(box.get(), domains);

    if (!narrowToFixpoint(domains)) {
        return;
    }

    std::unique_ptr<LtiSystem> narrowed = boxFromDomains(box.get(), domains);

    const BoxFlag flag = certainlyFeasible(domains) ? feasible : ambiguous;

    //The index is read BEFORE the box is handed over: as arguments of one
    //call their evaluation order is unspecified.
    const double gainInf = narrowed->gain().range().min;

    liveList->insert(std::make_unique<SearchNode>(gainInf, std::move(narrowed), flag));
}


inline bool AlgorithmMr::narrowToFixpoint(std::map<std::string, cxsc::interval> & domains){

    for (std::int32_t pass = 0; pass < kMaxNarrowingPasses; ++pass) {

        const std::map<std::string, cxsc::interval> snapshot = domains;

        for (const std::unique_ptr<ExpressionTree> & tree : constraints) {
            if (!tree->propagate(&domains)) {
                return false;
            }
        }

        bool changed = false;
        for (auto it = domains.begin(); it != domains.end(); ++it) {
            const cxsc::interval previous = snapshot.at(it->first);
            if (Inf(it->second) != Inf(previous) || Sup(it->second) != Sup(previous)) {
                changed = true;
                break;
            }
        }

        if (!changed) {
            break;
        }
    }

    return true;
}


inline bool AlgorithmMr::certainlyFeasible(std::map<std::string, cxsc::interval> & domains){

    for (const std::unique_ptr<ExpressionTree> & tree : constraints) {
        if (cxsc::_double(Inf(tree->eval(&domains))) < 0.0) {
            return false;
        }
    }

    return true;
}


inline void AlgorithmMr::loadDomains(LtiSystem * box,
                                           std::map<std::string, cxsc::interval> & domains){

    domains.clear();

    const auto load = [&](Parameter & var) {
        if (var.isUncertain()) {
            domains[var.name()] =
                    cxsc::interval(var.range().min, var.range().max);
        }
    };

    for (Parameter & var : box->numerator()) {
        load(var);
    }
    for (Parameter & var : box->denominator()) {
        load(var);
    }
    load(box->gain());
}


//The paper's termination criterion: a box is a solution box once every
//controller parameter has been narrowed below the requested accuracy
//(FDA-10 sec. 5, "the controller solutions are to be found to an accuracy
//eps"). The other four algorithms stop on the diameter of the NICHOLS box
//instead - the criterion of their own papers, which work on the projection
//- and that criterion does not transfer here: it scales with |P|, so on a
//plant reaching |P| = 1e4 at its lowest design frequency the same number
//is four decades tighter than it looks, and the ICSP never comes back.
//Widths are absolute, as the paper's are: it quotes its answers to four
//significant figures at eps = 0.001.
inline bool AlgorithmMr::isParameterBoxSmall(LtiSystem * box) const {

    const auto small = [&](Parameter & var) {
        return !var.isUncertain() || var.range().width() <= epsilon;
    };

    for (Parameter & var : box->numerator()) {
        if (!small(var)) {
            return false;
        }
    }

    for (Parameter & var : box->denominator()) {
        if (!small(var)) {
            return false;
        }
    }

    return small(box->gain());
}


//The corner rule of pointFromBox(), as degenerate domains: the same
//parameter names the constraint expressions are written in, so that the
//candidate point can be evaluated by the same trees.
inline void AlgorithmMr::loadPointDomains(LtiSystem * box, bool lowerCorner,
                                          std::map<std::string, cxsc::interval> & domains){

    domains.clear();

    const auto at = [&](Parameter & var, double value) {
        if (var.isUncertain()) {
            domains[var.name()] = cxsc::interval(value, value);
        }
    };

    for (Parameter & var : box->numerator()) {
        at(var, lowerCorner ? var.range().min : var.range().max);
    }

    //Poles always take the lower corner: see pointFromBox().
    for (Parameter & var : box->denominator()) {
        at(var, var.range().min);
    }

    at(box->gain(), lowerCorner ? box->gain().range().min : box->gain().range().max);
}


inline std::unique_ptr<LtiSystem> AlgorithmMr::boxFromDomains(LtiSystem * box,
                                                     const std::map<std::string, cxsc::interval> & domains){

    const auto rebuilt = [&](Parameter & var) -> Parameter {
        if (!var.isUncertain()) {
            return Parameter(var.nominal());
        }
        const cxsc::interval value = domains.at(var.name());
        return Parameter(var.name(),
                         Range(cxsc::_double(Inf(value)), cxsc::_double(Sup(value))),
                         cxsc::_double(Inf(value)));
    };

    std::vector<Parameter> nume;
    nume.reserve(box->numerator().size());
    for (Parameter & var : box->numerator()) {
        nume.push_back(rebuilt(var));
    }

    std::vector<Parameter> deno;
    deno.reserve(box->denominator().size());
    for (Parameter & var : box->denominator()) {
        deno.push_back(rebuilt(var));
    }

    return box->create(box->name(), std::move(nume), std::move(deno),
                       rebuilt(box->gain()), Parameter(double(0)));
}
