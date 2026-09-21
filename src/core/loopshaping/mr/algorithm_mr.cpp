/**
 * @file
 * @brief Algorithm MR: the interval constraint satisfaction search of the paper.
 *
 * The controller magnitude and phase at every design frequency are built as
 * expressions over the uncertain parameter names, a zero-pole-gain factor
 * contributing sqrt(x^2 + w^2) and a time-constant factor sqrt(1 + w^2/x^2),
 * both atan(w/x) to the phase. The constraint set is the paper's equations
 * (10) and (11) plus the analogous quadratics for the other specifications,
 * one inequality per template representative and design frequency, each in
 * its own tree because the propagation caches an enclosure per node. Nine
 * representatives per frequency, as the paper uses, sample the tracking
 * spread, and on the FDA-10 example the certified design exceeds the true
 * bound by up to a fifth at two frequencies: the sampled constraint set is
 * the paper's, and bounding the spread over the contour instead would change
 * what the algorithm computes.
 */

#include <string>
#include <vector>
#include <cstdint>
#include "src/core/common/exception.h"
#include "src/core/loopshaping/mr/algorithm_mr.h"

#include "src/core/specifications/specification_record.h"

#include <cmath>

namespace qftbx {

namespace {

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

void AlgorithmMr::buildControllerExpressions(){

    const bool timeConstant =
            controller->type() == LtiSystem::SystemType::TimeConstantGain;

    if (!timeConstant && controller->type() != LtiSystem::SystemType::ZeroPoleGain) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "The ICSP loop-shaping algorithm needs a zero-pole-gain or "
                "time-constant controller structure."));
    }

    const auto value = [](Parameter & var) {
        return var.isUncertain() ? Expression::variable(var.name()) : Expression(var.nominal());
    };

    const auto term = [&](Parameter & var, double w) {
        if (timeConstant) {
            return sqrt(Expression(1.0) + Expression(w * w) / pow(value(var), Expression(2.0)));
        }
        return sqrt(pow(value(var), Expression(2.0)) + Expression(w * w));
    };

    const auto phaseTerm = [&](Parameter & var, double w) {
        return atan(Expression(w) / value(var));
    };

    const Expression gain = value(controller->gain());

    magnitudeExpressions.clear();
    phaseExpressions.clear();

    for (double w : *omega) {

        Expression magnitude = gain;
        Expression phase(0.0);

        for (Parameter & var : controller->numerator()) {
            magnitude = magnitude * term(var, w);
            phase = phase + phaseTerm(var, w);
        }

        for (Parameter & var : controller->denominator()) {
            magnitude = magnitude / term(var, w);
            phase = phase - phaseTerm(var, w);
        }

        magnitudeExpressions.push_back(magnitude);
        phaseExpressions.push_back(phase);
    }
}

