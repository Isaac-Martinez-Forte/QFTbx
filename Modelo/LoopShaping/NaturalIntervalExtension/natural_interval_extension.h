#ifndef INTERVAL_COMPLEX_H
#define INTERVAL_COMPLEX_H

#include <QVector>
#include <QPointF>
#include <math.h>

#include "Modelo/EstructurasDatos/var.h"
#include "Modelo/EstructuraSistema/sistema.h"

#include <complex>

#include "interval.hpp"
#include "cinterval.hpp"

using namespace cxsc;

class Natura_Interval_extension
{

public:
    Natura_Interval_extension();
    ~Natura_Interval_extension();

    cinterval get_box(Sistema * sistema, qreal w, complex p0, bool nyquist = false);
    
    cinterval get_box_nume (QVector <Var * > * nume, qreal w, Sistema::tipo_planta tipo, bool nyquist = false);
    cinterval get_box_deno (QVector <Var * > * deno, qreal w, Sistema::tipo_planta tipo, bool nyquist = false);

    qreal getBoxInf();
    cxsc::cinterval getBoxDB();

    cinterval get_box_termino_nume(Var * var, qreal w, complex p0);
    cinterval get_box_termino_deno(Var * var, qreal w, complex p0);
    cinterval get_box_termino_k(Var * var, complex p0);

private:

    inline cinterval get_box_kganancia (Sistema * sistema, qreal w);
    inline cinterval get_box_knoganacia (Sistema * sistema, qreal w);
    inline cinterval get_box_cpolinomios (Sistema * sistema, qreal w);
    inline cinterval get_box_flibre (Sistema * sistema, qreal w);

    inline cinterval get_box_kganancia_nume (QVector <Var * > * nume, qreal w);
    inline cinterval get_box_kganancia_deno (QVector <Var * > * deno, qreal w);
    inline interval _arg(cinterval z);

    qreal boxInf;
    cinterval boxDB;

};

#endif // INTERVAL_COMPLEX_H
