#ifndef INTINCERTIDUMBRE_H
#define INTINCERTIDUMBRE_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QStringBuilder>
#include <QVector>
#include <list>
#include <QVBoxLayout>

#include "Modelo/EstructurasDatos/var.h"
#include "Modelo/EstructurasDatos/parlineedit.h"
#include "muparserx/mpParser.h"
#include "Modelo/Herramientas/tools.h"

namespace Ui {
class IntIncertidumbre;
}

 /**
    * @class IntIncertidumbre
    * @brief Clase gráfica a través de la cual se puede introducir los distintos tipos de incertidumbre de la planta.
    * 
    * Dicha incertidumbre introducida después se guardará en una jerarquía de clases diferenciando los distintos tipos.
    * 
    * @author Isaac Martínez Forte
   */


class IntIncertidumbre : public QDialog
{
    Q_OBJECT
    
public:
  
   /**
    * @fn IntIncertidumbre
    * @brief Constructor de la clase, al ser una clase que hereda de QDialog tiene parámetros especiales.
    * 
    * @param parent Objeto que se envía por parámetros al constructor padre indicando cual es el padre de la clase.
    */
  
    explicit IntIncertidumbre(QWidget *parent = 0);
    ~IntIncertidumbre();

    
   /**
    * @fn getNumerador
    * @brief Función que retorna la incertidumbre del numerador que ha sido introducida por el usuario.
    * 
    * @return un QVector de Variables que contiene la incertidumbre introducida para cada variable del numerador.
   */
    
    QVector <Var*> *  getNumerador();
    
    
   /**
    * @fn getDenominador
    * @brief Función que retorna la incertidumbre del denominador que ha sido introducida por el usuario.
    * 
    * @return un QVector de Variables que contiene la incertidumbre introducida para cada variable del denominador.
   */
    
    QVector <Var*> * getDenominador();

    
   /**
    * @fn getK
    * @brief Función que retorna la incertidumbre de la variable K que representa la ganancia.
    * 
    * @return un objeto tipo QPointF que el par de valores que representa la incertidumbre de K.
   */
    
    QPointF getK();
    
    
   /**
    * @fn getRet
    * @brief Función que retorna la incertidumbre de la variable Ret que representa el retardo de la planta.
    * 
    * @return un objeto tipo QPointF que el par de valores que representa la incertidumbre de Ret.
   */
    
    QPointF getRet();
    
    
   /**
    * @fn lanzarViewIncer
    * @brief Función que pone en ejecución toda la funcionalidad de la clase gráfica.
    * 
    * @param numerador de la planta en forma de QString.
    * @param denominador de la planta en forma de QString.
    * @param k ganancia de la planta en forma de QString.
    * @param ret retardo de la planta en forma de QString.
    * 
    * @return booleano que indica si ha funcionado correctamente todo.
   */

    bool lanzarViewIncer(QVector<QVector<QString> *> *tabla, QVector<QVector<QString> *> *exp,
                         QVector <QVector <bool> * > * isVar, bool rango);


    /**
     * @fn lanzarViewIncer
     * @brief Función que pone en ejecución toda la funcionalidad de la clase gráfica.
     *
     * @param numerador de la planta en forma de QString.
     * @param denominador de la planta en forma de QString.
     * @param k ganancia de la planta en forma de QString.
     *
     * @return booleano que indica si ha funcionado correctamente todo.
    */

    //bool lanzarViewIncer(QVector<QString> *numerador, QVector<QString> *denominador, QString k);



    
private slots:
    void on_numerador_clicked();

    void on_denominador_clicked();

    void on_aceptar_clicked();

signals:
    void close_ok ();

private:

    bool rango;
    QVector <QString> * numeradorNombre = NULL;
    QVector <QString> * denominadorNombre = NULL;
    QVector <Var*> * numerador;
    QVector <Var*> * denominador;
    std::list <ParLineEdit*>* parNume;
    std::list <ParLineEdit*>* parDeno;
    QVBoxLayout *layoutdeno;
    QVBoxLayout *layoutnume;

    QVector <QWidget *> * cajas;


    void formarango();

    bool guardarrango();
    void formarLinea(QWidget *widget, QString numero, std::list <ParLineEdit*> * vector, bool rango);
   // void formarango (QVector<QString> *numerador, QVector<QString> *denominador);
    qreal parse(QString cadena);

    Ui::IntIncertidumbre *ui;

    qreal k;
    qreal ret;

    qreal resultado;
    mup::ParserX p;

    QVector<QVector<QString> *> *tabla;
    QVector<QVector<QString> *> *exp;
    QVector <QVector <bool> * > * isVar;

    bool controlador;
};

#endif // INTINCERTIDUMBRE_H
