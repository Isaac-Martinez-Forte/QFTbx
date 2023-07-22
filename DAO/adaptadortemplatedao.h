#ifndef ADAPTADORTEMPLATEDAO_H
#define ADAPTADORTEMPLATEDAO_H

#include <complex>

#include "DAO/templatedao.h"

/**
 * @class AdaptadorTemplateDAO
 * @brief Clase que implementa el patrón DAO para guardar en memoria un template y su contorno.
 * @author Isaac Martínez Forte
 */

class AdaptadorTemplateDAO : public TemplateDAO
{
public:
  
  /**
    * @fn AdaptadorTemplateDAO
    * @brief Constructor de la clase.
   */
  
    AdaptadorTemplateDAO();
    ~AdaptadorTemplateDAO();
    
    /**
    * @fn setTemplates
    * @brief Guarda un template, si ya hubiera uno guardado lo borra para evitar fugas de memoria.
    * 
    * @param templates a guardar en el sistema.
   */

    void setTemplates(QVector<QVector<std::complex<qreal> > *> *templates);
    
    /**
    * @fn getTemplates
    * @brief Retorna el template guardado en la clase, si no hay ninguno devuelve un puntero a NULL.
    * @return templates guardados en la clase.
   */
    
    QVector <QVector <std::complex <qreal> > * > * getTemplates();

    /**
    * @fn setContorno
    * @brief Guarda un contorno del template, si ya hubiera uno guardado lo borra para evitar fugas de memoria.
    * @param contorno a guardar en el sistema.
   */
    
    void setContorno(QVector<QVector<std::complex<qreal> > *> *contorno);
    
    /**
    * @fn getContorno
    * @brief Retorna el contorno del template guardado en la clase, si no hay ninguno devuelve un puntero a NULL.
    * @return contorno guardado en la clase.
   */
    
    QVector <QVector <std::complex <qreal> > * > * getContorno();

    /**
    * @fn isContorno
    * @brief Función que retorna un booleano indicando si hay un contorno guardado en el sistema.
    *  Esta función existe dado que puede haber templates guardados en el sistema pero sin contorno.
    *
    * @return Booleano true si hay contorno guardado y false en clase contrario.
   */

    bool isContorno();

    void setEpsilon (QVector <qreal> * epsilon);

    QVector <qreal> * getEpsilon ();

private:
    QVector <QVector <std::complex <qreal> > * > * templates = NULL;
    QVector <QVector <std::complex <qreal> > * > * contorno = NULL;
    QVector <qreal> * epsilon = NULL;

    bool contornoExists;
};

#endif // ADAPTADORTEMPLATEDAO_H
