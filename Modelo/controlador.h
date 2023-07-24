#ifndef CONTROLADOR_H
#define CONTROLADOR_H


#include "DAO/dao.h"
#include "DAO/fdao.h"
#include "DAO/plantadao.h"
#include "DAO/omegadao.h"
#include "DAO/templatedao.h"
#include "DAO/bounddao.h"
#include "DAO/especificacionesdao.h"
#include "DAO/adaptadorcontroladordao.h"

#include <QHash>
#include <complex>

#include "EstructuraSistema/sistema.h"
#include "Templates/templates.h"
#include "Objetos/omega.h"
#include "Boundaries/boundaries.h"
#include "XmlParser/parsersave.h"
#include "XmlParser/parserload.h"
#include "EstructurasDatos/datosplanta.h"
#include "Herramientas/tools.h"
#include "LoopShaping/loopshaping.h"

#include <iostream>


/**
    * @class Controlador
    * @brief Clase que controla el sistema, y gestiona los objetos del programa.
    *
    * Al usar el patrón Modelo-Vista-Controlador, la interfaz está separada de la lógica del programa, con lo cual el punto de comunicación es el Controlador.
    * @author Isaac Martínez Forte
    */

class Controlador
{
public:


    /**
    * @fn Controlador
    * @brief Constructor de la clase controlador.
    */

    Controlador();
    
    
    /**
     * @fn ~Controlador
     * @brief Destructor de la clase.
     */
    
    ~Controlador();
    
    
    
    /**
    * @fn getPlanta
    * @brief Función que retorna una Planta guardada en el sistema.
    *
    * Esta función accede directamente al adaptador Dao usado por el sistema para recuperar la Planta que se solicita.
    * @return planta guardada en el sistema.
    */
    std::shared_ptr<Sistema> getPlanta();


    Omega * getOmega();


    /**
     * @fn getOmega
     * @brief Función que retorna el vector de frecuencias de diseño guardadas en el sistema.
     *
     * Esta función accede directamente al adaptador Dao usado por el sistema para recuperar el vector de frecuencias de diseño.
     * @return QVector de qreal con las frecuencias de diseño.
     */

    QVector <qreal> * getFrecuencias();


    QVector<tools::dBND *> *getEspecificaciones();


    /**
     * @fn setPlanta
     * @brief Función que guarda una Planta en el sistema.
     *
     * La Planta se guardará a través el adaptador DAO definido en el sistema.
     * @param planta Sistema a guardar en el sistema.
     */
    
    void setPlanta (std::shared_ptr<Sistema> planta);
    
    
    /**
     * @fn setOmega
     * @brief Función que guarda un vector de frecuencias de diseño en el sistema.
     *
     * El vector de Omega de guardará a través del adaptador DAO definido en el sistema.
     * @Param frecuencias vector de reales que contiene las frecuencias Omega.
     */
    
    void setOmega (Omega * omega);
    
    
    /**
     * @fn setTemplate
     * @brief Función que guarda los templates calculados en el sistema.
     *
     * Los templates se guardarán a través del adaptador DAO definido en el sistema.
     * @Param templates plantillas calculadas.
     */


    void setEspecificaciones (QVector <tools::dBND * > * espe);

    void setTemplate (QVector <QVector <std::complex <qreal> > * > * temp, QVector <QVector <std::complex <qreal> > * > * contorno, bool isContorno);

    void setContorno(QVector<QVector<std::complex<qreal> > *> *contorno);
    
    /**
     * @fn setBoundaries
     * @brief Función que guarda los boundaries calculados en el sistema.
     *
     * Los boundaries se guardarán a través del adaptador DAO definido en el sistema.
     * @Param boundaries fronteras calculadas.
     */
    
    void setBoundaries (std::shared_ptr<DatosBound> dbound);


    /**
    * @fn calcularTemplates
    * @brief Función que calcula los templates para la planta guardada en el sistema.
    *
    * Esta función también calcula el contorno de dichos templates.
    *
    * @param epsilon real necesitado por el algoritmo e_hull para calcular el contorno.
    * @param mapa QHash donde se relacionan las variables de la planta con con los reales concretos de esa variabilidad.
   */

