#ifndef QFTBX_UNCERTAINTY_DIALOG_H
#define QFTBX_UNCERTAINTY_DIALOG_H

#include "src/gui/coefficient_tables.h"

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
#include "src/gui/parlineedit.h"
#include "mpParser.h"
#include "src/core/math/sequence_vectors.h"

namespace Ui {
class UncertaintyDialog;
}

/**
 * @brief Edits the parametric uncertainty of a plant or a controller: the
 * minimum, maximum and nominal value of every coefficient the user marked
 * as uncertain, plus the gain and the delay.
 *
 * It works on the tables the calling dialog read out of its line edits and
 * hands them back edited; it never touches the project.
 *
 * @author Isaac Martínez Forte
 */
class UncertaintyDialog : public QDialog
{
    Q_OBJECT
    
public:
  
    explicit UncertaintyDialog(QWidget *parent = 0);
    ~UncertaintyDialog();

    
   /// The numerator coefficients as the user left them, one Parameter per
    /// coefficient, uncertain ones carrying their range.
    std::vector<Parameter> & numerator();
    
    
   /// The denominator coefficients, likewise.
    std::vector<Parameter> & denominator();

    
   /// Range of the gain k.
    Range gain();
    
    
   /// Range of the transport delay.
    Range delay();

    /// True when the user accepted the dialog with valid ranges.
    bool getTodoCorrecto();

    
   /**
    * @brief Shows the dialog over the tables the caller read out of its
    * line edits.
    *
    * @param valueTable the numeric value of every coefficient, by row.
    * @param expressionTable the reparametrising expression of each, if any.
    * @param uncertainTable which of them the user marked as uncertain.
    * @param rowsBuilt whether the rows already exist from a previous edit,
    * so their contents are kept instead of rebuilt.
    * @return whether the user accepted with valid ranges.
    */
    bool launch(CoefficientTable valueTable, CoefficientTable expressionTable,
                UncertainTable uncertainTable, bool rowsBuilt);



    
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

    mup::ParserX p;

    CoefficientTable valueTable;
    CoefficientTable expressionTable;
    UncertainTable uncertainTable;


    bool rangeOnlyMode = false;
    bool accepted_ok = false;
};

#endif // QFTBX_UNCERTAINTY_DIALOG_H
