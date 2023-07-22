#ifndef TEMPLATES_H
#define TEMPLATES_H

#include "QVector"
#include <complex>
#include <complex>
#include <limits>
#include <qmath.h>
#include <QHash>


#include "../Herramientas/tools.h"
#include "../EstructurasDatos/var.h"
#include "../EstructuraSistema/funciontransferencia.h"
#include "../EstructuraSistema/sistema.h"


#include "muparserx/mpParser.h"

#ifdef OpenMP_AVAILABLE
    #include <omp.h>
#endif


/**
    * @class Templates
    * @brief Clase que resuelve el algoritmo de cálculo de templates y el cálculo del contorno de los templates.
    *
    * Esta clase resuelve el cálculo por fuerza bruta, en un futuro se puede plantear resolver este mismo problema con otros algoritmos más rápidos.
    *
    * @author Isaac Martínez Forte
   */


class Templates
{
public:

    /**
    * @fn Templates
    * @brief Constructor por defecto de la clase.
    *
   */

    Templates();
    
    
    /**
     * @fn ~Templates
     * @brief Destructor de la clase.
     */
    
    ~Templates();


    /**
      * @fn lanzarCalculo
      * @brief Función que lanza el cálculo de los templates y su contorno.
      *
      * @param planta de la que calcular el Template.
      * @param Omega vector de reales que representa las frecuencias de diseño para calcular el algoritmo.
      * @param cuda booleano que indica si se quiere utilizar CUDA para resolver el contorno de los templates.
      *
      * @return booleano indicando el cálculo se ha realizado correctamente.
      */

    bool lanzarCalculo(Sistema *planta, QVector<qreal>* omega, bool cuda);

    bool lanzarCalculoContorno (QVector <qreal> * epsilon);

    /**
    * @fn calcularTemplate
    * @brief Función que resuelve el algoritmo de cálculo de Templates.
    *
    * @param planta de la que calcular el template.
    * @param Omega vector de reales que representa las frecuencias de diseño para calcular el algoritmo.
    *
    * @return templates calculados.
    */
    
#ifndef OpenMP_AVAILABLE
    QVector<QVector<std::complex<qreal> > *> * calcularTemplate_secuencial(Sistema *planta, QVector<qreal>* omega);
#else
    QVector<QVector<std::complex<qreal> > *> * calcularTemplate_paralelo(Sistema *planta, QVector<qreal>* omega);
#endif

    /**
      * @fn calcularContorno
      * @brief Función que lanza el cálculo del contorno de todas las frecuencias de diseño.
      *
      * @return Booleano que indica si el cálculo ha funcionado corréctamente.
      *
      */
    
    bool calcularContorno(bool cuda);
    
    /**
      * @fn e_hull
      * @brief Función que calcula la envolvente de los templates
      *
      * @param Template calculados previamente
      * @param Epsilon distancia máxima entre dos puntos de la envolvente.
      * @param cuda booleano que indica si se quiere utilizar CUDA para resolver el contorno de los templates.
      *
      * @return Vector con los puntos del contorno resultante.
      */

    QVector <std::complex <qreal> > * e_hull(QVector<std::complex<qreal> > *temp, qreal epsilon);
    
    
    /**
      * @fn setMapa
      * @brief Función que guarda un mapa con los valores de las variables.
      *
      * @param mapa Valores de las variables.
      */

    void setMapa (QHash<Var *, QVector<qreal> *> * mapa);
    
    
    /**
      * @fn setEpsilon
      * @brief Función que guarda el valor de epsilon.
      *
      * @param epsilon valor a utilizar en el algoritmo de e_hull.
      */

    void setEpsilon (QVector <qreal> * epsilon);
    
    
    /**
      * @fn getTemplates
      * @brief Función que retorna los templates calculados
      *
      * @return templates calculados.
      */

    QVector<QVector<std::complex<qreal> > *> * getTemplates();
    
    
    /**
      * @fn getContorno
      * @brief Función que retorna el contorno de los templates.
      *
      * @return contorno de los templates.
      */
    
    QVector<QVector<std::complex<qreal> > *> * getContorno();

    QVector <qreal> * getOmega();

    QVector <qreal> * getEpsilon ();

private:
    QVector<qreal> * getVariables(Var *a);

    QHash <Var *, QVector<qreal> * > * mapa = NULL;
    qint32 combinaciones;
    QVector <qreal> * epsilon;
    bool cuda;
    bool contornoCalculado;

    QVector<QVector<std::complex<qreal> > *> * templates = NULL;
    QVector<QVector<std::complex<qreal> > *> * contorno = NULL;
    QVector <qreal> * omega;

    qint32 buscarSegundo(qint32 b1, QVector<std::complex<qreal> > *cv, qreal epsilon);
    qint32 buscarSiguiente(qint32 punto_previo, qint32 punto_actual, QVector<std::complex<qreal> > *cv, qreal epsilon);

};

#endif // TEMPLATES_H
