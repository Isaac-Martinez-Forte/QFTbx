#ifndef QFTBX_FREE_FORM_H
#define QFTBX_FREE_FORM_H

#include <QVector>

#include "transfer_function.h"
#include "complex"
#include "mpParser.h"

 /**
    * @class FreeForm
    * @brief Función de transferencia definida por expresiones de texto libre en 's'.
    *
    * Los vectores de variables no describen la estructura (numerador y
    * denominador son texto): solo enumeran las variables inciertas presentes
    * en las expresiones, para el barrido de templates.
    *
    * @author Isaac Martínez Forte
   */

class FreeForm : public TransferFunction
{
public:

   /**
    * @fn FreeForm
    * @brief Constructor de la clase.
    *
    * @param nombre de la planta.
    * @param numerador variables inciertas presentes en exp_nume.
    * @param denominador variables inciertas presentes en exp_deno.
    * @param k ganancia asociada.
    * @param ret retardo asociado.
    * @param exp_nume expresión de texto del numerador.
    * @param exp_deno expresión de texto del denominador.
    */
    FreeForm(QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador, Parameter * k, Parameter* ret, QString exp_nume,
                 QString exp_deno);

    QString expression (QVector <qreal> * numerador, QVector <qreal> * denominador,
                             qreal k, qreal ret, qreal omega);

    QString expression(qreal w);

    QString expression();

    std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega);

    std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega);

    std::complex <qreal> evaluate (QVector <qreal> * numerador, QVector <qreal> * denominador,
                                           qreal k, qreal ret, qreal omega);

    //Se reexpone la resolución nominal heredada, oculta por las sobrecargas anteriores.
    using TransferFunction::evaluate;

    SystemType type();

    LtiSystem * create (QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador,
                              Parameter * k, Parameter* ret, QString exp_nume = 0, QString exp_deno = 0);

    QString numeratorString();
    QString denominatorString();

    LtiSystem * clone();

private:
    QString exp_nume;
    QString exp_deno;
};

#endif // QFTBX_FREE_FORM_H