void AlgorithmMr::buildConstraints(){

    constraints.clear();

    const qftbx::SpecificationSet specifications = qftbx::toSpecificationSet(*specificationRecords);

    using qftbx::SpecificationType;

    const auto applies = [&](SpecificationType slot, double w) {
        return specifications.at(slot).appliesAt(w);
    };

    const auto boundDb = [&](SpecificationType slot, double w) {
        return specifications.at(slot).boundDb(w);
    };

    const auto addConstraint = [&](const Expression & expression) {
        constraints.push_back(std::make_unique<ExpressionTree>(expression, 0.0, qftbx::GREATER_EQUAL));
    };

    for (std::size_t i = 0; i < omega->size(); ++i) {

        const double w = omega->at(i);
        const Expression & g = magnitudeExpressions.at(i);
        const Expression & phi = phaseExpressions.at(i);
        const Expression g2 = pow(g, Expression(2.0));

        std::vector<std::complex<double>> points;
        const qftbx::ComplexCloud & contour = temp.at(i);
        const std::size_t take = std::min<std::size_t>(m_settings.algorithms.templateRepresentatives, contour.size());
        for (std::size_t j = 0; j < take; ++j) {
            const std::complex<double> value = contour.at(j * contour.size() / take);
            if (std::isfinite(value.real()) && std::isfinite(value.imag()) &&
                    std::abs(value) > 0.0) {
                points.push_back(value);
            }
        }

        for (const std::complex<double> & value : points) {

            const double p = std::abs(value);
            const double p2 = p * p;
            const double theta = std::arg(value);

            const Expression crossTerm = Expression(2.0) * g * Expression(p) * cos(phi + Expression(theta));
            const Expression l2 = g2 * Expression(p2) + crossTerm + Expression(1.0);

            for (SpecificationType slot : {SpecificationType::Stability, SpecificationType::SensorNoise}) {
                if (applies(slot, w)) {
                    const double ws = std::pow(10.0, boundDb(slot, w) / 20.0);
                    addConstraint(g2 * Expression(p2) * Expression(1.0 - 1.0 / (ws * ws)) + crossTerm + Expression(1.0));
                }
            }

            if (applies(SpecificationType::OutputDisturbance, w)) {
                const double d = std::pow(10.0, boundDb(SpecificationType::OutputDisturbance, w) / 20.0);
                addConstraint(l2 - Expression(1.0 / (d * d)));
            }

            if (applies(SpecificationType::InputDisturbance, w)) {
                const double d = std::pow(10.0, boundDb(SpecificationType::InputDisturbance, w) / 20.0);
                addConstraint(l2 - Expression(p2) * Expression(1.0 / (d * d)));
            }

            if (applies(SpecificationType::ControlEffort, w)) {
                const double d = std::pow(10.0, boundDb(SpecificationType::ControlEffort, w) / 20.0);
                addConstraint(l2 - g2 * Expression(1.0 / (d * d)));
            }
        }

        if (applies(SpecificationType::TrackingLower, w) && applies(SpecificationType::TrackingUpper, w)) {

            const double deltaDb = boundDb(SpecificationType::TrackingUpper, w) - boundDb(SpecificationType::TrackingLower, w);
            const double delta2 = std::pow(10.0, deltaDb / 10.0);
            const double invDelta2 = 1.0 / delta2;

            for (std::size_t a = 0; a < points.size(); ++a) {
                for (std::size_t b = 0; b < points.size(); ++b) {
                    if (a == b) {
                        continue;
                    }

                    const double pi = std::abs(points.at(a));
                    const double thetaI = std::arg(points.at(a));
                    const double pk = std::abs(points.at(b));
                    const double thetaK = std::arg(points.at(b));

                    addConstraint(g2 * Expression(pk * pk * pi * pi) * Expression(1.0 - invDelta2)
                                  + Expression(2.0) * g * Expression(pk * pi)
                                    * (Expression(pk) * cos(phi + Expression(thetaI))
                                       - Expression(pi) * Expression(invDelta2) * cos(phi + Expression(thetaK)))
                                  + Expression(pk * pk) - Expression(pi * pi) * Expression(invDelta2));
                }
            }
        }
    }
}

bool AlgorithmMr::solve(){

    liveList = std::make_unique<OrderedList>(false, m_settings.search.maxLiveNodes);
    stability = std::make_unique<NominalStabilityChecker>(plant, omega, m_settings.stability);

    buildControllerExpressions();
    buildConstraints();
    bindConstraints();

    const bool nicholsEpsilon = m_settings.algorithms.mrNicholsEpsilon;
    if (nicholsEpsilon) {
        if (controller->type() != LtiSystem::SystemType::ZeroPoleGain) {
            throw qftbx::InvalidInput(QFTBX_TR("Core", "The Nichols-box termination of algorithm MR "
                                      "(algorithms.mr-nichols-epsilon) needs a zero-pole-gain "
                                      "controller structure, as the other algorithms do."));
        }
        conversion = std::make_unique<NaturalIntervalExtension>();
        nominalPlantValues.clear();
        for (double o : *omega) {
            nominalPlantValues.push_back(plant->evaluate(o));
        }
    }

    classifyAndInsert(std::move(controller));

    while (true) {

        if (qftbx::cancellationAsked(m_cancellation)) {
            throw qftbx::Cancelled();
        }

        if (liveList->isEmpty()) {
            throw qftbx::InvalidInput(QFTBX_TR("Core", "No feasible solution exists in the given search box."));
        }

        std::unique_ptr<SearchNode> node = liveList->takeFirstAs<SearchNode>();

        const bool small = nicholsEpsilon
                ? isEpsilonSmall(node->system(), epsilon, omega, conversion.get(), nominalPlantValues)
                : isParameterBoxSmall(node->system());

        if (node->flag() == feasible || small) {

            const bool lowerCorner = node->flag() != ambiguous;

            std::vector<Interval> point;
            loadPointDomains(node->system(), lowerCorner, point);
            if (!certainlyFeasible(point)) {
                continue;
            }

            designedController = pointFromBox(node->system(), lowerCorner);

            if (!stability->isNominallyStable(designedController.get())) {
                designedController.reset();
                continue;
            }

            return true;
        }

        BisectionResult halves = bisectWidestParameter(node->system());

        classifyAndInsert(std::move(halves.v1));
        classifyAndInsert(std::move(halves.v2));
    }
}

std::size_t AlgorithmMr::peakLiveNodes() const
{
    return liveList != nullptr ? liveList->peakSize() : 0;
}

