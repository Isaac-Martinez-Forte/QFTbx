#ifndef DETECCIONVIOLACIONNINI_H
#define DETECCIONVIOLACIONNINI_H

#include <QPointF>
#include <limits>
#include "Modelo/Herramientas/tools.h"
#include "Modelo/EstructurasDatos/datosbound.h"
#include "Modelo/LoopShaping/EstructuraDatos/data_box.h"
#include "Modelo/LoopShaping/EstructuraDatos/etapas.h"

#include <cinterval.hpp>
#include <interval.hpp>
#include <complex>
#include <cmath>

#include "GUI/viewboundreun.h"

/**
 * @class DeteccionViolacionBoundaries
 * @brief Clase que usa los datos calculados en la intersección de boundaries 1D para detectar si unos puntos dados violan o no violan.
 *
 * @author Moisés Frutos Plaza
 * @author Isaac Martínez Forte
 */

using namespace cxsc;

class DeteccionViolacionBoundaries
{
public:

    DeteccionViolacionBoundaries();
    ~DeteccionViolacionBoundaries();

    data_box * deteccionViolacionCajaNiNi(cinterval box, std::shared_ptr<DatosBound> boundaries, qint32 contador);
    data_box * deteccionViolacionCajaNiNi(cinterval box, std::shared_ptr<DatosBound> boundaries, qint32 contador, Etapas e);

    data_box * deteccionViolacionCajaNi(cinterval box, std::shared_ptr<DatosBound> boundaries, qint32 contador);

    data_box * deteccionViolacionCajaNyNi(cinterval box, std::shared_ptr<DatosBound> boundaries, qint32 contador);

    //tools::recortes deteccionViolacionCajaNyNy(cinterval box, DatosBound * boundaries, qint32 contador);

private:


    inline tools::flags_box deteccionViolacion(QPointF punto, QVector< QVector<QPointF> * > * interseccionHash,
                                        qint32 totalFase, bool abierta, bool arriba, qint32 numeroFases);
    inline qint32 funcionHash(qreal x, qreal totalFase, qint32 numeroFases);
    inline qint32 funcionHashNy(qreal x, qreal totalFase, qint32 numeroFases, qreal minFas);
    inline qint32 interseccionCajaCirculo(cinterval box, qreal radioMayor, qreal radioMenor, QPointF centroCirculo);

    ////////////////////////////////7

    inline bool seg_intersection(std::complex<qreal> u1, std::complex<qreal> u2, std::complex<qreal> v1, std::complex<qreal> v2);

    inline qint32 side_p_to_seg(std::complex<qreal> v1, std::complex<qreal> v2, std::complex<qreal> p);

    inline qreal _arg(std::complex <qreal> c);
    inline cxsc::interval _arg(cinterval z);

    //////////////////////////

    bool cambioEtapas = false;

    qint32 c = 0;

};

#endif // DETECCIONVIOLACIONNINI_H
