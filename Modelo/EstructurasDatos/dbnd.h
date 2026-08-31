#ifndef QFTBX_DBND_H
#define QFTBX_DBND_H

#include <cmath>

#include <QString>

#include "src/core/system/lti_system.h"
#include "src/core/specifications/specification.h"

//Transitional home for the specification record, moved out of tools.h.
//It will become the specifications module in phase 4.
namespace tools {

struct dBND{
    QString nombre;
    bool utilizado;
    LtiSystem * sistema;
    qreal altura;
    bool constante;
    qreal frecinicio;
    qreal frecfinal;

    //Deep copy: the clone owns a fresh copy of the embedded plant.
    dBND * clone() const {
        dBND * copy = new dBND(*this);
        if (sistema != nullptr){
            copy->sistema = sistema->clone();
        }
        return copy;
    }

    qreal getAltura(qreal omega) {
        if (constante){
            return 20 * std::log10(altura);
        }

        return 20 * std::log10(std::abs(sistema->evaluate(omega)));
    }
};

//Conversion transitoria del registro historico al tipo validado. Lanza
//qftbx::InvalidInput si el registro viola los invariantes (altura <= 0,
//banda invertida, planta nula): la robustez que el registro no imponia.
inline qftbx::Specification toSpecification(const dBND & d, qftbx::SpecificationType type){
    if (!d.utilizado){
        return qftbx::Specification::unused(type);
    }
    if (d.constante){
        return qftbx::Specification::constant(type, d.altura, d.frecinicio, d.frecfinal);
    }
    if (d.sistema == nullptr){
        throw qftbx::InvalidInput("A used specification needs a plant or a constant height.");
    }
    return qftbx::Specification::fromSystem(type, d.sistema->clone(), d.frecinicio, d.frecfinal);
}

inline qftbx::SpecificationSet toSpecificationSet(const QVector<dBND *> & specs){
    qftbx::SpecificationSet set;
    for (int i = 0; i < qftbx::kSpecificationCount && i < specs.size(); ++i){
        set.set(toSpecification(*specs.at(i), static_cast<qftbx::SpecificationType>(i)));
    }
    return set;
}

} // namespace tools

#endif // QFTBX_DBND_H
