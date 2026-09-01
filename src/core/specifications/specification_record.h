#ifndef QFTBX_SPECIFICATION_RECORD_H
#define QFTBX_SPECIFICATION_RECORD_H

#include <cmath>

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
    //Initialised here so that a plain `SpecificationRecord r;` is as safe as
    //the `new SpecificationRecord()` every caller happens to use today:
    //value-initialisation zeroes the members, a declaration without it does
    //not, and `system` is deleted by the owners.
    QString name;
    bool used = false;
    LtiSystem * system = nullptr;
    qreal height = 0.0;    //LINEAR magnitude (heightDb() converts)
    bool constant = false;
    qreal omegaStart = 0.0;
    qreal omegaEnd = 0.0;

    //Deep copy: the clone owns a fresh copy of the embedded plant.
    SpecificationRecord * clone() const {
        SpecificationRecord * copy = new SpecificationRecord(*this);
        if (system != nullptr){
            //The record keeps its plant as a raw pointer its owners delete:
            //release() marks the one place where that contract is entered.
            copy->system = system->clone().release();
        }
        return copy;
    }

    qreal heightDb(qreal omega) {
        if (constant){
            return 20 * std::log10(height);
        }

        return 20 * std::log10(std::abs(system->evaluate(omega)));
    }
};

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
    //fromSystem takes the plant over (Specification deletes it).
    return Specification::fromSystem(type, d.system->clone().release(),
                                     d.omegaStart, d.omegaEnd);
}

inline SpecificationSet toSpecificationSet(const QVector<SpecificationRecord *> & specs){
    SpecificationSet set;
    for (int i = 0; i < kSpecificationCount && i < specs.size(); ++i){
        set.set(toSpecification(*specs.at(i), static_cast<SpecificationType>(i)));
    }
    return set;
}


} // namespace qftbx

#endif // QFTBX_SPECIFICATION_RECORD_H