LoopShapingStatistics AlgorithmMr::statistics() const
{
    LoopShapingStatistics statistics;
    if (liveList != nullptr) {
        statistics.peakLiveNodes = liveList->peakSize();
        statistics.nodesProcessed = liveList->takenCount();
    }
    if (stability != nullptr) {
        statistics.stabilityVerdicts = stability->statistics().verdicts;
        statistics.stabilityProfiles = stability->statistics().profilesComputed;
    }
    return statistics;
}

std::unique_ptr<LtiSystem> AlgorithmMr::controllerStructure(){
    return std::move(designedController);
}

void AlgorithmMr::classifyAndInsert(std::unique_ptr<LtiSystem> box){

    std::vector<Interval> domains;
    loadDomains(box.get(), domains);

    if (!narrowToFixpoint(domains)) {
        return;
    }

    std::unique_ptr<LtiSystem> narrowed = boxFromDomains(box.get(), domains);

    const BoxFlag flag = certainlyFeasible(domains) ? feasible : ambiguous;

    const double gainInf = narrowed->gain().range().min;

    liveList->insert(std::make_unique<SearchNode>(gainInf, std::move(narrowed), flag));
}

bool AlgorithmMr::narrowToFixpoint(std::vector<Interval> & domains){

    std::vector<Interval> snapshot;

    for (std::int32_t pass = 0; pass < m_settings.algorithms.maxNarrowingPasses; ++pass) {

        snapshot = domains;

        for (const std::unique_ptr<ExpressionTree> & tree : constraints) {
            if (!tree->propagate(domains)) {
                return false;
            }
        }

        bool changed = false;
        for (std::size_t i = 0; i < domains.size(); ++i) {
            if (domains[i].lower() != snapshot[i].lower() || domains[i].upper() != snapshot[i].upper()) {
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

bool AlgorithmMr::certainlyFeasible(std::vector<Interval> & domains){

    for (const std::unique_ptr<ExpressionTree> & tree : constraints) {
        if (tree->eval(domains).lower() < 0.0) {
            return false;
        }
    }

    return true;
}

namespace {

template <class Visit>
void forEachUncertain(LtiSystem * box, Visit visit)
{
    for (Parameter & var : box->numerator()) {
        if (var.isUncertain()) {
            visit(var);
        }
    }
    for (Parameter & var : box->denominator()) {
        if (var.isUncertain()) {
            visit(var);
        }
    }
    if (box->gain().isUncertain()) {
        visit(box->gain());
    }
}

}

void AlgorithmMr::bindConstraints(){

    parameterNames.clear();
    forEachUncertain(controller.get(), [&](Parameter & var) {
        parameterNames.push_back(var.name());
    });

    for (const std::unique_ptr<ExpressionTree> & tree : constraints) {
        tree->bind(parameterNames);
    }
}

void AlgorithmMr::loadDomains(LtiSystem * box, std::vector<Interval> & domains){

    domains.clear();
    domains.reserve(parameterNames.size());
    forEachUncertain(box, [&](Parameter & var) {
        domains.push_back(Interval(var.range().min, var.range().max));
    });
}

bool AlgorithmMr::isParameterBoxSmall(LtiSystem * box) const {

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

void AlgorithmMr::loadPointDomains(LtiSystem * box, bool lowerCorner,
                                          std::vector<Interval> & domains){

    domains.clear();
    domains.reserve(parameterNames.size());

    for (Parameter & var : box->numerator()) {
        if (var.isUncertain()) {
            domains.push_back(Interval(lowerCorner ? var.range().min : var.range().max));
        }
    }
    for (Parameter & var : box->denominator()) {
        if (var.isUncertain()) {
            domains.push_back(Interval(var.range().min));
        }
    }
    if (box->gain().isUncertain()) {
        domains.push_back(Interval(lowerCorner ? box->gain().range().min : box->gain().range().max));
    }
}

std::unique_ptr<LtiSystem> AlgorithmMr::boxFromDomains(LtiSystem * box,
                                                     const std::vector<Interval> & domains){

    std::size_t next = 0;
    const auto rebuilt = [&](Parameter & var) -> Parameter {
        if (!var.isUncertain()) {
            return Parameter(var.nominal());
        }
        const Interval value = domains.at(next++);
        return Parameter(var.name(),
                         Range(value.lower(), value.upper()),
                         value.lower());
    };

    std::vector<Parameter> numerator;
    numerator.reserve(box->numerator().size());
    for (Parameter & var : box->numerator()) {
        numerator.push_back(rebuilt(var));
    }

    std::vector<Parameter> denominator;
    denominator.reserve(box->denominator().size());
    for (Parameter & var : box->denominator()) {
        denominator.push_back(rebuilt(var));
    }

    return box->create(box->name(), std::move(numerator), std::move(denominator),
                       rebuilt(box->gain()), box->delay());
}

}
