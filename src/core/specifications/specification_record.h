/**
 * @file
 * @brief The editing and persistence record of a specification slot.
 *
 * Declares the raw form of a specification as the interface and the .qft
 * files handle it: plain values with no invariants, since a slot being
 * edited may be temporarily invalid. The height is a linear magnitude, not
 * dB, and skipped lists the design frequencies of the band the user took
 * out of this specification, empty for the whole band. A record owns its
 * plant, so it is move-only and clone() is the deliberate deep copy. The
 * seven records are a fixed positional array indexed by specification
 * type, which the persistence writes in that order. The engines never read
 * a record directly: toSpecification() is the validating conversion to the
 * engine-facing specification, where the invariants are enforced, and it
 * throws qftbx::InvalidInput when the height is not positive, the band is
 * inverted or the plant is missing.
 */

#ifndef QFTBX_SPECIFICATION_RECORD_H
#define QFTBX_SPECIFICATION_RECORD_H

#include <array>
#include <memory>
#include <vector>

#include <string>

#include "src/core/system/lti_system.h"
#include "src/core/specifications/specification.h"

namespace qftbx {

struct SpecificationRecord {
    std::string name;
    bool used = false;
    std::unique_ptr<LtiSystem> system;
    double height = 0.0;
    bool constant = false;
    double omegaStart = 0.0;
    double omegaEnd = 0.0;
    std::vector<double> skipped;

    SpecificationRecord clone() const {
        SpecificationRecord copy;
        copy.name = name;
        copy.used = used;
        copy.height = height;
        copy.constant = constant;
        copy.omegaStart = omegaStart;
        copy.omegaEnd = omegaEnd;
        copy.skipped = skipped;

        if (system != nullptr){
            copy.system = system->clone();
        }

        return copy;
    }
};

using SpecificationRecords = std::array<SpecificationRecord, kSpecificationCount>;

inline Specification toSpecification(const SpecificationRecord & d, SpecificationType type){
    if (!d.used){
        return Specification::unused(type);
    }
    if (d.constant){
        return Specification::constant(type, d.height, d.omegaStart, d.omegaEnd, d.skipped);
    }
    if (d.system == nullptr){
        throw InvalidInput(QFTBX_TR("Core", "A used specification needs a plant or a constant height."));
    }
    return Specification::fromSystem(type, d.system->clone(),
                                     d.omegaStart, d.omegaEnd, d.skipped);
}

inline SpecificationSet toSpecificationSet(const SpecificationRecords & specs){
    SpecificationSet set;
    for (std::size_t i = 0; i < kSpecificationCount; ++i){
        set.set(toSpecification(specs.at(i),
                                static_cast<SpecificationType>(i)));
    }
    return set;
}

}

#endif
