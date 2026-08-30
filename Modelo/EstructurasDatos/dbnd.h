#ifndef QFTBX_DBND_H
#define QFTBX_DBND_H

#include <cmath>

#include <QString>

#include "src/core/system/lti_system.h"

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

} // namespace tools

#endif // QFTBX_DBND_H
