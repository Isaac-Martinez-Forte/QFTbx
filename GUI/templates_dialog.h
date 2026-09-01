#ifndef QFTBX_TEMPLATES_DIALOG_H
#define QFTBX_TEMPLATES_DIALOG_H

#include <vector>

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
#include <QStringList>


#include "GUI/parlineedit.h"
#include "src/core/system/lti_system.h"
#include "Modelo/controlador.h"
#include "src/core/system/parameter.h"
#include "GUI/parlineedit.h"
#include "src/core/math/sequence_vectors.h"
#include "specifications_dialog.h"

#include "mpParser.h"


  /**
    * @class TemplateViewer
    * @brief Clase gráfica que sirve para que el usuario introduzca los datos necesarios para calcular los templates.
    * 
    * @author Isaac Martínez Forte
    */



namespace Ui {
class TemplatesDialog;
}

class TemplatesDialog : public QDialog
{
    Q_OBJECT
    
public:
  
  
   /**
    * @fn TemplatesDialog
    * @brief Constructor de la clase que solo tiene como parámetro el padre de la misma.
    * 
    * @param parent padre de la clase que se para como parámetro al constructor de la superclase, puede ser vacío.
    */
  
    explicit TemplatesDialog(QWidget *parent = 0);

    ~TemplatesDialog();

    
   /**
    * @fn launch
    * @brief Función que lanza la creación gráfica de la clase.
    */
    
    void launch(LtiSystem * plant, qint32 frequencyCount);
    
    
   /**
    * @fn grids
    * @brief Función que retorna un gridMap con los distintos valores que pueden tomar las variables.
    * 
    * @return gridMap hash con los distintos valores que pueden tomar las variables.
    */
    
    QHash<QString, QVector<qreal> *> * grids();
    
    
   /**
    * @fn getEpsilon
    * @brief Función que retorna el name_text de epsilonValues.
    * 
    * @return real con el name_text de epsilonValues necesario para calcular el contorno de los templates.
    */
    
    QVector <qreal> * getEpsilon();
    
    
   /**
    * @fn nicholsSelected
    * @brief Función que retorna un booleano indicando que tipo de nicholsDiagram ha seleccionado el usuario para representar los templates y su contorno.
    * 
    * @return booleano con el tipo de nicholsDiagram seleccionado por el usuario.
    */
    
    bool nicholsSelected();


    /**
     * @fn cudaSelected
     * @brief Función que retorna un booleano indicando si se ha elegido usar cudaCheck.
     *
     * @return booleano indicando si se ha elegido usar cudaCheck.
     */

    bool cudaSelected();


    struct ThreeRadioButtons{
        QRadioButton * uno;
        QRadioButton * dos;
        QRadioButton * tres;
    };


    bool getTodoCorrecto();
    
private slots:
    void on_allVariablesRadio_clicked();

    void on_oneByOneRadio_clicked();

    void on_numeratorRadio_clicked();

    void on_denominatorRadio_clicked();

    void on_cancelButton_clicked();

    void on_okButton_clicked();

signals:
    void close_ok ();


private:
    void clearTables();
    void releaseGrids();

    Ui::TemplatesDialog *ui;


    void buildRow (QWidget *widget, QVector<ParLineEdit *> *par, QVector <ThreeRadioButtons> * rowRadios);
    void buildTables(std::vector<Parameter> & numerator, std::vector<Parameter> & denominator);
    bool readVariable(ParLineEdit* rowEdits, ThreeRadioButtons rowRadios, Parameter & parameter,
                         bool useLinspace, bool useLogspace);

    QVector <ParLineEdit*>* numeratorRows;
    QVector <ParLineEdit*>* denominatorRows ;
    QHash <QString, QVector<qreal> * > * gridMap = NULL;
    QVector <ThreeRadioButtons> * numeratorRadios;
    QVector <ThreeRadioButtons> * denominatorRadios;
    std::vector<Parameter> numerator;
    std::vector<Parameter> denominator;
    LtiSystem * plant;

    bool rowsBuilt = false;
    bool cudaEnabled = false;

    bool nicholsDiagram  = true;

    mup::ParserX * parser;

    bool todoCorrecto;

    QVector <qreal> * epsilonValues;

    //Names entered more than once in the current OK pass (numerator and
    //denominator sharing a parameter): reported once to the user.
    QStringList duplicateNames;

    qint32 frequencyCount;

};

#endif // QFTBX_TEMPLATES_DIALOG_H
