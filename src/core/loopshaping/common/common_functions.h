/**
 * @file
 * @brief Operations on controller boxes shared by the interval algorithms.
 *
 * pointFromBox builds a corner of a box as a controller, and
 * satisfiesBoundaries asks the boundaries about one controller at every design
 * frequency, projected by the same interval extension the search uses. An
 * epsilon-small ambiguous box ends the search with a point of it (Tharewal
 * 2005, Remark 3.1; QFTbx thesis sec. 3.1), and the anti-blocking rule picks
 * the corner that moves the projection towards the allowed side when that side
 * is up: maximum gain and zeros, and minimum poles, which push the projection
 * down. That is a guess about the boundary, not a certificate: where a closed
 * boundary crosses the box its allowed side is down, and on the QFT toolbox
 * example 2 every algorithm returned such a corner at some frequency. So
 * forEachCandidate walks the candidates in order, the anti-blocking corner,
 * the lower corner, the centre and then, while there are at most six
 * uncertain parameters, every other corner; a caller that recomputes the
 * gain at the zeros and poles of each candidate, as MC2 does under the exact
 * reading, walks only the corners of the zeros and poles. verifiedCornerBy
 * returns the first candidate a test passes, and verifiedCorner the first
 * the boundaries accept.
 *
 * isEpsilonSmall is the termination test of NT, NK, MC1 and MC: the Nichols
 * rectangle of the box narrower than epsilon, in both coordinates, at every
 * design frequency. Their papers place the accuracy on the loop transmission
 * they return, so it is measured there and scales with the plant's modulus;
 * MR measures it on the parameters (AlgorithmMr::isParameterBoxSmall).
 * bisectWidestParameter splits the widest uncertain parameter at its middle,
 * which is how NT, NK, MR and MC1 branch; MC of the thesis and MC2 measure
 * the widest on the projection of the box, and MC3 splits a zero or a pole on
 * a logarithmic scale, each in its own bisect(). BisectionResult holds the two
 * halves for whoever receives them.
 *
 * ParameterBounds holds the bounds of a box as the Quick Solution equations
 * (quick_solution.h) read and write them, the fixed parameters at their
 * nominal at both ends, and boxFromBounds writes them back. The four cuts
 * apply those equations: cutBelowBoundary (NK sec. 3.3, steps 3-8), with the
 * strip under B_min certainly forbidden, raises the infimum of the gain and
 * of every zero and lowers the supremum of every pole, sequentially on the
 * latest values; cutAboveBoundary is its mirror over B_max; cutRightOfPhase
 * and cutLeftOfPhase (thesis 4.1.2) cut on the phase strips, the zeros and
 * the poles moving opposite ends. capGain caps the gain range of a box at the
 * prune variable C (MC1 step 3bis.(b), thesis 5.4.3), and nominalPhase puts
 * the nominal plant phase on the (-2 pi, 0] branch of the Nichols boxes.
 */

#ifndef QFTBX_LOOPSHAPING_COMMON_FUNCTIONS_H
#define QFTBX_LOOPSHAPING_COMMON_FUNCTIONS_H

#include <complex>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/common/exception.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/loopshaping/common/point_controller.h"
#include "src/core/loopshaping/common/quick_solution.h"
#include "src/core/math/constants.h"
#include "src/core/system/lti_system.h"

