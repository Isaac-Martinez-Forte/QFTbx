/**
 * @file
 * @brief The tag names and the version of the .qft project format.
 *
 * One table names every element and attribute, so neither side can rename
 * a tag without the other following. A file has three parts in the order a
 * reader meets them: the inputs the user described, the settings each
 * computation was run with, and the results. The version this build writes
 * (kVersion) is the only one it reads; any other is refused rather than
 * read with its sections missing. Algorithms are stored by name rather than
 * by position, so a file still says which algorithm produced a design after
 * another one is added to the list; algorithmFromName gives nothing for a
 * name this build does not have. The file also names why the verifier left
 * the family's stability unchecked. Internal to src/persistence, shared by
 * the reader and the writer.
 */

#ifndef QFTBX_QFT_DIALECT_H
#define QFTBX_QFT_DIALECT_H

#include "src/core/loopshaping/loop_shaping_types.h"

#include <optional>
#include <string>

#include "src/core/loopshaping/common/specification_checker.h"

namespace qftbx {

struct Tags {
    const char * inputs;
    const char * settings;
    const char * results;

    const char * plant;
    const char * controller;
    const char * loopShaping;
    const char * check;
    const char * nameAttribute;
    const char * descriptionAttribute;
    const char * type;
    const char * typeAttribute;
    const char * expression;
    const char * numerator;
    const char * denominator;
    const char * nominal;
    const char * uncertain;
    const char * parameterName;
    const char * parameterExpression;
    const char * range;
    const char * rangeMin;
    const char * rangeMax;
    const char * specifications;
    const char * specification;
    const char * used;
    const char * skipped;
    const char * minFrequency;
    const char * maxFrequency;
    const char * constant;
    const char * magnitude;
    const char * omega;
    const char * omegaMin;
    const char * omegaMax;
    const char * pointCount;
    const char * omegaType;
    const char * values;
    const char * templates;
    const char * metadata;
    const char * epsilon;
    const char * fullTemplates;
    const char * templateContour;
    const char * boundaries;
    const char * boundariesData;
    const char * phases;
    const char * phaseCountAttribute;
    const char * magnitudes;
    const char * magnitudeCountAttribute;
    const char * axisMin;
    const char * axisMax;
    const char * openFlags;
    const char * upperFlags;
    const char * perFrequency;
    const char * boundaryUnion;
    const char * unionBuckets;
    const char * loopShapingPointCountAttribute;
    const char * boundaryColumns;
    const char * sweep;
    const char * sweepParameter;
    const char * doiAttribute;
};

inline constexpr int kVersion = 4;

inline const Tags kV4 = {
    "inputs", "settings", "results",
    "plant", "controller", "loop-shaping", "check",
    "name", "description", "type", "id", "expression", "numerator", "denominator",
    "nominal", "uncertain", "name", "expr", "range", "min", "max",
    "specifications", "specification", "used", "skipped", "min-frequency",
    "max-frequency", "constant", "magnitude",
    "omega", "min", "max", "point-count", "type", "values",
    "templates", "metadata", "epsilon", "full", "contour",
    "boundaries", "data", "phases", "count", "magnitudes", "count",
    "min", "max", "open-flags", "upper-flags", "per-frequency",
    "union", "union-buckets",
    "point-count",
    "columns",
    "sweep", "parameter",
    "doi",
};

inline const char * familyNotCheckedName(FamilyStability::NotChecked why)
{
    switch (why) {
    case FamilyStability::NotChecked::No:            return "";
    case FamilyStability::NotChecked::NoSweepRecord: return "no-sweep-record";
    case FamilyStability::NotChecked::Delay:         return "delay";
    case FamilyStability::NotChecked::NotRational:   return "not-rational";
    }
    return "";
}

inline std::optional<FamilyStability::NotChecked> familyNotCheckedFromName(const std::string & name)
{
    if (name == "no-sweep-record") return FamilyStability::NotChecked::NoSweepRecord;
    if (name == "delay")           return FamilyStability::NotChecked::Delay;
    if (name == "not-rational")    return FamilyStability::NotChecked::NotRational;
    return std::nullopt;
}

inline const char * algorithmName(LoopShapingAlgorithm algorithm)
{
    switch (algorithm) {
    case nt:        return "nt";
    case nk:        return "nk";
    case mr:        return "mr";
    case mc1:       return "mc1";
    case mc_thesis: return "mc-thesis";
    case mc2:       return "mc2";
    case mc3:       return "mc3";
    }

    return "nt";
}

inline std::optional<LoopShapingAlgorithm> algorithmFromName(const std::string & name)
{
    for (const LoopShapingAlgorithm algorithm : {nt, nk, mr, mc1, mc_thesis, mc2, mc3}) {
        if (name == algorithmName(algorithm)) {
            return algorithm;
        }
    }

    return std::nullopt;
}

}

#endif
