#ifndef QFTBX_TEMPLATES_DIALOG_H
#define QFTBX_TEMPLATES_DIALOG_H

#include "src/core/settings.h"
#include "src/gui/step_dialog.h"
#include <memory>

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


#include "src/gui/parlineedit.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/parameter_grids.h"
#include "src/core/system/parameter.h"
#include "src/core/math/sequence_vectors.h"

#include "mpParser.h"


namespace Ui {
class TemplatesDialog;
}

namespace qftbx {


/**
 * @brief Step 3 of the design: the sweep grid of every uncertain plant
 * parameter and the epsilon of the contour walk.
 *
 * The class block used to name TemplateViewer, which is a different class.
 *
 * @author Isaac Martínez Forte
 */
class TemplatesDialog : public StepDialog
{
    Q_OBJECT
    
public:
    /// Ceiling on a parameter grid's point count, from the settings. It only
    /// ever refuses input, so moving it changes no computed result.
    void setMaxPointCount(double points) { m_maxPointCount = points; }

    /// Prefills the general point count from the settings.
    void setDefaultPointCount(std::int32_t points);

  
  
    explicit TemplatesDialog(QWidget *parent = 0);

    ~TemplatesDialog();

    
   /**
    * @brief Builds the rows and shows the dialog.
    *
    * @param plant the plant whose uncertain parameters need a grid.
    * @param frequencyCount how many epsilon values to ask for, one per
    * design frequency.
    */
    void launch(LtiSystem * plant, qint32 frequencyCount);
    
    
    /// The grids BY VALUE: nobody has to free them, and the dialog keeps its
    /// own copy for a second accept. See qftbx::ParameterGrids.
    qftbx::ParameterGrids grids() const;
    
    
    /// The per-frequency epsilon the user described, or nullptr when the
    /// dialog was cancelled or rejected. Ownership passes to the caller:
    /// this used to be a plain getter whose value the project then took,
    /// leaving the dialog holding a dangling pointer between accepts.
    std::vector<double> takeEpsilon();
    
    
    /// Which plane the templates and their contour are drawn on: Nichols
    /// rather than Nyquist.
    bool nicholsSelected();


    /// Whether the user asked for the GPU path (requires a CUDA build).
    bool cudaSelected();


    struct ThreeRadioButtons{
        //Observers on radio buttons owned by their row widget: the three
        //ways of entering one parameter's grid.
        QRadioButton * linear = nullptr;
        QRadioButton * logarithmic = nullptr;
        QRadioButton * manual = nullptr;
    };


    
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

    std::unique_ptr<Ui::TemplatesDialog> ui;


    void buildRow (QWidget *widget, QVector<ParLineEdit> & par,
                   QVector <ThreeRadioButtons> & rowRadios);
    void buildTables(std::vector<Parameter> & numerator, std::vector<Parameter> & denominator);
    /// Why the last readVariable() refused its input, empty when it has
    /// nothing to add to the caller's message. It exists so the operator is
    /// told WHICH rule the entry broke: "invalid" alone left them guessing
    /// between a syntax error and a count out of range.
    QString m_readReason;

    bool readVariable(const ParLineEdit & rowEdits, ThreeRadioButtons rowRadios, Parameter & parameter,
                         bool useLinspace, bool useLogspace);

    //A ParLineEdit is three QLineEdit POINTERS, and Qt owns those through
    //the row widget: the rows themselves are values.
    QVector <ParLineEdit> numeratorRows;
    QVector <ParLineEdit> denominatorRows;
    qftbx::ParameterGrids gridMap;
    QVector <ThreeRadioButtons> numeratorRadios;
    QVector <ThreeRadioButtons> denominatorRadios;
    std::vector<Parameter> numerator;
    std::vector<Parameter> denominator;
    //An observer on the project's plant, handed in by launch(): the
    //dialog never owns it.
    LtiSystem * plant = nullptr;

    bool rowsBuilt = false;
    bool cudaEnabled = false;

    bool nicholsDiagram  = true;

    std::unique_ptr<mup::ParserX> parser;


    std::vector<double> epsilonValues;

    //Names entered more than once in the current OK pass (numerator and
    //denominator sharing a parameter): reported once to the user.
    QStringList duplicateNames;

    qint32 frequencyCount = 0;

    double m_maxPointCount = qftbx::Settings().limits.maxTemplatePoints;

};

} // namespace qftbx

#endif // QFTBX_TEMPLATES_DIALOG_H
