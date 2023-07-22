#ifndef DATOSPLANTA_H
#define DATOSPLANTA_H

#include "DAO/bounddao.h"
#include "DAO/omegadao.h"
#include "DAO/plantadao.h"
#include "DAO/templatedao.h"
#include "DAO/loopshapingdao.h"
#include "DAO/especificacionesdao.h"
#include "DAO/adaptadorcontroladordao.h"
#include "DAO/adaptadorloopshapingdao.h"


/**
   * @class DatosPlanta
   * @brief Clase que representa todos los datos calculados sobre una planta.
   *
   * Esta clase sirve para encapsular los adaptadores DAO para poder enviarlos a otra clase como un solo objeto.
   * @author Isaac Martínez Forte
  */

class DatosPlanta
{
public:


    /**
     * @fn DatosPlanta
     * @brief Constructor por defecto que crea el objeto.
     */

    DatosPlanta();


    /**
     * @fn DatosPlanta
     * @brief Constructor que crea el objeto.
     *
     * @param planta Sistema que se quiere controlar.
     * @param omegas Frecuencias de diseño.
     * @param boundaries Fronteras calculadas para la planta con las frecuencias de diseño introducidas.
     * @param templates Plantillas calculadas para la planta con las frecuencias de diseño introducidas.
     */

    DatosPlanta(PlantaDAO * planta, EspecificacionesDAO * espec, OmegaDAO * omegas, BoundDAO * boundaries, TemplateDAO * templates);


    /**
     * @fn ~DatosPlanta
     * @brief Destructor del objeto.
     */

    ~DatosPlanta();


    /**
     * @fn setPlanta
     * @brief Función que guarda una planta en la clase.
     *
     * @param planta adaptadorDAO que corresponde a la planta.
     */

    void setPlanta (PlantaDAO * planta);


    void setEspecificaciones (EspecificacionesDAO * espec);

    /**
     * @fn setPlanta
     * @brief Función que guarda las frecuencias de diseño en la clase.
     *
     * @param omegas adaptadorDAO que corresponde a las frecuencias de diseño.
     */

    void setOmega (OmegaDAO * omegas);
    
    
    /**
     * @fn setBoundaries
     * @brief Función que guarda las los boundaries calculados en la clase.
     *
     * @param omegas adaptadorDAO que corresponde a los boundaries.
     */
    
    void setBoundaries (BoundDAO * boundaries);
    
    
    /**
     * @fn setTemplates
     * @brief Función que guarda los templates calculados en la clase.
     *
     * @param omegas adaptadorDAO que corresponde a los templates.
     */
    
    void setTemplates (TemplateDAO * templates);
    
    
    /**
     * @fn getPlanta
     * @brief Función que retorna la planta guardada en la clase.
     *
     * @return adaptadorDAO que corresponde a la planta.
     */

    PlantaDAO *  getPlanta();
    

    EspecificacionesDAO * getEspecificaciones ();
    
    /**
     * @fn getOmega
     * @brief Función que retorna las frecuencias de diseño guardadas en la clase.
     *
     * @return adaptadorDAO que corresponde a las frecuencias de diseño.
     */
    
    OmegaDAO * getOmega();
    
    
    /**
     * @fn getBoundaries
     * @brief Función que retorna los boundaries guardados en la planta.
     *
     * @return adaptadorDAO que corresponde a los boundaries.
     */
    
    BoundDAO * getBoundaries();
    
    
    /**
     * @fn getTemplates
     * @brief Función que retorna los templates guardados en la planta.
     *
     * @return adaptadorDAO que corresponde a los templates.
     */
    
    TemplateDAO * getTemplates();

    
     /**
     * @fn getExisten
     * @brief Función que retorna un vector de booleanos indicando que datos han sido introducidos en la clase.
     *
     * @return vector de booleanos que indica que datos han sido introducidos en la clase.
     */
    
    QVector <bool> * getExisten();

    void setControlador (ControladorDAO * contro);
    ControladorDAO * getControlador();

    void setLoopShaping(LoopShapingDAO * loopShaping);
    LoopShapingDAO * getLoopShaping();


private:

    BoundDAO * boundaries;
    OmegaDAO * omegas;
    PlantaDAO * planta;
    TemplateDAO * templates;
    EspecificacionesDAO * espec;
    ControladorDAO * contro;
    LoopShapingDAO * loopShaping;


    QVector <bool> * existen;

};

#endif // DATOSPLANTA_H
