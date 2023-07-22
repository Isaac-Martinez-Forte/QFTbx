#ifndef LOOPSHAPING_H
#define LOOPSHAPING_H

#include "Modelo/LoopShaping/algorithm_sachin.h"
#include "Modelo/LoopShaping/algorithm_nandkishor.h"
#include "Modelo/LoopShaping/algorithm_rambabu.h"
#include "Modelo/LoopShaping/algorithm_primer_articulo.h"
#include "Modelo/LoopShaping/algorithm_segundo_articulo.h"
#include "Modelo/EstructuraSistema/sistema.h"
#include "Modelo/EstructurasDatos/datosbound.h"

#include "GUI/verboundaries.h"

class LoopShaping
{
public:
    LoopShaping();
    ~LoopShaping();

    bool iniciar(Sistema * planta, Sistema * controlador, QVector<qreal> *omega, DatosBound * boundaries,
                   qreal epsilon, alg_loop_shaping seleccionado, bool depuracion, qreal delta,
                 QVector<QVector<std::complex<qreal> > *> *temp, QVector<dBND *> *espe, qint32 inicializacion,
                 bool hilos, bool bisection_avanced, bool deteccion_avanced, bool a);

    Sistema * getControlador();

private:

    Sistema * controlador;
};

#endif // LOOPSHAPING_H
