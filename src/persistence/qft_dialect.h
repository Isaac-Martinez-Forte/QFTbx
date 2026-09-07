#ifndef QFTBX_QFT_DIALECT_H
#define QFTBX_QFT_DIALECT_H

//Internal to src/persistence: the tag names of the .qft dialects, shared by
//the reader and the writer.

namespace qftbx {

/**
 * @brief The tag names of the .qft format, shared by the reader and the
 * writer so the two cannot drift apart.
 *
 * There used to be a second table, kLegacy, holding the historical Spanish
 * names (<Planta>, <especificaciones>, <tamFas>...) for files with no version
 * attribute. Version 2 is the only format now: every .qft in the repository
 * and in documentos/plantas was converted, through the reader and the writer
 * themselves, and the reader refuses anything that is not version 2 rather
 * than guessing at it.
 *
 * The indirection stays even with a single table, because that is not what it
 * was for: one place names the format, and neither side can rename a tag
 * without the other following.
 */
struct Tags {
    const char * plant;
    const char * controller;
    const char * loopShaping;
    const char * nameAttribute;
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
};

inline const Tags kV2 = {
    "plant", "controller", "loop-shaping",
    "name", "type", "id", "expression", "numerator", "denominator",
    "nominal", "uncertain", "name", "expr", "range", "min", "max",
    "specifications", "specification", "used", "min-frequency",
    "max-frequency", "constant", "magnitude",
    "omega", "min", "max", "point-count", "type", "values",
    "templates", "metadata", "epsilon", "full", "contour",
    "boundaries", "data", "phases", "count", "magnitudes", "count",
    "min", "max", "open-flags", "upper-flags", "per-frequency",
    "union", "union-buckets",
    "point-count",
};

} // namespace qftbx

#endif // QFTBX_QFT_DIALECT_H
