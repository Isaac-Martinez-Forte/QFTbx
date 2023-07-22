#ifndef TEMPLATEDAO_H
#define TEMPLATEDAO_H

#include <QVector>
#include <complex>


/**
  * @class TemplateDAO
  * @brief Clase que implementa el patrón DAO para guardar los templates en el sistema.
  *
  * Esta clase es abstracta dado que contiene funciones virtuales puras, para que sea redefinidas por sus hijos y así implementar adapatadores del patrón DAO.
  *
  * @author Isaac Martínez Forte
 */

class TemplateDAO
{
public:


    /**
      * @fn ~PlantaDAO
      * @brief Destructor de la clase, solo puede ser llamado desde un hijo.
      *
      */

    virtual ~TemplateDAO() {}


    /**
      * @fn setTemplates
      * @brief Clase virtual pura que guarda los templates completos en el sistema.
      *
      * @param templates a guardar en el sistema.
      */

    virtual void setTemplates(QVector <QVector <std::complex <qreal> > * > * templates) = 0;


    /**
      * @fn getTemplates
      * @brief Clase virtual pura que retorna los templates completos guardados en el sistema.
      *
      * @return templates guardados en el sistema.
      */

    virtual QVector <QVector <std::complex <qreal> > * > * getTemplates () = 0;


    /**
      * @fn setContorno
      * @brief Clase virtual pura que guarda el contorno de los templates en el sistema.
      *
      * @param contorno a guardar en el sistema.
      */

    virtual void setContorno (QVector <QVector <std::complex <qreal> > * > * contorno) = 0;


    /**
      * @fn getContorno
      * @brief Clase virtual pura que retorna el contorno de los templates guardados en el sistema.
      *
      * @return contorno guardado en el sistema.
      */

    virtual QVector <QVector <std::complex <qreal> > * > * getContorno () = 0;


    /**
      * @fn isContorno
      * @brief Clase virtual pura que retorna un booleano indicando si hay contornos guardados en el sistema.
      *
      * @return booleano indicando si hay un contorno guardado en el sistema.
      */

    virtual bool isContorno() = 0;

    virtual void setEpsilon (QVector <qreal> * epsilon) = 0;

    virtual QVector <qreal>* getEpsilon () = 0;
};

#endif // TEMPLATEDAO_H
