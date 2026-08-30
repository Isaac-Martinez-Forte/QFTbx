#ifndef TOOLS_H
#define TOOLS_H

#include <QVector>
#include <qmath.h>
#include <QMessageBox>
#include <QObject>
#include <QColor>
#include <Qt>
#include <QTranslator>


#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"

/**
    * @namespace tools
    * @brief namespace que agrupa funciones útiles que no dependen de ninguna clase.
    *
    * @author Isaac Martínez Forte
    */


namespace tools{



enum alg_loop_shaping {sachin, nandkishor, rambabu,
                       nandkishor_primeraversion, primer_articulo, segundo_articulo};


/**
    * @fn linSpace
    * @brief Función que calcula un linspace.
    *
    * @param a valor inicial del linspace.
    * @param b valor final del linspace.
    * @param N número de valores del linspace.
    */

QVector <qreal> * linspace(qreal a, qreal b, qint32 N);

std::vector <float> linspace1(qreal a, qreal b, qint32 N);


/**
    * @fn logSpace
    * @brief Función que calcula un logspace.
    *
    * @param a valor inicial del logspace.
    * @param b valor final del logspace.
    * @param log número de valores del logspace.
    */

QVector <qreal> * logspace (qreal a, qreal b, qint32 N);


/**
    * @fn menerror
    * @brief Función que saca un mensaje de error.
    *
    * @param mensaje de error a sacar.
    * @param nombre de la clase que lo ha producido.
    */

void menerror(QString mensaje, QString nombre);


/**
    * @fn srtovectorString
    * @brief Función que convierte una cadena en un vector de cadenas.
    *
    * @param cadena que queremos dividir.
    *
    * @return vector de string con las cadenas individuales.
    */

QVector <QString> * srtovectorString (QString cadena);


/**
    * @fn srtovectorReal
    * @brief Función que convierte una cadena de números en un vector
    *
    * @param cadena que queremos dividir.
    *
    * @return vector de reales con los números individuales.
    */

QVector <qreal> * srtovectorReal (QString cadena);


/**
    * @fn ramdonColor
    * @brief Función que retorna un color aleatorio.
    *
    * @return color aleatorio.
    */

QColor ramdonColor (qint32 i);


struct dBND{
    QString nombre;
    bool utilizado;
    LtiSystem * sistema;
    qreal altura;
    bool constante;
    qreal frecinicio;
    qreal frecfinal;

    qreal getAltura(qreal omega) {
        if (constante){
            return 20 * log10(altura);
        }

        return 20 * log10(abs(sistema->evaluate(omega)));
    }
};

enum flags_box{
    feasible,
    infeasible,
    ambiguous,
    completo
};

struct lado {
    bool siRecorte;
    tools::flags_box tipo;
};

struct recortes {
    tools::flags_box tipoCaja;
    lado imagSup;
    lado imagInf;
    lado realSup;
    lado realInf;
    QVector <qreal> * minimosMaximos;
};

QVector <Parameter *> * clonarVectorVar (QVector <Parameter *> * v);
}

#endif // TOOLS_H
