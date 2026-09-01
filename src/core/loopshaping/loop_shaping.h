#ifndef QFTBX_LOOPSHAPING_LOOP_SHAPING_H
#define QFTBX_LOOPSHAPING_LOOP_SHAPING_H

#include "src/core/templates/cloud_set.h"
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

    bool run(LtiSystem * plant, LtiSystem * controller, QVector<qreal> * omega, BoundaryData * boundaries,
                 qreal epsilon, LoopShapingAlgorithm algorithm,
                 const qftbx::CloudSet & contour, QVector<qftbx::SpecificationRecord *> * specifications,
                 qint32 initialisation);

    LtiSystem * controllerStructure();

private:

    LtiSystem * controller;
};

#endif // QFTBX_LOOPSHAPING_LOOP_SHAPING_H
