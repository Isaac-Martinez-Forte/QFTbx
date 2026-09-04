#ifndef QFTBX_SPECIFICATION_RECORD_H
#define QFTBX_SPECIFICATION_RECORD_H

#include <array>
#include <memory>

#include <string>

#include "src/core/system/lti_system.h"
#include "src/core/specifications/specification.h"

namespace qftbx {

//Editing/persistence record of one specification slot, as the GUI and the
//.qft files handle it: raw values, no invariants (a slot being edited can
//be temporarily invalid). The engines never consume it directly: they take
//the validated Specification produced by toSpecification(), which is where
//the invariants are enforced.
struct SpecificationRecord {
    std::string name;
    bool used = false;
    //The record OWNS its plant. It used to be a raw pointer whose owners
    //had to walk the container and delete it, in four different places.
    std::unique_ptr<LtiSystem> system;
    double height = 0.0;    //LINEAR magnitude (heightDb() converts)
    bool constant = false;
    double omegaStart = 0.0;
    double omegaEnd = 0.0;

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

    //heightDb() lived here: the same formula as Specification::boundDb(),
    //written a second time on the raw record, dereferencing a null system on
    //a record that had none. Nothing in the program called it; the bound is
    //asked of the validated Specification, which is what toSpecification()
    //is for.
};

/**
 * @brief The seven editing slots, positional: every consumer indexes them
 * by SpecificationType, and the persistence writes them in that order.
 *
 * By value, with each record owning its plant. This was a POINTER to a
 * std::vector of POINTERS, so four modules carried the same nested deletion
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
    for (std::size_t i = 0; i < kSpecificationCount; ++i){
        set.set(toSpecification(specs.at(i),
                                static_cast<SpecificationType>(i)));
    }
    return set;
}


} // namespace qftbx

#endif // QFTBX_SPECIFICATION_RECORD_H
