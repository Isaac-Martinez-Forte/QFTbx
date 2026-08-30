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

    qreal getAltura(qreal omega) {
        if (constante){
            return 20 * std::log10(altura);
        }

        return 20 * std::log10(std::abs(sistema->evaluate(omega)));
    }
};

} // namespace tools

#endif // QFTBX_DBND_H
