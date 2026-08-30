#ifndef OMEGA_H
#define OMEGA_H

#include <QString>
#include <QVector>


   /**
    * @class Omega
    * @brief Clase que representa las frecuencias de diseño.
    *
    * @author Isaac Martínez Forte
    */

class Omega
{
public:

   /**
    * @enum tiposOmega
    * @brief Cómo se han generado las frecuencias de diseño. Los valores
    * numéricos se serializan en los .qft: no reordenar.
    */
    enum tiposOmega {linSpace, logSpace, manual, fichero};

  /**
    * @fn Omega
    * @brief Constructor de la clase.
    *
    * @param inicio primer valor de las frecuencias de diseño
    * @param final último valor de las frecuencias de diseño.
    * @param nPuntos número de valores que tienen las frecuencias de diseño.
    * @param valores de las frecuencias de diseño.
    * @param tiposOmega tipo con el cual se han generado las frecuencias de diseño.
    */

    Omega(qreal inicio, qreal final, qint32 nPuntos, QVector <qreal> * valores, tiposOmega tipo);

   /**
    * @fn valuesFromFile
    * @brief Lee un fichero de frecuencias (valores separados por espacios o
    * saltos de línea). Lanza qftbx::FileError si no puede abrirse o no
    * contiene frecuencias válidas.
    */
    static QVector <qreal> * valuesFromFile(QString path);
    
    
    /**
     * @fn ~Omega
     * @brief Destructor de la clase.
     */
    
    ~Omega();
    
    
   /**
    * @fn getInicio
    * @brief Función que retorna el valor de inicio de las frecuencias de diseño.
    * 
    * @return valor inicial de las frecuencias de diseño.
    */

    qreal getInicio();
    
    
   /**
    * @fn getFinal
    * @brief Función que retorna el valor final de las frecuencias de diseño.
    * 
    * @return valor final de las frecuencias de diseño.
    */
    
    qreal getFinal();
    
    
   /**
    * @fn getNPuntos
    * @brief Función que retorna en número de valores de las frecuencias de diseño.
    * 
    * @return número de valores de las frecuencias de diseño.
    */
    
    qint32 getNPuntos();
    
    
   /**
    * @fn getValores
    * @brief Función que retorna los valores de las frecuencias de diseño.
    * 
    * @return vector con las frecuencias de diseño.
    */
    
    QVector <qreal> * getValores();
    
    
   /**
    * @fn getTipo
    * @brief Función que retorna el tipo con el cual se ha generado las frecuencias de diseño.
    * 
    * @return tipo de las frecuencias de diseño.
    */
    
    tiposOmega getTipo();

    void setOmega (QVector <qreal> * nueva_omega);


private:
    qreal inicio;
    qreal final;
    qint32 nPuntos;
    QVector <qreal> * valores;
    tiposOmega tipo;
};

#endif // OMEGA_H
