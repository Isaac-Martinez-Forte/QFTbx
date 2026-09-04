#ifndef QFTBX_LOOPSHAPING_COMMON_FUNCTIONS_H
#define QFTBX_LOOPSHAPING_COMMON_FUNCTIONS_H


#include <cstdint>
#include "src/core/math/constants.h"
#include <memory>

#include <vector>

#include "src/core/system/lti_system.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
#include "src/core/loopshaping/ordered_list.h"
#include "src/core/loopshaping/mc_search_node.h"
#include "src/core/loopshaping/quick_solution.h"

#include "cinterval.hpp"
#include <complex>

#include "src/core/exception.h"


namespace qftbx {

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

    std::vector <Parameter> numerator;
    numerator.reserve(controller->numerator().size());

    for (Parameter & v : controller->numerator()) {
        if (!v.isUncertain()){
            numerator.emplace_back(v.nominal());
        } else {
            numerator.emplace_back(x ? v.range().min : v.range().max);
        }
    }

    std::vector <Parameter> denominator;
    denominator.reserve(controller->denominator().size());

    for (Parameter & v : controller->denominator()) {
        //Poles always take the lower corner: a larger pole moves the
        //projection towards the forbidden side.
        denominator.emplace_back(v.isUncertain() ? v.range().min : v.nominal());
    }

    const double k = x ? controller->gain().range().min
                      : controller->gain().range().max;

