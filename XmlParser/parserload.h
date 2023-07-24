#ifndef XMLPARSERLOAD_H
#define XMLPARSERLOAD_H

#include <QFile>
#include <QXmlStreamReader>
#include <complex>


#include "Modelo/Herramientas/tools.h"
#include "Modelo/EstructurasDatos/var.h"
#include "Modelo/EstructuraSistema/sistema.h"
#include "Modelo/EstructuraSistema/cpolinomios.h"
#include "Modelo/EstructuraSistema/formatolibre.h"
#include "Modelo/EstructuraSistema//kganancia.h"
#include "Modelo/EstructuraSistema/knganancia.h"
#include "Modelo/Objetos/omega.h"
#include "Modelo/controlador.h"
#include "Modelo/Herramientas/tools.h"
#include "Modelo/EstructurasDatos/datosloopshaping.h"




/**
 * @class XmlParserLoad
 * @brief Clase que carga los datos guardados en un fichero y los introduce en el sistema.
 *
 * @author Isaac Martínez Forte
 */

class XmlParserLoad
{
public:


/**
 * @fn XmlParserLoad
 * @brief Constructor de la clase.
 */

    XmlParserLoad();

/**
 * @fn recuperarXmlDatos
 * @brief Función que guarda los datos necesarios para la recuperación.
 *
 * @param fichero Ruta al fichero donde están guardados los datos.
 */

    QVector <bool> * recuperarXmlDatos (QString fichero);


    /**
      * @fn getPlanta
      * @brief Función que recupera los datos de la planta desde fichero.
      *
      * @return Planta recuperada.
      */

    std::shared_ptr<Sistema> getPlanta();


    QVector <tools::dBND *> * getEspecificaciones();

    /**
      * @fn getOmega
      * @brief Función que recupera los datos de las frecuencias de diseño (Omega).
      *
      * @return Omega frecuencias de diseño recuperadas.
      */

    Omega * getOmega();


    /**
      * @fn getTemplates
      * @brief Función que recupera los datos de los templates guardados en el fichero.
      *
      * @return templates recuperados.
      */

    QVector <QVector <std::complex<qreal>> * > * getTemplates();


    /**
      * @fn getContorno
      * @brief Función que recupera los datos del contorno de los templates guardados en el sistema.
      *
      * @return Contorno recuperado.
      */

    QVector <QVector <std::complex<qreal>> * > * getContorno();

    QVector <qreal> * getEpsilon ();


    /**
      * @fn getBoundaries
      * @brief Función que recupera los datos de los boundaries guardados en el sistema.
      *
      * @return boundaries recuperados.
      */

    std::shared_ptr<DatosBound> getBoundaries();


    /**
      * @fn getSabana
      * @brief Función que recupera los datos de las sábanas guardados en el fichero.
      *
      * @return sabana recuperada.
      */

    QVector <QVector <QVector <qreal> * > *> * getSabana();

    std::shared_ptr<Sistema> getControlador();

    std::shared_ptr<DatosLoopShaping> getLoopShaping();

private:
     QXmlStreamReader * stream;

     inline bool salidaError();

     inline Var * leerVariable(qint32 tipoLectura);

     inline bool leerPlanta(qint32 tipoLectura);
     inline bool leerEspecificaciones();
     inline bool leerOmega();
     inline bool leerTemplates();
     inline bool leerBoundaries();
     inline bool leerLoopShaping();

     inline bool asignarLectura(QString cont);
     inline void leerNombreSeccion();
     inline bool leerVectorComplejos(QVector <std::complex<qreal>> * vector);

     inline bool leerFrecuencias(QMap<QString, QVector<QVector<QPointF> *> *> *mapa);
     inline bool leerEspec(QMap<QString, QVector<QVector<QPointF> *> *> *mapa);

     inline bool leerVectorVectorPunto(QVector <QVector <QPointF> * > * vec);
     inline QVector<bool> *strtobool(QString cadena);

     inline QVector<bool> *leerVectorBool();

     bool isPlanta;
     bool isEspecificaciones;
     bool isOmega;
     bool isTemplates;
     bool isContorno;
     bool isBoundaries;
     bool isControlador;
     bool isLoopShaping;


     std::shared_ptr<Sistema> planta;

     QVector <tools::dBND *> * especificaciones;

     Omega * omega;

     QVector <QVector <std::complex<qreal>> * > * templates;
     QVector <QVector <std::complex<qreal>> * > * contorno;
     QVector <qreal> * epsilon;

     std::shared_ptr<DatosBound> bound;
     QVector<QVector<QVector<qreal> *> *> * sabanas;

     std::shared_ptr<Sistema> controlador;

     std::shared_ptr<DatosLoopShaping> loopShaping;

};

#endif // XMLPARSERLOAD_H
