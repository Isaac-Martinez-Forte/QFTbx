#ifndef QFTBX_SPECIFICATION_RECORD_H
#define QFTBX_SPECIFICATION_RECORD_H

#include <array>
#include <cmath>
#include <memory>

#include <QString>

#include "src/core/system/lti_system.h"
#include "src/core/specifications/specification.h"

namespace qftbx {

//Editing/persistence record of one specification slot, as the GUI and the
//.qft files handle it: raw values, no invariants (a slot being edited can
//be temporarily invalid). The engines never consume it directly: they take
//the validated Specification produced by toSpecification(), which is where
//the invariants are enforced.
struct SpecificationRecord {
    QString name;
    bool used = false;
    //The record OWNS its plant. It used to be a raw pointer whose owners
    //had to walk the container and delete it, in four different places.
    std::unique_ptr<LtiSystem> system;
    qreal height = 0.0;    //LINEAR magnitude (heightDb() converts)
    bool constant = false;
    qreal omegaStart = 0.0;
    qreal omegaEnd = 0.0;

    //Deep copy: the copy owns a fresh copy of the embedded plant. Explicit
    //because owning the plant makes the record move-only, which is the
    //point: a copy of a specification is always deliberate.
    SpecificationRecord clone() const {
        SpecificationRecord copy;
        copy.name = name;
        copy.used = used;
        copy.height = height;
        copy.constant = constant;
        copy.omegaStart = omegaStart;
        copy.omegaEnd = omegaEnd;

        if (system != nullptr){
            copy.system = system->clone();
        }

        return copy;
    }

    qreal heightDb(qreal omega) const {
        if (constant){
            return 20 * std::log10(height);
        }

        return 20 * std::log10(std::abs(system->evaluate(omega)));
    }
};

/**
 * @brief The seven editing slots, positional: every consumer indexes them
 * by SpecificationType, and the persistence writes them in that order.
 *
 * By value, with each record owning its plant. This was a POINTER to a
 * QVector of POINTERS, so four modules carried the same nested deletion
 * loop and the size was never checked outside the reader.
 */
using SpecificationRecords = std::array<SpecificationRecord, kSpecificationCount>;

//Validating conversion to the engine-facing type. Throws qftbx::InvalidInput
//when the record breaks the invariants (height <= 0, inverted band, null
//plant): the robustness the raw record does not impose.
inline Specification toSpecification(const SpecificationRecord & d, SpecificationType type){
    if (!d.used){
        return Specification::unused(type);
    }
    if (d.constant){
        return Specification::constant(type, d.height, d.omegaStart, d.omegaEnd);
    }
    if (d.system == nullptr){
        throw InvalidInput("A used specification needs a plant or a constant height.");
    }
    return Specification::fromSystem(type, d.system->clone(),
                                     d.omegaStart, d.omegaEnd);
}

inline SpecificationSet toSpecificationSet(const SpecificationRecords & specs){
    SpecificationSet set;
    for (int i = 0; i < kSpecificationCount; ++i){
        set.set(toSpecification(specs.at(static_cast<std::size_t>(i)),
                                static_cast<SpecificationType>(i)));
    }
    return set;
}


} // namespace qftbx

#endif // QFTBX_SPECIFICATION_RECORD_H
