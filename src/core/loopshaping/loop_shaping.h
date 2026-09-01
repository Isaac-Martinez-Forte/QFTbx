#ifndef QFTBX_LOOPSHAPING_LOOP_SHAPING_H
#define QFTBX_LOOPSHAPING_LOOP_SHAPING_H

#include "src/core/loopshaping/algorithm_nt.h"
#include "src/core/loopshaping/algorithm_nk.h"
#include "src/core/loopshaping/algorithm_mr.h"
#include "src/core/loopshaping/algorithm_mc1.h"
#include "src/core/loopshaping/algorithm_mc_thesis.h"
#include "src/core/system/lti_system.h"
#include "src/core/boundaries/boundary_data.h"


class LoopShaping
{
public:
    LoopShaping();
    ~LoopShaping();

    bool iniciar(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> * omega, BoundaryData * boundaries,
                 qreal epsilon, LoopShapingAlgorithm seleccionado,
                 QVector<QVector<std::complex<qreal>> *> * temp, QVector<qftbx::SpecificationRecord *> * espe,
                 qint32 inicializacion);

    LtiSystem * getControlador();

private:

    LtiSystem * controlador;
};

#endif // QFTBX_LOOPSHAPING_LOOP_SHAPING_H
