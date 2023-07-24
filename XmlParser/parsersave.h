#ifndef XMLPARSERSAVE_H
#define XMLPARSERSAVE_H

#include <QFile>
#include <QXmlStreamWriter>
#include "Modelo/Herramientas/tools.h"
#include "Modelo/EstructuraSistema/sistema.h"
#include "Modelo/EstructurasDatos/var.h"
#include "Modelo/EstructurasDatos/datosplanta.h"
#include "DAO/omegadao.h"
#include "DAO/templatedao.h"
#include "DAO/bounddao.h"
#include "complex"
#include "DAO/especificacionesdao.h"
#include "Modelo/Herramientas/tools.h"


/**
 * @class XmlParserSave
 * @brief Clase que guarda los datos del sistema en un fichero.
 *
 * @author Isaac Martínez Forte
 */

class XmlParserSave
{
public:


   /**
    * @fn XmlParserSave
    * @brief Constructor de la clase.
    */

    XmlParserSave();


   /**
    * @fn XmlParserSave
    * @brief Destructor de la clase.
    */

    ~XmlParserSave();

   /**
    * @fn guardarXMLDatos
    * @brief Función que inicia del guardado de los datos del sistema en un fichero.
    *
    * @param fichero Ruta al fichero donde se quieren guardar los datos.
    * @param datosPlanta Datos a guardar en un fichero.
    *
    * @return booleano indicando si todo a funcionado correctamente.
    */

    bool guardarXMLDatos(QString fichero, std::shared_ptr<DatosPlanta> datosPlanta);



private:

    inline bool guardarPlanta(std::shared_ptr<Sistema> planta, QString nombre);

    inline void guardarVariable (Var * var, QString idem);

    inline void guardarOmega(std::shared_ptr<OmegaDAO> omega);

    inline void guardarEspecificaciones (std::shared_ptr<EspecificacionesDAO> especificaciones);

    inline void guardarTemplates (std::shared_ptr<TemplateDAO> templates, bool isContorno);

    inline void guardarBoundaries (std::shared_ptr<BoundDAO> boundaries);

    inline void guardarLoopShaping (std::shared_ptr<LoopShapingDAO> loopShaping);

    inline QString vectorTString (QVector<qreal> *vector);

    QXmlStreamWriter * stream;

    bool guardadoDatos;

    inline void guardarVector (QVector <QVector <qreal> *>* vectores);

    inline void guardarPunto (QVector <QVector <QPointF> *>* vectores);

    inline void guardarVectorComplejo (QVector <QVector <std::complex<qreal> > *>* vectores);

    inline void guardarVectores (QVector <QVector <QVector <qreal> *>*> * vectores);

    inline void guardarPuntos (QVector <QVector <QVector <QPointF> *>*> * puntos);

    inline void guardarEstructuraEspec (tools::dBND * estructura);

    inline void guardarEstructuraBound (QString nombre, QVector <QVector <QPointF> * > * datos);

    inline void guardarVectorBool (QVector <bool> * vec, QString nombre);

};

#endif // XMLPARSERSAVE_H
