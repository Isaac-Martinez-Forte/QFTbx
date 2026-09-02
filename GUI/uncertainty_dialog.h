#ifndef QFTBX_UNCERTAINTY_DIALOG_H
#define QFTBX_UNCERTAINTY_DIALOG_H

#include "GUI/coefficient_tables.h"

#include <memory>

#include <vector>

#include <QDialog>

#include "src/core/range.h"
#include <QLabel>
#include <QLineEdit>
#include <QStringBuilder>
#include <QVector>
#include <list>
#include <QVBoxLayout>

#include "src/core/system/parameter.h"
#include "GUI/parlineedit.h"
#include "mpParser.h"
#include "src/core/math/sequence_vectors.h"

namespace Ui {
class UncertaintyDialog;
}

 /**
    * @class UncertaintyDialog
    * @brief Clase gráfica a través de la cual se puede introducir los distintos tipos de incertidumbre de la planta.
    * 
    * Dicha incertidumbre introducida después se guardará en una jerarquía de clases diferenciando los distintos tipos.
    * 
    * @author Isaac Martínez Forte
   */


class UncertaintyDialog : public QDialog
{
    Q_OBJECT
    
public:
  
   /**
    * @fn UncertaintyDialog
    * @brief Constructor de la clase, al ser una clase que hereda de QDialog tiene parámetros especiales.
    * 
    * @param parent Objeto que se envía por parámetros al constructor padre indicando cual es el padre de la clase.
    */
  
    explicit UncertaintyDialog(QWidget *parent = 0);
    ~UncertaintyDialog();

    
   /**
    * @fn numerator
    * @brief Función que retorna la incertidumbre del numeratorParameters que ha sido introducida por el usuario.
    * 
    * @return un QVector de Variables que contiene la incertidumbre introducida para cada parameter del numeratorParameters.
   */
    
    std::vector<Parameter> & numerator();
    
    
   /**
    * @fn denominator
    * @brief Función que retorna la incertidumbre del denominatorParameters que ha sido introducida por el usuario.
    * 
    * @return un QVector de Variables que contiene la incertidumbre introducida para cada parameter del denominatorParameters.
   */
    
    std::vector<Parameter> & denominator();

    
   /**
    * @fn gain
    * @brief Función que retorna la incertidumbre de la parameter K que representa la ganancia.
    * 
    * @return un objeto tipo QPointF que el par de valores que representa la incertidumbre de K.
   */
    
    Range gain();
    
    
   /**
    * @fn delay
    * @brief Función que retorna la incertidumbre de la parameter Ret que representa el retardo de la planta.
    * 
    * @return un objeto tipo QPointF que el par de valores que representa la incertidumbre de Ret.
   */
    
    Range delay();

    /// True when the user accepted the dialog with valid ranges.
    bool getTodoCorrecto();

    
   /**
    * @fn launch
    * @brief Función que pone en ejecución toda la funcionalidad de la clase gráfica.
    * 
    * @param numeratorParameters de la planta en forma de QString.
    * @param denominatorParameters de la planta en forma de QString.
    * @param k ganancia de la planta en forma de QString.
    * @param ret retardo de la planta en forma de QString.
    * 
    * @return booleano que indica si ha funcionado correctamente todo.
   */

    /// Takes the tables the plant or controller dialog read out of its line
    /// edits, and edits them.
    bool launch(CoefficientTable valueTable, CoefficientTable expressionTable,
                UncertainTable uncertainTable, bool rowsBuilt);


    /**
     * @fn launch
     * @brief Función que pone en ejecución toda la funcionalidad de la clase gráfica.
     *
     * @param numeratorParameters de la planta en forma de QString.
     * @param denominatorParameters de la planta en forma de QString.
     * @param k ganancia de la planta en forma de QString.
     *
     * @return booleano que indica si ha funcionado correctamente todo.
    */

    //bool launch(QVector<QString> *numeratorParameters, QVector<QString> *denominatorParameters, QString k);



    
private slots:
    void on_numeratorRadio_clicked();

    void on_denominatorRadio_clicked();

    void on_okButton_clicked();

signals:
    void close_ok ();

private:

    bool rowsBuilt = false;
    std::vector<Parameter> numeratorParameters;
    std::vector<Parameter> denominatorParameters;
    //A ParLineEdit is three QLineEdit POINTERS, and Qt owns those through
    //the row widget: the rows themselves are values, consumed front-first
    //as each parameter is read.
    std::list <ParLineEdit> numeratorRows;
    std::list <ParLineEdit> denominatorRows;
    QVBoxLayout *denominatorLayout = nullptr;
    QVBoxLayout *numeratorLayout = nullptr;

    //The row widgets belong to the numerator/denominator boxes: only the
    //container is the dialog's.
    QVector <QWidget *> rowWidgets;


    void buildRows();

    bool readRanges();
    void buildRow(QWidget *widget, QString numero, std::list <ParLineEdit> & vector, bool rowsBuilt);
   // void buildRows (QVector<QString> *numeratorParameters, QVector<QString> *denominatorParameters);
    qreal parse(QString cadena);

    std::unique_ptr<Ui::UncertaintyDialog> ui;

    qreal k;
    qreal ret;

    qreal resultado;
    mup::ParserX p;

    CoefficientTable valueTable;
    CoefficientTable expressionTable;
    UncertainTable uncertainTable;


    bool rangeOnlyMode = false;
    bool accepted_ok = false;
};

#endif // QFTBX_UNCERTAINTY_DIALOG_H