    //The delay is not searched over: the point keeps the box's own, as the
    //bisection does. It used to be written as zero, which changed the
    //controller for any structure carrying a delay.
    return controller->create(controller->name(), std::move(numerator),
                               std::move(denominator), Parameter(k),
                               controller->delay());
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
                            const std::vector <cxsc::complex> & nominalPlantValues) {

    cxsc::cinterval box;
    for (std::size_t i = 0; i < omega->size(); ++i){
        box = conversion->nicholsBox(controller, omega->at(i), nominalPlantValues.at(i));

        if ((cxsc::diam(cxsc::Re(box)) >= epsilon) || (cxsc::diam(cxsc::Im(box)) >= epsilon)) {
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

    //A box with nothing uncertain cannot be halved: the two "halves" would
    //be the box itself, and a search that bisects it never gets smaller.
    if (widest == -2) {
        throw qftbx::ComputationError("The search asked to bisect a controller box with no uncertain parameter.");
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

    BisectionResult halves;
    halves.v1 = half(true);
    halves.v2 = half(false);

    return halves;
}

/**
 * @brief The parameter bounds of a controller box as plain vectors: what
 * the Quick Solution equations read and write.
 *
 * Fixed parameters carry their nominal at both ends and are never cut. NK,
 * MC1 and the thesis MC each used to extract these vectors, run the same
 * cutting loops over them and rebuild the box from them, in three copies
 * that had to agree on every detail (which end a pole cuts, which corner
 * the other parameters sit at).
 */
struct ParameterBounds {
    std::vector<double> zeroInfs, zeroSups, poleInfs, poleSups;
    std::vector<char> zeroUncertain, poleUncertain;
    double gainInf = 0.0;
    double gainSup = 0.0;
    bool gainUncertain = false;
};

inline ParameterBounds boundsOf(LtiSystem * box) {
    ParameterBounds b;

    for (Parameter & var : box->numerator()) {
        b.zeroInfs.push_back(var.isUncertain() ? var.range().min : var.nominal());
        b.zeroSups.push_back(var.isUncertain() ? var.range().max : var.nominal());
        b.zeroUncertain.push_back(var.isUncertain() ? 1 : 0);
    }
    for (Parameter & var : box->denominator()) {
        b.poleInfs.push_back(var.isUncertain() ? var.range().min : var.nominal());
        b.poleSups.push_back(var.isUncertain() ? var.range().max : var.nominal());
        b.poleUncertain.push_back(var.isUncertain() ? 1 : 0);
    }

    b.gainInf = box->gain().range().min;
    b.gainSup = box->gain().range().max;
    b.gainUncertain = box->gain().isUncertain();

    return b;
}

/// The box with the bounds written back: every uncertain parameter takes
/// its new range with the nominal at the infimum, the gain keeps the "kv"
/// name of the search, and the fixed ones stay as they were.
inline std::unique_ptr<LtiSystem> boxFromBounds(LtiSystem * box, const ParameterBounds & b) {
    std::vector<Parameter> numerator;
    numerator.reserve(b.zeroInfs.size());
    for (std::size_t j = 0; j < b.zeroInfs.size(); ++j) {
        Parameter & old = box->numerator()[j];
        numerator.push_back(old.isUncertain()
                ? Parameter(old.name(), Range(b.zeroInfs[j], b.zeroSups[j]), b.zeroInfs[j])
                : Parameter(old.nominal()));
    }

    std::vector<Parameter> denominator;
    denominator.reserve(b.poleInfs.size());
    for (std::size_t j = 0; j < b.poleInfs.size(); ++j) {
        Parameter & old = box->denominator()[j];
        denominator.push_back(old.isUncertain()
                ? Parameter(old.name(), Range(b.poleInfs[j], b.poleSups[j]), b.poleInfs[j])
                : Parameter(old.nominal()));
    }

    return box->create(box->name(), std::move(numerator), std::move(denominator),
            b.gainUncertain
                ? Parameter("kv", Range(b.gainInf, b.gainSup), b.gainInf, "kv")
                : Parameter(box->gain().nominal()),
            box->delay());
}

/**
 * @brief Quick Solution from below (NK, sec. 3.3, steps 3-8): with the strip
 * under the boundary minimum certainly forbidden, the gain and every zero
 * raise their infimum and every pole lowers its supremum to the point where
 * the loop stops being certainly below B_min. Sequential, on the latest
 * updated values. Returns whether anything moved.
 */
inline bool cutBelowBoundary(ParameterBounds & b, double boundMin, double w, std::complex<double> p0) {
    bool cut = false;

    if (b.gainUncertain) {
        const double k = qftbx::quick_solution::gainCut(boundMin, b.zeroSups, b.poleInfs, w, p0);
        if (k > b.gainInf && k < b.gainSup) {
            b.gainInf = k;
            cut = true;
        }
    }

    for (std::size_t j = 0; j < b.zeroInfs.size(); ++j) {
        if (!b.zeroUncertain[j]) continue;
        const double z = qftbx::quick_solution::zeroCut(boundMin, b.gainSup, b.zeroSups, b.poleInfs, j, w, p0);
        if (z > b.zeroInfs[j] && z < b.zeroSups[j]) {
            b.zeroInfs[j] = z;
            cut = true;
        }
    }

    for (std::size_t j = 0; j < b.poleInfs.size(); ++j) {
        if (!b.poleUncertain[j]) continue;
        const double p = qftbx::quick_solution::poleCut(boundMin, b.gainSup, b.zeroSups, b.poleInfs, j, w, p0);
        if (p > b.poleInfs[j] && p < b.poleSups[j]) {
            b.poleSups[j] = p;
            cut = true;
        }
    }

    return cut;
}

/// The mirror of cutBelowBoundary on the strip over the boundary maximum
/// (thesis MC): the loop-minimising corner and B_max, cutting the other
/// end of every range.
inline bool cutAboveBoundary(ParameterBounds & b, double boundMax, double w, std::complex<double> p0) {
    bool cut = false;

    if (b.gainUncertain) {
        const double k = qftbx::quick_solution::gainCut(boundMax, b.zeroInfs, b.poleSups, w, p0);
        if (k > b.gainInf && k < b.gainSup) {
            b.gainSup = k;
            cut = true;
        }
    }

    for (std::size_t j = 0; j < b.zeroInfs.size(); ++j) {
        if (!b.zeroUncertain[j]) continue;
        const double z = qftbx::quick_solution::zeroCut(boundMax, b.gainInf, b.zeroInfs, b.poleSups, j, w, p0);
        if (z > b.zeroInfs[j] && z < b.zeroSups[j]) {
            b.zeroSups[j] = z;
            cut = true;
        }
    }

    for (std::size_t j = 0; j < b.poleInfs.size(); ++j) {
        if (!b.poleUncertain[j]) continue;
        const double p = qftbx::quick_solution::poleCut(boundMax, b.gainInf, b.zeroInfs, b.poleSups, j, w, p0);
        if (p > b.poleInfs[j] && p < b.poleSups[j]) {
            b.poleInfs[j] = p;
            cut = true;
        }
    }

    return cut;
}

/// Phase cuts on the right strip (phases above thetaMax certainly
/// forbidden; MC stage 2, thesis 4.1.2): zeros raise their infimum, poles
/// lower their supremum. Angles in radians, phi0 the nominal plant phase.
inline bool cutRightOfPhase(ParameterBounds & b, double thetaMax, double phi0, double w) {
    bool cut = false;

    for (std::size_t j = 0; j < b.zeroInfs.size(); ++j) {
        if (!b.zeroUncertain[j]) continue;
        const double z = qftbx::quick_solution::zeroPhaseCutHigh(thetaMax, phi0, b.zeroSups, b.poleInfs, j, w);
        if (z > b.zeroInfs[j] && z < b.zeroSups[j]) {
            b.zeroInfs[j] = z;
            cut = true;
        }
    }

    for (std::size_t j = 0; j < b.poleInfs.size(); ++j) {
        if (!b.poleUncertain[j]) continue;
        const double p = qftbx::quick_solution::polePhaseCutHigh(thetaMax, phi0, b.zeroSups, b.poleInfs, j, w);
        if (p > b.poleInfs[j] && p < b.poleSups[j]) {
            b.poleSups[j] = p;
            cut = true;
        }
    }

    return cut;
}

/// Phase cuts on the left strip (phases below thetaMin certainly
/// forbidden): zeros lower their supremum, poles raise their infimum.
inline bool cutLeftOfPhase(ParameterBounds & b, double thetaMin, double phi0, double w) {
    bool cut = false;

    for (std::size_t j = 0; j < b.zeroInfs.size(); ++j) {
        if (!b.zeroUncertain[j]) continue;
        const double z = qftbx::quick_solution::zeroPhaseCutLow(thetaMin, phi0, b.zeroInfs, b.poleSups, j, w);
        if (z > b.zeroInfs[j] && z < b.zeroSups[j]) {
            b.zeroSups[j] = z;
            cut = true;
        }
    }

    for (std::size_t j = 0; j < b.poleInfs.size(); ++j) {
        if (!b.poleUncertain[j]) continue;
        const double p = qftbx::quick_solution::polePhaseCutLow(thetaMin, phi0, b.zeroInfs, b.poleSups, j, w);
        if (p > b.poleInfs[j] && p < b.poleSups[j]) {
            b.poleInfs[j] = p;
            cut = true;
        }
    }

    return cut;
}

/// The prune step of MC1 (paper step 3bis.(b)) and the thesis MC (5.4.3):
/// the gain range of a box capped at the prune variable C. Takes the box
/// over and returns the capped replacement, or the box itself when the cap
/// does not apply.
inline std::unique_ptr<LtiSystem> capGain(std::unique_ptr<LtiSystem> box, double cap) {
    if (!box->gain().isUncertain() ||
            cap <= box->gain().range().min || cap >= box->gain().range().max) {
        return box;
    }

    return box->create(box->name(), box->numerator(), box->denominator(),
            Parameter("kv", Range(box->gain().range().min, cap),
                          box->gain().range().min, "kv"),
            box->delay());
}

/// Nominal plant phase on the (-2 pi, 0] branch the Nichols boxes use.
inline double nominalPhase(std::complex<double> p0) {
    double phi0 = std::arg(p0);

    if (phi0 > 0.0) {
        phi0 -= 2.0 * qftbx::math::kPi;
    }

    return phi0;
}

} // namespace qftbx

#endif
