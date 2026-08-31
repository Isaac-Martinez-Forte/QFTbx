#ifndef LOOPSHAPING_H
#define LOOPSHAPING_H

#include "Modelo/LoopShaping/algorithm_nt.h"
#include "Modelo/LoopShaping/algorithm_nk.h"
#include "Modelo/LoopShaping/algorithm_mr.h"
#include "Modelo/LoopShaping/algorithm_mc1.h"
#include "Modelo/LoopShaping/algorithm_mc_thesis.h"
#include "src/core/system/lti_system.h"
#include "src/core/boundaries/boundary_data.h"


class LoopShaping
{
public:
    LoopShaping();
    ~LoopShaping();

    bool iniciar(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> * omega, BoundaryData * boundaries,
                 qreal epsilon, alg_loop_shaping seleccionado,
                 QVector<QVector<std::complex<qreal>> *> * temp, QVector<dBND *> * espe,
                 qint32 inicializacion);

    LtiSystem * getControlador();

private:

    LtiSystem * controlador;
};

#endif // LOOPSHAPING_H