    bool calcularTemplates(QVector<qreal> *epsilon, QHash<Var *, QVector<qreal> *> *mapa, bool cuda);
    
    
    /**
     * @fn calcularBoundaries
     * @brief Función que calcula los boundaries.
     *
     * Actualmente se calculan los boundaries de estabilidad.
     *
     * @param altura número real que se utiliza para "cortar" a la altura determinada la sábana generada por el algoritmo.
     * @param datosFas números de inicio y de fin del eje de coordenadas de las fases.
     * @param puntosFas número de puntos que tiene el eje de coordenadas correspondiente a las fases.
     * @param datosMag números de inicio y de fin del eje de coordenadas de las magnitudes.
     * @param puntosMag número de puntos que tiene el eje de coordenadas correspondiente a las magnitudes.
     * @param infinito número real que representa el infinito, si es 0 se toma como vacío.
     * @param contorno booleano que indica si se ha calculado el contorno de los templates.
     *
     * @return booleano que indica si se han calculado correctamente los boundaries.
     */

    QVector <QVector <std::complex <qreal> > * > * recalcularContorno(QVector<qreal> *epsilon);


    /**
     * @fn getTemplate
     * @brief Función que retorna los templates calculados para la planta
     *
     * Esta función accede directamente al adaptador Dao usado por el sistema para recuperar los templates solicitados
     * @return templates guardados de la planta.
     */

    QVector <QVector <std::complex <qreal> > * > * getTemplate();


    /**
    * @fn getContorno
    * @brief Función que retorna el contorno de los templates calculados para la planta.
    *
    * Esta función accede directamente al adaptador Dao, el mismo que se usa para los templates dado que son dados que van asociados.
    * @return contorno de los templates guardados de la planta.
   */

    QVector <QVector <std::complex <qreal> > * > * getContorno();


    bool calcularBoundaries(QPointF datosFas, qint32 puntosFas, QPointF datosMag, qint32 puntosMag, qreal infinito, bool contorno, bool cuda);

    
    /**
    * @fn getBound
    * @brief Función que retorna los boundaries calculados en el sistema.
    *
    * @return Boundaries calculados en el sistema.
    */
    
    std::shared_ptr<DatosBound> getBound();

    QVector<QVector<QPointF> *> *getBoundariesReun();
    QVector<QVector<QVector<QPointF> *> *> *getBoundariesReunHash();


    bool setControlador (std::shared_ptr<Sistema> controlador);

    std::shared_ptr<Sistema> getControlador();
    

    bool calcularLoopShaping(qreal epsilon, alg_loop_shaping seleccionado, QPointF rango, qreal nPuntos, bool depuracion,
                             qreal delta, qint32 inicializacion, bool hilos, bool bisection_avanced, bool deteccion_avanced,
                             bool a);

    std::shared_ptr<DatosLoopShaping> getLoopShaping ();
    
    /**
    * @fn guardarSistema
    * @brief Función que guarda los datos calculados en el sistema en un fichero.
    *
    * @param fichero ruta al fichero donde guardar los datos.
    *
    * @return booleano indicando si la función a terminado correctamente.
    */

    bool guardarSistema(QString fichero);


    /**
    * @fn cargarSistema
    * @brief Función que lee los datos guardados en un fichero.
    *
    * @param fichero ruta al fichero donde leer los datos.
    *
    * @return vector de booleanos que indica que datos se han leído correctamente.
    */

    QVector<bool> * cargarSistema(QString fichero);
    
    
    /**
    * @fn nivel
    * @brief Función que retorna un entero indicando las fases de QFT que han sido completadas
    */

    qint32 nivel();

    QVector<qreal> *getEpsilon();

    void setLoopShaping (std::shared_ptr<DatosLoopShaping> datos);

    
private:
    bool paso1; //Planta
    bool paso2; //Especificaciones
    bool paso3; //Omega
    bool paso4; //Templates
    bool paso5; //Boundaries
    bool paso6; //Introducir controlador
    bool paso7; //Loop Shaping

    Boundaries * bound;
    Templates * templates;
    LoopShaping * loopShaping;

    DAO * dao;
    PlantaDAO *  plantadao;
    OmegaDAO * omegadao;
    TemplateDAO * templatedao;
    BoundDAO * bounddao;
    EspecificacionesDAO * especdao;
    ControladorDAO * controladordao;
    LoopShapingDAO * loopshapingdao;

};

#endif // CONTROLADOR_H
