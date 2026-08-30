#ifndef FORMATOLIBRE_H
#define FORMATOLIBRE_H

#include <QVector>

#include "funciontransferencia.h"
#include "complex"
#include "mpParser.h"

 /**
    * @class FormatoLibre
    * @brief Función de transferencia definida por expresiones de texto libre en 's'.
    *
    * Los vectores de variables no describen la estructura (numerador y
    * denominador son texto): solo enumeran las variables inciertas presentes
    * en las expresiones, para el barrido de templates.
    *
    * @author Isaac Martínez Forte
   */

class FormatoLibre : public FuncionTransferencia
{
public:

   /**
    * @fn FormatoLibre
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
    FormatoLibre(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador, Var * k, Var* ret, QString exp_nume,
                 QString exp_deno);

    QString getExpr (QVector <qreal> * numerador, QVector <qreal> * denominador,
                             qreal k, qreal ret, qreal omega);

    QString getExpr(qreal w);

    QString getExpr();

    std::complex <qreal> getPuntoNume(QVector <qreal> * nume, qreal omega);

    std::complex <qreal> getPuntoDeno(QVector <qreal> * deno, qreal omega);

    std::complex <qreal> getPunto (QVector <qreal> * numerador, QVector <qreal> * denominador,
                                           qreal k, qreal ret, qreal omega);

    //Se reexpone la resolución nominal heredada, oculta por las sobrecargas anteriores.
    using FuncionTransferencia::getPunto;

    tipo_planta getClass();

    Sistema * invoke (QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador,
                              Var * k, Var* ret, QString exp_nume = 0, QString exp_deno = 0);

    QString getNumeradorString();
    QString getDenominadorString();

    Sistema * clone();

private:
    QString exp_nume;
    QString exp_deno;
};

#endif // FORMATOLIBRE_H
