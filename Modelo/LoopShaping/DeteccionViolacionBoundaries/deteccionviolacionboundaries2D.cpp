#include "DeteccionViolacionBoundaries2D.h"

using namespace std;
using namespace tools;
using namespace intervalares;


DeteccionViolacionBoundaries2D::DeteccionViolacionBoundaries2D()
{
    ejecutado = false;

}

DeteccionViolacionBoundaries2D::~DeteccionViolacionBoundaries2D()
{

}

flags_box DeteccionViolacionBoundaries2D::deteccionViolacion(QPointF punto, bool ** interseccion, qint32 totalFase, qint32 totalMagnitud, qint32 minDB)
{
    qint32 violacion = 0;
    if(punto.y()>0)
    {
        if(!interseccion[(qint32)round(punto.x())+totalFase][(qint32)round(punto.y())+minDB+totalMagnitud]) violacion++;
    }
    else if(punto.y()==0)
    {
        if(!interseccion[(qint32)round(punto.x())+totalFase][(qint32)round(punto.y())+minDB+totalMagnitud]||!interseccion[(qint32)round(punto.x())
                +totalFase][(qint32)round(punto.y())+minDB+totalMagnitud-1]) violacion++;
    }
    else
    {
        if(!interseccion[(qint32)round(punto.x())+totalFase][(qint32)round(punto.y())+minDB+totalMagnitud-1]) violacion++;
    }
    if(!violacion) return feasible;
    else if(violacion) return infeasible;
    else return ambiguous;
}

QVector<flags_box> DeteccionViolacionBoundaries2D::deteccionViolacionCaja(cinterval<qreal> intervalo, bool ** interseccion,
                                                                          qint32 totalFase, qint32 totalMagnitud, qint32 minDB)
{
    /*
        Los 4 puntos son las 4 esquinas del rectángulo definido por esos 4 números:
        - punto 1: (fmin, mmin)
        - punto 2: (fmin, mmax)
        - punto 3: (fmax, mmin)
        - punto 4: (fmax, mmax)*/

    interval<qreal> g = intervalo.re;
    interval<qreal> theta = intervalo.im;
    QPointF punto, puntoSuperiorIzquierdo, puntoInferiorDerecho;
    punto.setX(theta.inf);
    punto.setY(g.inf);
    QVector<flags_box> violaciones;
    violaciones.append(deteccionViolacion(punto,interseccion,totalFase,totalMagnitud,minDB));
    punto.setX(theta.inf);
    punto.setY(g.sup);
    //Guardamos las coordenadas del punto superior izquierdo de la caja para posteriormente inicializar el recorrido
    //de la parte del grid correspondiente a la caja de ser necesario par determinar los máximos y mínimos
    puntoSuperiorIzquierdo.setX(punto.x());
    puntoSuperiorIzquierdo.setY(punto.y());
    violaciones.append(deteccionViolacion(punto,interseccion,totalFase,totalMagnitud,minDB));
    punto.setX(theta.sup);
    punto.setY(g.inf);
    //Guardamos las coordenadas del punto inferior derecho para terminar el recorrido de ser necesario.
    puntoInferiorDerecho.setX(punto.x());
    puntoInferiorDerecho.setY(punto.y());
    violaciones.append(deteccionViolacion(punto,interseccion,totalFase,totalMagnitud,minDB));
    punto.setX(theta.sup);
    punto.setY(g.sup);
    violaciones.append(deteccionViolacion(punto,interseccion,totalFase,totalMagnitud,minDB));
    bool hayAmbiguedad = false;
    foreach (flags_box flag, violaciones) {
        if(flag == ambiguous) hayAmbiguedad = true;
    }
    qreal minMagnitudCaja = numeric_limits<qreal>::max(), maxMagnitudCaja = numeric_limits<qreal>::min(),
            minFaseCaja = numeric_limits<qreal>::max(), maxFaseCaja = numeric_limits<qreal>::min();

    if(!hayAmbiguedad)
    {
        //En realidad no hay ni máximos ni mínimos porque toda la caja es zona permitida (No hay frontera entre zona permitida y zona no permitida)
        minMagnitudCaja = 0;
        maxMagnitudCaja = 0;
        minFaseCaja = 0;
        maxFaseCaja = 0;
    }
    else //Si viola aunque sólo sea uno de los puntos de la caja
    {
        //Recorremos la parte del grid correspondiente a la caja para determinar los máximos y los mínimos
        for(qint32 i = puntoSuperiorIzquierdo.x();i<puntoInferiorDerecho.x();i++)
        {
            for(qint32 j = puntoSuperiorIzquierdo.y();i<puntoInferiorDerecho.y();j++)
            {
                QPointF puntoRecorrido(i,j);
                if(deteccionViolacion(puntoRecorrido,interseccion,totalFase,totalMagnitud,minDB) != feasible)
                {
                    if(j<minMagnitudCaja) minMagnitudCaja = j;
                    if(j>maxMagnitudCaja) maxMagnitudCaja = j;
                    if(i<minFaseCaja) minFaseCaja = i;
                    if(i>maxFaseCaja) maxFaseCaja = i;
                }
            }
        }
    }

    if (ejecutado){
        minimosMaximos->clear();
    }

    minimosMaximos = new QVector <qreal> ();

    ejecutado = true;

    minimosMaximos->append(minMagnitudCaja);
    minimosMaximos->append(maxMagnitudCaja);
    minimosMaximos->append(minFaseCaja);
    minimosMaximos->append(maxFaseCaja);

    return violaciones;
}

QVector <qreal> * DeteccionViolacionBoundaries2D::getMinimosMaximos(){
    return minimosMaximos;
}
