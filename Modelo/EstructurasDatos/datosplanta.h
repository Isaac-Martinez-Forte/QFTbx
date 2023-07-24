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

    DatosPlanta(std::shared_ptr<PlantaDAO> planta, std::shared_ptr<EspecificacionesDAO> espec, std::shared_ptr<OmegaDAO> omegas,
                std::shared_ptr<BoundDAO> boundaries, std::shared_ptr<TemplateDAO> templates);


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

    void setPlanta (std::shared_ptr<PlantaDAO> planta);


    void setEspecificaciones (std::shared_ptr<EspecificacionesDAO> espec);

    /**
     * @fn setPlanta
     * @brief Función que guarda las frecuencias de diseño en la clase.
     *
     * @param omegas adaptadorDAO que corresponde a las frecuencias de diseño.
     */

    void setOmega (std::shared_ptr<OmegaDAO> omegas);
    
    
    /**
     * @fn setBoundaries
     * @brief Función que guarda las los boundaries calculados en la clase.
     *
     * @param omegas adaptadorDAO que corresponde a los boundaries.
     */
    
    void setBoundaries (std::shared_ptr<BoundDAO> boundaries);
    
    
    /**
     * @fn setTemplates
     * @brief Función que guarda los templates calculados en la clase.
     *
     * @param omegas adaptadorDAO que corresponde a los templates.
     */
    
    void setTemplates (std::shared_ptr<TemplateDAO> templates);
    
    
    /**
     * @fn getPlanta
     * @brief Función que retorna la planta guardada en la clase.
     *
     * @return adaptadorDAO que corresponde a la planta.
     */

    std::shared_ptr<PlantaDAO>  getPlanta();
    

    std::shared_ptr<EspecificacionesDAO> getEspecificaciones ();
    
    /**
     * @fn getOmega
     * @brief Función que retorna las frecuencias de diseño guardadas en la clase.
     *
     * @return adaptadorDAO que corresponde a las frecuencias de diseño.
     */
    
    std::shared_ptr<OmegaDAO> getOmega();
    
    
    /**
     * @fn getBoundaries
     * @brief Función que retorna los boundaries guardados en la planta.
     *
     * @return adaptadorDAO que corresponde a los boundaries.
     */
    
    std::shared_ptr<BoundDAO> getBoundaries();
    
    
    /**
     * @fn getTemplates
     * @brief Función que retorna los templates guardados en la planta.
     *
     * @return adaptadorDAO que corresponde a los templates.
     */
    
    std::shared_ptr<TemplateDAO> getTemplates();

    
     /**
     * @fn getExisten
     * @brief Función que retorna un vector de booleanos indicando que datos han sido introducidos en la clase.
     *
     * @return vector de booleanos que indica que datos han sido introducidos en la clase.
     */
    
    QVector <bool> * getExisten();

    void setControlador (std::shared_ptr<ControladorDAO> contro);
    std::shared_ptr<ControladorDAO> getControlador();

    void setLoopShaping(std::shared_ptr<LoopShapingDAO> loopShaping);
    std::shared_ptr<LoopShapingDAO> getLoopShaping();


private:

    std::shared_ptr<BoundDAO> boundaries;
    std::shared_ptr<OmegaDAO> omegas;
    std::shared_ptr<PlantaDAO> planta;
    std::shared_ptr<TemplateDAO> templates;
    std::shared_ptr<EspecificacionesDAO> espec;
    std::shared_ptr<ControladorDAO> contro;
    std::shared_ptr<LoopShapingDAO> loopShaping;


    QVector <bool> * existen;

};

#endif // DATOSPLANTA_H
