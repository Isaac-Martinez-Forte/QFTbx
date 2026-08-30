#ifndef INTRODUCIRTEMPLATES_H
#define INTRODUCIRTEMPLATES_H

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QString>
#include <QMessageBox>
#include <QRadioButton>
#include <QHash>


#include "Modelo/EstructurasDatos/parlineedit.h"
#include "src/core/system/lti_system.h"
#include "Modelo/controlador.h"
#include "src/core/system/parameter.h"
#include "Modelo/EstructurasDatos/parlineedit.h"
#include "Modelo/Herramientas/tools.h"
#include "intespecificaciones.h"

#include "mpParser.h"


  /**
    * @class ViewTemplates
    * @brief Clase gráfica que sirve para que el usuario introduzca los datos necesarios para calcular los templates.
    * 
    * @author Isaac Martínez Forte
    */



namespace Ui {
class IntroducirTemplates;
}

class IntroducirTemplates : public QDialog
{
    Q_OBJECT
    
public:
  
  
   /**
    * @fn IntroducirTemplates
    * @brief Constructor de la clase que solo tiene como parámetro el padre de la misma.
    * 
    * @param parent padre de la clase que se para como parámetro al constructor de la superclase, puede ser vacío.
    */
  
    explicit IntroducirTemplates(QWidget *parent = 0);

    ~IntroducirTemplates();

    
   /**
    * @fn lanzarViewTemp
    * @brief Función que lanza la creación gráfica de la clase.
    */
    
    void lanzarViewTemp(LtiSystem * planta, qint32 numOmegas);
    
    
   /**
    * @fn getMapa
    * @brief Función que retorna un mapa con los distintos valores que pueden tomar las variables.
    * 
    * @return mapa hash con los distintos valores que pueden tomar las variables.
    */
    
    QHash<QString, QVector<qreal> *> * getMapa();
    
    
   /**
    * @fn getEpsilon
    * @brief Función que retorna el valor de epsilon.
    * 
    * @return real con el valor de epsilon necesario para calcular el contorno de los templates.
    */
    
    QVector <qreal> * getEpsilon();
    
    
   /**
    * @fn getElecDiagram
    * @brief Función que retorna un booleano indicando que tipo de diagrama ha seleccionado el usuario para representar los templates y su contorno.
    * 
    * @return booleano con el tipo de diagrama seleccionado por el usuario.
    */
    
    bool getElecDiagram();


    /**
     * @fn getElecCUDA
     * @brief Función que retorna un booleano indicando si se ha elegido usar CUDA.
     *
     * @return booleano indicando si se ha elegido usar CUDA.
     */

    bool getElecCUDA();


    struct tresRadioButton{
        QRadioButton * uno;
        QRadioButton * dos;
        QRadioButton * tres;
    };


    bool getTodoCorrecto();
    
private slots:
    void on_Todas_clicked();

    void on_unaxuna_clicked();

    void on_radioNumerador_clicked();

    void on_radioDenominador_clicked();

    void on_cancelar_clicked();

    void on_Aceptar_clicked();

signals:
    void close_ok ();


private:
    Ui::IntroducirTemplates *ui;


    void crearWidget (QWidget *widget, QVector<ParLineEdit *> *par, QVector <tresRadioButton> * radioButtons);
    void formartablas(QVector<Parameter *> *numerador, QVector<Parameter *> *denominador);
    bool extraerVariable(ParLineEdit* parlines, tresRadioButton radioButtons, Parameter * var,
                         bool linsp, bool logsp);

    QVector <ParLineEdit*>* parNume;
    QVector <ParLineEdit*>* parDeno ;
    QHash <QString, QVector<qreal> * > * mapa = NULL;
    QVector <tresRadioButton> * radioButtonsNume;
    QVector <tresRadioButton> * radioButtonsDeno;
    QVector<Parameter *> *numerador;
    QVector<Parameter *> *denominador;
    LtiSystem * planta;

    bool variablesCreadas = false;
    bool cuda = false;

    bool diagrama  = true;

    mup::ParserX * parser;

    bool todoCorrecto;

    QVector <qreal> * epsilon;

    qint32 numOmegas;

};

#endif // INTRODUCIRTEMPLATES_H
