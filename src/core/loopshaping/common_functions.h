#ifndef QFTBX_LOOPSHAPING_COMMON_FUNCTIONS_H
#define QFTBX_LOOPSHAPING_COMMON_FUNCTIONS_H


#include <cstdint>
#include <memory>

#include <vector>

#include "src/core/system/lti_system.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
#include "src/core/loopshaping/ordered_list.h"
#include "src/core/loopshaping/mc_search_node.h"

#include "cinterval.hpp"
#include <complex>

using namespace tools;
using namespace cxsc;

namespace FC {

/// The halves of a bisection belong to whoever receives them: each one is
/// either classified into the live list or dropped.
struct BisectionResult {
    std::unique_ptr<LtiSystem> v1;
    std::unique_ptr<LtiSystem> v2;
};

/// The children of a bisection belong to whoever receives them: they are
/// either inserted in the live list or dropped, and the type says so.
struct McBisectionResult {
    std::unique_ptr<McSearchNode> t1;
    std::unique_ptr<McSearchNode> t2;
};

/// Which plane a projection is asked for.
enum diagrama {Nichol = false, Nyquist = true};

/**
 * @brief Extracts a point controller from a box.
 *
 * @param controller the box to take a corner of.
 * @param x true takes the lower corner of every parameter, which is where
 * a feasible box realises its optimum gain. false takes the corner that
 * the monotonicity of the Nichols projection makes feasible for an
 * epsilon-small ambiguous box sitting on a boundary whose allowed side is
 * up (the anti-blocking rule, QFTbx thesis sec. 3.1): maximum gain and
 * zeros push the box up, but poles push it DOWN, so poles take their
 * minimum. The historical code took every maximum, stepping AWAY from the
 * allowed side in the pole directions.
 */
inline std::unique_ptr<LtiSystem> pointFromBox(LtiSystem * controller, bool x) {

    std::vector <Parameter> numerador;
    numerador.reserve(controller->numerator().size());

    for (Parameter & v : controller->numerator()) {
        if (!v.isUncertain()){
            numerador.emplace_back(v.nominal());
        } else {
            numerador.emplace_back(x ? v.range().min : v.range().max);
        }
    }

    std::vector <Parameter> denominador;
    denominador.reserve(controller->denominator().size());

    for (Parameter & v : controller->denominator()) {
        //Poles always take the lower corner: a larger pole moves the
        //projection towards the forbidden side.
        denominador.emplace_back(v.isUncertain() ? v.range().min : v.nominal());
    }

    const double k = x ? controller->gain().range().min
                      : controller->gain().range().max;

    return controller->create(controller->name(), std::move(numerador),
                               std::move(denominador), Parameter(k),
                               Parameter(double(0)));
}

/**
 * @brief Termination test of NT, NK, MC1 and MC: the box's Nichols
 * rectangle is narrower than epsilon, in both coordinates, at EVERY
 * design frequency.
 *
 * The four measure epsilon on the projection because that is where their
 * papers place the accuracy: the answer they give is a loop transmission,
 * and how well it is pinned down is a question about \f$ L_0 \f$, not
 * about the parameters that produced it. MR is the exception and does not
 * come through here - its paper solves a constraint satisfaction problem
 * over the parameters and measures the accuracy there
 * (AlgorithmMr::isParameterBoxSmall).
 *
 * One consequence worth keeping in mind: this criterion scales with
 * \f$ |P| \f$, so the same number is far tighter on a plant with a large
 * low-frequency gain than on one without.
 */
inline bool isEpsilonSmall(LtiSystem * controller, double epsilon, std::vector <double> * omega,
                            NaturalIntervalExtension *conversion,
                            const std::vector <complex> & nominalPlantValues) {

    cinterval box;
    for (std::size_t i = 0; i < omega->size(); ++i){
        box = conversion->nicholsBox(controller, omega->at(i), nominalPlantValues.at(i));

        if ((cxsc::diam(Re(box)) >= epsilon) || (cxsc::diam(Im(box)) >= epsilon)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Bisects the box at the middle of its widest uncertain parameter.
 *
 * The direction that halves the largest remaining uncertainty is the one
 * that makes the interval enclosure tightest fastest, which is why every
 * one of the five algorithms branches this way by default (MC's tree
 * bisection is the documented exception).
 */
inline BisectionResult bisectWidestParameter(LtiSystem * box) {

    //Widest uncertain parameter: -1 is the gain, then the numerator and
    //denominator positions.
    std::int32_t widest = -2;
    double width = -1;
    Range range;

    if (box->gain().isUncertain()) {
        range = box->gain().range();
        widest = -1;
        width = range.max - range.min;
    }

    std::int32_t position = 0;

    const auto consider = [&](Parameter & var) {
        if (var.isUncertain() && var.range().max - var.range().min > width) {
            widest = position;
            width = var.range().max - var.range().min;
            range = var.range();
        }
        position++;
    };

    for (Parameter & var : box->numerator()) {
        consider(var);
    }
    for (Parameter & var : box->denominator()) {
        consider(var);
    }

    const double middle = range.middle();

    //Both children are DEEP copies and the parent stays untouched: its
    //node keeps sole ownership of it (the historical version handed the
    //parent's vectors to the second child, forcing every caller to leak
    //the parent shell to stay safe). The halves keep the parameter's
    //NAME: the ICSP constraint trees address the variables by name.
    const auto half = [&](bool lower) -> std::unique_ptr<LtiSystem> {
        const Range halfRange = lower ? Range(range.min, middle)
                                      : Range(middle, range.max);

        Parameter gain = widest == -1
                ? Parameter(box->gain().name(), halfRange, halfRange.min, box->gain().name())
                : box->gain();

        std::int32_t index = 0;

        std::vector <Parameter> numerator;
        numerator.reserve(box->numerator().size());
        for (Parameter & var : box->numerator()) {
            numerator.push_back(index++ == widest
                    ? Parameter(var.name(), halfRange, halfRange.min)
                    : var);
        }

        std::vector <Parameter> denominator;
        denominator.reserve(box->denominator().size());
        for (Parameter & var : box->denominator()) {
            denominator.push_back(index++ == widest
                    ? Parameter(var.name(), halfRange, halfRange.min)
                    : var);
        }

        return box->create(box->name(), std::move(numerator), std::move(denominator),
                           std::move(gain), box->delay());
    };

    BisectionResult retur;
    retur.v1 = half(true);
    retur.v2 = half(false);

    return retur;
}

} // fin namespace

#endif