namespace qftbx {

struct BisectionResult {
    std::unique_ptr<LtiSystem> v1;
    std::unique_ptr<LtiSystem> v2;
};

inline std::unique_ptr<LtiSystem> pointFromBox(LtiSystem * controller, bool lower)
{
    return systemFromPoint(controller, cornerOf(controller, lower));
}

inline bool satisfiesBoundaries(const PointController & point, std::vector<double> * omega,
                                NaturalIntervalExtension * conversion, BoundaryViolationDetector * detector,
                                const BoundaryData * boundaries,
                                const std::vector<std::complex<double>> & nominalPlantValues) {

    for (std::size_t i = 0; i < omega->size(); ++i) {
        const NicholsBox box = conversion->nicholsPoint(point, omega->at(i), nominalPlantValues.at(i));

        if (detector->classifyBox(box, boundaries, i).flag() != feasible) {
            return false;
        }
    }

    return true;
}

enum class CandidateGain { FromTheCorner, Recomputed };

template <class Visit>
inline bool forEachCandidate(LtiSystem * box, Visit && visit,
                             CandidateGain candidateGain = CandidateGain::FromTheCorner) {

    for (const bool lower : {false, true}) {
        if (visit(cornerOf(box, lower))) {
            return true;
        }
    }

    const std::vector<Parameter> & numerator = box->numerator();
    const std::vector<Parameter> & denominator = box->denominator();
    const Parameter & gain = box->gain();

    std::size_t uncertain = 0;
    for (const Parameter & v : numerator) uncertain += v.isUncertain() ? 1 : 0;
    for (const Parameter & v : denominator) uncertain += v.isUncertain() ? 1 : 0;
    uncertain += gain.isUncertain() ? 1 : 0;

    const auto pointAt = [&](auto choose) {
        PointController point;
        std::size_t index = 0;
        const auto pick = [&](const Parameter & v) {
            if (!v.isUncertain()) {
                return v.nominal();
            }
            const int c = choose(index++);
            const Range r = v.range();
            return c == 0 ? r.min : c == 1 ? r.max : 0.5 * (r.min + r.max);
        };
        point.zeros.reserve(numerator.size());
        for (const Parameter & v : numerator) point.zeros.push_back(pick(v));
        point.poles.reserve(denominator.size());
        for (const Parameter & v : denominator) point.poles.push_back(pick(v));
        point.gain = pick(gain);
        return point;
    };

    if (visit(pointAt([](std::size_t) { return 2; }))) {
        return true;
    }

    if (uncertain <= 6) {
        unsigned zeroBits = 0;
        std::size_t index = 0;
        for (const Parameter & v : numerator) {
            if (v.isUncertain()) {
                zeroBits |= 1u << index++;
            }
        }
        const bool walkTheGain = gain.isUncertain() && candidateGain == CandidateGain::FromTheCorner;
        const std::size_t bits = gain.isUncertain() && !walkTheGain ? uncertain - 1 : uncertain;
        const unsigned antiBlocking = zeroBits | (walkTheGain ? 1u << (uncertain - 1) : 0u);
        const unsigned corners = 1u << bits;
        for (unsigned mask = 1; mask < corners; ++mask) {
            if (mask == antiBlocking) {
                continue;
            }
            if (visit(pointAt([mask](std::size_t i) { return static_cast<int>((mask >> i) & 1u); }))) {
                return true;
            }
        }
    }

    return false;
}

template <class Passes>
inline std::optional<PointController> verifiedCornerBy(LtiSystem * box, Passes && passes) {
    std::optional<PointController> found;
    forEachCandidate(box, [&](const PointController & candidate) {
        if (passes(candidate)) {
            found = candidate;
            return true;
        }
        return false;
    });
    return found;
}

inline std::optional<PointController> verifiedCorner(LtiSystem * box, std::vector<double> * omega,
                                                     NaturalIntervalExtension * conversion,
                                                     BoundaryViolationDetector * detector,
                                                     const BoundaryData * boundaries,
                                                     const std::vector<std::complex<double>> & nominalPlantValues) {

    return verifiedCornerBy(box, [&](const PointController & point) {
        return satisfiesBoundaries(point, omega, conversion, detector, boundaries, nominalPlantValues);
    });
}

inline bool isEpsilonSmall(LtiSystem * controller, double epsilon, std::vector <double> * omega,
                            NaturalIntervalExtension *conversion,
                            const std::vector<std::complex<double>> & nominalPlantValues) {

    NicholsBox box;
    for (std::size_t i = 0; i < omega->size(); ++i){
        box = conversion->nicholsBox(controller, omega->at(i), nominalPlantValues.at(i));

        if ((box.magnitudeDb.width() >= epsilon) || (box.phaseDegrees.width() >= epsilon)) {
            return false;
        }
    }

    return true;
}

inline BisectionResult bisectWidestParameter(LtiSystem * box) {

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

    if (widest == -2) {
        throw qftbx::ComputationError(QFTBX_TR("Core", "The search asked to bisect a controller box with no uncertain parameter."));
    }

    const double middle = range.middle();

    const auto half = [&](bool lower) -> std::unique_ptr<LtiSystem> {
        const Range halfRange = lower ? Range(range.min, middle)
                                      : Range(middle, range.max);

        Parameter gain = widest == -1
                ? Parameter(box->gain().name(), halfRange, halfRange.min)
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
                ? Parameter(box->gain().name(), Range(b.gainInf, b.gainSup), b.gainInf)
                : Parameter(box->gain().nominal()),
            box->delay());
}

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

inline std::unique_ptr<LtiSystem> capGain(std::unique_ptr<LtiSystem> box, double cap) {
    if (!box->gain().isUncertain() ||
            cap <= box->gain().range().min || cap >= box->gain().range().max) {
        return box;
    }

    return box->create(box->name(), box->numerator(), box->denominator(),
            Parameter(box->gain().name(), Range(box->gain().range().min, cap),
                      box->gain().range().min),
            box->delay());
}

inline double nominalPhase(std::complex<double> p0) {
    double phi0 = std::arg(p0);

    if (phi0 > 0.0) {
        phi0 -= 2.0 * qftbx::math::kPi;
    }

    return phi0;
}

}

#endif
