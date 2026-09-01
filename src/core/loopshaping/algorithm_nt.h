#ifndef QFTBX_LOOPSHAPING_ALGORITHM_NT_H
#define QFTBX_LOOPSHAPING_ALGORITHM_NT_H

#include <QVector>
#include <QHash>
#include <cmath>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/loopshaping/search_node.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
#include "src/core/loopshaping/nominal_stability_checker.h"
#include "src/core/loopshaping/ordered_list.h"

#include "src/core/loopshaping/common_functions.h"



class AlgorithmNt
{
public:
    AlgorithmNt();
    ~AlgorithmNt();

    void set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> *omega, const BoundaryData * boundaries,
                    qreal epsilon);

    bool init_algorithm();

    LtiSystem * controllerStructure();


private:

    inline void check_box_feasibility(LtiSystem *controlador);
    inline LtiSystem *acelerated(LtiSystem * v, qreal minimo_boundarie, qreal o, qint32 contador, bool arriba);
    inline bool feasibleGainFrom(LtiSystem * v, qreal maximo_boundarie, cxsc::cinterval caja,
                                 qreal o, qint32 contador, qreal & from);

    LtiSystem * planta;
    LtiSystem * controlador;
    QVector <qreal> * omega;
    const BoundaryData * boundaries = nullptr;
    NaturalIntervalExtension * conversion = nullptr;
    OrderedList * lista = nullptr;
    qreal epsilon;

    LtiSystem * controlador_retorno = nullptr;
    qreal minimo_boundaries;


    QPointF interseccion (QPointF uno, QPointF dos);


    qint32 tamFas;

    BoundaryViolationDetector * deteccion = nullptr;
    NominalStabilityChecker * stability = nullptr;
    QVector <complex> * plantas_nominales = nullptr;

};

#endif // QFTBX_LOOPSHAPING_ALGORITHM_NT_H
