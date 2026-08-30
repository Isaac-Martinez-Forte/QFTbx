#ifndef INTERVAL_COMPLEX_H
#define INTERVAL_COMPLEX_H

#include <QVector>
#include <QPointF>
#include <math.h>

#include "src/core/system/parameter.h"
#include "src/core/system/lti_system.h"

#include <complex>

#include "interval.hpp"
#include "cinterval.hpp"

using namespace cxsc;

class Natura_Interval_extension
{

public:
    Natura_Interval_extension();
    ~Natura_Interval_extension();

    cinterval get_box(LtiSystem * sistema, qreal w, complex p0, bool nyquist = false);
    
    cinterval get_box_nume (QVector <Parameter * > * nume, qreal w, LtiSystem::SystemType tipo, bool nyquist = false);
    cinterval get_box_deno (QVector <Parameter * > * deno, qreal w, LtiSystem::SystemType tipo, bool nyquist = false);

    qreal getBoxInf();
    cxsc::cinterval getBoxDB();

    cinterval get_box_termino_nume(Parameter * var, qreal w, complex p0);
    cinterval get_box_termino_deno(Parameter * var, qreal w, complex p0);
    cinterval get_box_termino_k(Parameter * var, complex p0);

private:

    inline cinterval get_box_kganancia (LtiSystem * sistema, qreal w);
    inline cinterval get_box_knoganacia (LtiSystem * sistema, qreal w);
    inline cinterval get_box_cpolinomios (LtiSystem * sistema, qreal w);
    inline cinterval get_box_flibre (LtiSystem * sistema, qreal w);

    inline cinterval get_box_kganancia_nume (QVector <Parameter * > * nume, qreal w);
    inline cinterval get_box_kganancia_deno (QVector <Parameter * > * deno, qreal w);
    inline interval _arg(cinterval z);

    qreal boxInf;
    cinterval boxDB;

};

#endif // INTERVAL_COMPLEX_H
