#ifndef PARLABEL_H
#define PARLABEL_H

#include <QVector>
#include <QLineEdit>


  /**
    * @class ParLineEdit
    * @brief Clase que sirve para guardar tres QLineEdit de forma temporal para ser utilizados posteriormente.
    * 
    * @author Isaac Martínez Forte
   */

class ParLineEdit
{
public:
  
  /**
    * @fn ParLineEdit
    * @brief Constructor por defecto sin parámetros.
    * 
   */
    
  /**
    * @fn ParLineEdit
    * @brief Constructor de la clase con los tres parámetros básicos que guarda la clase.
    * 
    * @param x QLineEdit, es el primer objeto a guardar.
    * @param y QLineEdit, es el segundo objeto a guardar.
    * @param nominal QLineEdit, es el tercero objeto a guardar.
    * 
   */
    
    //Default constructible so a row can be held by value, and empty until
    //its widgets exist.
    ParLineEdit() = default;

    ParLineEdit(QLineEdit * x, QLineEdit * y, QLineEdit * nominal);
    
    
    /**
     * @fn ~ParLineEdit
     * @brief Destructor de la clase.
     */
    
   /**
    * @fn setX
    * @brief Función que guarda X en el sistema.
    * 
    * Si hubiera ya otro objeto guardardo anteriormente este sería borrado para que no haya fugas de memoria.
    * 
    * @param X a guardar en el sistema.
    * 
   */

    void setX (QLineEdit * label);
    
   /**
    * @fn getX
    * @brief Función que devuelve el objeto X guardado en el sistema.
    * 
    * @return Objeto X guardado en el sistema.
    * 
   */
    
    QLineEdit *getX() const;
    
   /**
    * @fn setY
    * @brief Función que guarda y en el sistema.
    * 
    * Si hubiera ya otro objeto guardardo anteriormente este sería borrado para que no haya fugas de memoria.
    * 
    * @param Y a guardar en el sistema.
    * 
   */
    
    void setY (QLineEdit * label);
    
   /**
    * @fn getY
    * @brief Función que devuelve el objeto y guardado en el sistema.
    * 
    * @return Objeto Y guardado en el sistema.
    * 
   */
    
    QLineEdit *getY() const;
    
   /**
    * @fn setNominal
    * @brief Función que guarda el nominal en el sistema.
    * 
    * Si hubiera ya otro objeto guardardo anteriormente este sería borrado para que no haya fugas de memoria.
    * 
    * @param nominal a guardar en el sistema.
    * 
   */
    
    void setNominal (QLineEdit *  nominal);
    
   /**
    * @fn nominal
    * @brief Función que devuelve el objeto y guardado en el sistema.
    * 
    * @return Objeto nominal guardado en el sistema.
    * 
   */
    
    QLineEdit * nominal() const;

private:
    QLineEdit *x = nullptr;
    QLineEdit *y = nullptr;
    QLineEdit * m_nominal = nullptr;
};

#endif // PARLABEL_H
