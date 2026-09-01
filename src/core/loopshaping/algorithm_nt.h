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

    /// The designed controller, handed over to the caller.
    std::unique_ptr<LtiSystem> controllerStructure();

    /// The most boxes the search kept alive at once (see kDefaultMaxLiveNodes).
    std::size_t peakLiveNodes() const;


private:

    inline void check_box_feasibility(std::unique_ptr<LtiSystem> box);
    inline std::unique_ptr<LtiSystem> acelerated(std::unique_ptr<LtiSystem> v, qreal minimo_boundarie,
                                                 qreal o, qint32 contador, bool arriba);
    inline bool feasibleGainFrom(LtiSystem * v, qreal maximo_boundarie, cxsc::cinterval caja,
                                 qreal o, qint32 contador, qreal & from);

    LtiSystem * planta;
    std::unique_ptr<LtiSystem> controlador;
    QVector <qreal> * omega;
    const BoundaryData * boundaries = nullptr;
    std::unique_ptr<NaturalIntervalExtension> conversion;
    std::unique_ptr<OrderedList> lista;
    qreal epsilon;

    std::unique_ptr<LtiSystem> controlador_retorno;
    qreal minimo_boundaries;


    QPointF interseccion (QPointF uno, QPointF dos);


    qint32 tamFas;

    std::unique_ptr<BoundaryViolationDetector> deteccion;
    std::unique_ptr<NominalStabilityChecker> stability;
    QVector <complex> plantas_nominales;

};

#endif // QFTBX_LOOPSHAPING_ALGORITHM_NT_H
