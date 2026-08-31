#include "plant_dialog.h"
#include "ui_plant_dialog.h"

#include "GUI/error_message.h"
#include "GUI/plot_palette.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"

using namespace tools;

namespace {

//The three parallel parse tables (values, expressions, flags) are created
//together and freed together. The error paths and accept itself used to
//abandon them with clear().
void releaseTables(QVector <QVector <QString> * > * valueTable,
                   QVector <QVector <QString> * > * expressionTable,
                   QVector <QVector <bool> * > * uncertainTable){
    if (valueTable != nullptr){
        qDeleteAll(*valueTable);
        delete valueTable;
    }
    qDeleteAll(*expressionTable);
    delete expressionTable;
    qDeleteAll(*uncertainTable);
    delete uncertainTable;
}

} // namespace

PlantDialog::PlantDialog(Controlador * controller, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PlantDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Plant input"));

    this->controller = controller;
    
    //Hide the buttons that do not apply yet.
    ui->zerosPolesRadio->setVisible(false);
    ui->polynomialRadio->setVisible(false);
    ui->zpkRadio->setVisible(false);
    ui->tcgRadio->setVisible(false);

    ui->polyGain->setText("1");
    ui->polyDelay->setText("0");

    ui->zpkGain->setText("1");
    ui->zpkDelay->setText("0");


    ui->tcgGain->setText("1");
    ui->tcgDelay->setText("0");

    ui->freeGain->setText("1");
    ui->freeDelay->setText("0");


    uncertaintyEntered = false;

    todoCorrecto = false;

    //The uncertainty dialog is created up front and reused.
    uncertaintyDialog = new UncertaintyDialog (this);

    //Plant figure images:

    QPixmap imagen1 (":/figures/kgan.png");
    ui->zpkImage->setPixmap(imagen1);

    QPixmap imagen2 (":/figures/knogan.png");
    ui->tcgImage->setPixmap(imagen2);

    QPixmap imagen3 (":/figures/copol.png");
    ui->polyImage->setPixmap(imagen3);

    //Wire the cancel button.
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
}

PlantDialog::~PlantDialog()
{
    delete ui;
    delete uncertaintyDialog;
}

void PlantDialog::on_zerosPolesRadio_toggled(bool checked)
{
    ui->zpkRadio->setVisible(checked);
    ui->tcgRadio->setVisible(checked);
    ui->zerosPolesRadio->setVisible(checked);
    ui->polynomialRadio->setVisible(checked);

    if (checked == true)
        ui->formStack->setCurrentIndex(0);
}

void PlantDialog::on_transferFunctionRadio_toggled(bool checked)
{
    ui->zerosPolesRadio->setVisible(checked);
    ui->polynomialRadio->setVisible(checked);

    if (checked == true)
        ui->formStack->setCurrentIndex(0);
}

void PlantDialog::on_zpkRadio_toggled(bool checked)
{
    ui->zerosPolesRadio->setVisible(checked);
    ui->polynomialRadio->setVisible(checked);
    ui->zpkRadio->setVisible(checked);
    ui->tcgRadio->setVisible(checked);

    if (checked == true)
        ui->formStack->setCurrentIndex(3);

}

void PlantDialog::on_tcgRadio_toggled(bool checked)
{

    ui->zerosPolesRadio->setVisible(checked);
    ui->polynomialRadio->setVisible(checked);
    ui->zpkRadio->setVisible(checked);
    ui->tcgRadio->setVisible(checked);

    if (checked == true)
        ui->formStack->setCurrentIndex(4);
}

void PlantDialog::on_polynomialRadio_toggled(bool checked)
{
    ui->zerosPolesRadio->setVisible(checked);
    ui->polynomialRadio->setVisible(checked);

    if (checked == true)
        ui->formStack->setCurrentIndex(2);
}

bool PlantDialog::parseCoefficients(QVector<QVector <QString> * > * tabla, QLineEdit *linea,
                                      QVector<QVector <QString> * > * expressionTable, QVector <QVector <bool> * > * uncertainTable){

    QVector <QString> * vec1 = tools::srtovectorString(linea->text());
    QVector <QString> * vec = new QVector <QString> ();
    QVector <bool> * vec2  = new QVector <bool> ();

    if (linea->text().isEmpty()){
        vec1->append("1");
        vec2->append(false);
    } else{

        foreach (QString e, *vec1) {

            QRegularExpression re("[a-zA-Z]+");

            QRegularExpressionMatch match = re.match(e);
            QString capture = match.captured(0);
            e.remove(capture);

            bool isUncertain = false;

            while (!capture.isNull()){

                if (!p.IsFunDefined(capture.toStdString())){
                    vec->append(capture);
                    capture = QString();
                    isUncertain = true;
                    break;
                }
                match = re.match(e);
                capture = match.captured(0);
                e.remove(capture);
            }

            vec2->append(isUncertain);

            if (!isUncertain){
                vec->append(e);
            }
        }
    }

    tabla->append(vec);
    uncertainTable->append(vec2);
    expressionTable->append(vec1);

    return true;
}

bool PlantDialog::parseScalar(QVector<QVector <QString> * > * tabla, QLineEdit *linea,
                                          QVector <QVector <QString> * > * expressionTable, QVector <QVector <bool> * > * uncertainTable){


    QString aux = linea->text();
    aux = aux.trimmed();

    QVector <QString> * vec1 = new QVector <QString> (1, aux);
    QVector <QString> * vec = new QVector <QString> ();
    QVector <bool> * vec2 = new QVector <bool> ();

    QRegularExpression re("[a-zA-Z]+");
    QRegularExpressionMatch match = re.match(aux);
    qint32 i = 0;
    QString capture = match.captured(i);

    bool isUncertain = false;

    while (!capture.isNull()){

        if (!p.IsFunDefined(capture.toStdString())){
            vec->append(capture);
            capture = QString();
            isUncertain = true;
            break;
        }
        match = re.match(aux);
        capture = match.captured(0);
        aux.remove(capture);
    }

    vec2->append(isUncertain);

    if (!isUncertain){
        vec->append(aux);
    }

    tabla->append(vec);
    expressionTable->append(vec1);
    uncertainTable->append(vec2);

    return true;
}

bool PlantDialog::parseFreeForm(QLineEdit * linea, QVector<QVector <QString> * > * tabla, QVector<QVector<QString> *> *expressionTable,
                       QVector <QVector <bool> * > * uncertainTable){

    QVector <QString> * expressions = new QVector <QString> ();
    QVector <QString> * values = new QVector <QString> ();
    QVector <bool> * flags  = new QVector <bool> ();

    QString text = linea->text();

    QRegularExpression re("[a-zA-Z]+");
    QRegularExpressionMatch match = re.match(text);
    QString capture = match.captured(0);

    text.remove(capture);

    while (!capture.isNull()){

        if (!p.IsFunDefined(capture.toStdString()) && capture != "s"){

            expressions->append(capture);
            values->append(capture);
            flags->append(true);

            capture = QString();
        }
        match = re.match(text);
        capture = match.captured(0);
        text.remove(capture);
    }


    tabla->append(values);
    expressionTable->append(expressions);
    uncertainTable->append(flags);

    return true;
}


QVector<QVector <QString> * > * PlantDialog::readTables(QVector <QVector <QString> * > * expressionTable, QVector <QVector <bool> * > * uncertainTable){

    QVector <QVector <QString> * > * tables = new QVector <QVector <QString> * > ();

    if (ui->polynomialRadio->isChecked()){
        parseCoefficients(tables, ui->polyNumerator, expressionTable, uncertainTable);
        parseCoefficients(tables, ui->polyDenominator, expressionTable, uncertainTable);
        parseScalar(tables, ui->polyGain, expressionTable, uncertainTable);
        parseScalar(tables, ui->polyDelay, expressionTable, uncertainTable);
    }else if (ui->zpkRadio->isChecked()){
        parseCoefficients(tables, ui->zpkNumerator, expressionTable, uncertainTable);
        parseCoefficients(tables, ui->zpkDenominator, expressionTable, uncertainTable);
        parseScalar(tables, ui->zpkGain, expressionTable, uncertainTable);
        parseScalar(tables, ui->zpkDelay, expressionTable, uncertainTable);
    }else if (ui->tcgRadio->isChecked()){
        parseCoefficients(tables, ui->tcgNumerator, expressionTable, uncertainTable);
        parseCoefficients(tables, ui->tcgDenominator, expressionTable, uncertainTable);
        parseScalar(tables, ui->tcgGain, expressionTable, uncertainTable);
        parseScalar(tables, ui->tcgDelay, expressionTable, uncertainTable);
    }else if (ui->freeFormRadio->isChecked()){
        parseFreeForm(ui->freeNumerator, tables, expressionTable, uncertainTable);
        parseFreeForm(ui->freeDenominator, tables, expressionTable, uncertainTable);
        parseScalar(tables, ui->freeGain, expressionTable, uncertainTable);
        parseScalar(tables, ui->freeDelay, expressionTable, uncertainTable);
    }else{
        tables->clear();
        expressionTable->clear();
        uncertainTable->clear();
        return NULL;
    }

    /* if (!valid){
        tables->clear();
        return NULL;
    }*/

    return tables;
}

void PlantDialog::on_okButton_clicked()
{

    //The name is required here too: only the uncertainty path used to
    //validate it, and nameless plants could be saved.
    if (ui->nameEdit->text().isEmpty()){
        errorMessage(tr("The plant name is missing."), tr("Plant input"));
        ui->nameEdit->setStyleSheet("background : red");
        return;
    }
    ui->nameEdit->setStyleSheet("background : white");

    QVector <QVector <QString> * > * expressionTable = new QVector <QVector <QString> * > ();
    QVector <QVector <bool> * > *  uncertainTable = new QVector <QVector <bool> * >  ();
    QVector <QVector <QString> * > * valueTable = readTables(expressionTable, uncertainTable);

    if (valueTable == NULL){
        qDeleteAll(*expressionTable);
        delete expressionTable;
        qDeleteAll(*uncertainTable);
        delete uncertainTable;
        errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
        return;
    }

    Parameter * kv = nullptr;
    Parameter * retv = nullptr;

    //The expressions come from the user: a muParserX syntax error used to
    //throw and bring the application down.
    try {
        if (valueTable->at(2)->size() == 0){
            kv = new Parameter (1);
        }else{

            QPointF range_point = uncertaintyDialog->gain();
            p.SetExpr(expressionTable->at(2)->at(0).toStdString());
            qreal d = p.Eval().GetFloat();

            if (d == range_point.x() && d == range_point.y()){
                kv = new Parameter (d);
            }else {
                kv = new Parameter ("kv", range_point, d, "kv");
            }
        }

        if (valueTable->at(3)->size() == 0){
            retv = new Parameter (qreal(0));
        }else{

            QPointF range_point = uncertaintyDialog->delay();
            p.SetExpr(expressionTable->at(3)->at(0).toStdString());
            qreal d = p.Eval().GetFloat();

            if (d == range_point.x() && d == range_point.y()){
                retv = new Parameter (d);
            }else {
                retv = new Parameter ("ret", range_point, d, "ret");
            }
        }
    } catch (mup::ParserError &) {
        delete kv;
        releaseTables(valueTable, expressionTable, uncertainTable);
        errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
        return;
    }

    //The uncertainty only counts if its dialog was ACCEPTED (opening and
    //cancelling used to leave the flag set and a half-built state in use).
    if (uncertaintyEntered && uncertaintyDialog->getTodoCorrecto()){
        //The plant takes ownership of its parameters: it receives copies
        //and the uncertainty dialog keeps its originals for editing.
        QVector <Parameter*> * nume = Parameter::cloneVector(uncertaintyDialog->numerator());
        QVector <Parameter*> * deno = Parameter::cloneVector(uncertaintyDialog->denominator());

        if (ui->zpkRadio->isChecked()){
            plant = new ZeroPoleGain(ui->nameEdit->text(),nume, deno,kv,retv);
        }else if(ui->tcgRadio->isChecked()){
            plant = new TimeConstantGain(ui->nameEdit->text(),nume, deno,kv,retv);
        }else if (ui->polynomialRadio->isChecked()){
            plant = new PolynomialForm(ui->nameEdit->text(), nume, deno,kv,retv);
        }else{
            plant = new FreeForm(ui->nameEdit->text(), nume, deno,kv,retv,
                                      ui->freeNumerator->text(), ui->freeDenominator->text());
        }
    }else{
        if (ui->zpkRadio->isChecked()){
            plant = new ZeroPoleGain(ui->nameEdit->text(),buildParameters(valueTable->at(0)),
                                   buildParameters(valueTable->at(1)),kv,retv );
        }else if(ui->tcgRadio->isChecked()){
            plant = new TimeConstantGain(ui->nameEdit->text(),buildParameters(valueTable->at(0)),
                                    buildParameters(valueTable->at(1)),kv,retv);
        }else if (ui->polynomialRadio->isChecked()){
            plant = new PolynomialForm(ui->nameEdit->text(), buildParameters(valueTable->at(0)),
                                     buildParameters(valueTable->at(1)),kv,retv);
        }else {
            plant = new FreeForm(ui->nameEdit->text(), buildParameters(valueTable->at(0)),
                                      buildParameters(valueTable->at(1)),kv,retv,
                                      ui->freeNumerator->text(), ui->freeDenominator->text());
        }


    }

    controller->setPlanta(plant);
    releaseTables(valueTable, expressionTable, uncertainTable);

    todoCorrecto = true;

    this->close();
}

QVector <Parameter * > * PlantDialog::buildParameters(QVector <QString> * numeros){
    QVector <Parameter *> * var = new QVector <Parameter *> ();
    var->reserve(numeros->size());

    if (numeros->isEmpty()){
        return var;
    }

    foreach (const QString &string, *numeros) {
        p.SetExpr(string.toStdString());
        try {
            var->append(new Parameter(p.Eval().GetFloat()));
        } catch (mup::ParserError &) {
            //Invalid coefficient: 0 is used and readTables already warned;
            //the exception used to bring the application down.
            var->append(new Parameter(qreal(0)));
        }
    }

    return var;
}

void PlantDialog::on_uncertaintyButton_clicked()
{
    QVector <QVector <QString> * > * expressionTable  = new QVector <QVector <QString> * > ();
    QVector <QVector <bool> * > *  uncertainTable = new QVector <QVector <bool> * >  ();
    QVector <QVector <QString> * > * valueTable = readTables(expressionTable, uncertainTable);

    if (valueTable == NULL){
        qDeleteAll(*expressionTable);
        delete expressionTable;
        qDeleteAll(*uncertainTable);
        delete uncertainTable;
        errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
        return;
    }

    if (ui->nameEdit->text().isEmpty()){
        releaseTables(valueTable, expressionTable, uncertainTable);
        errorMessage(tr("The plant name is missing."), tr("Plant input"));
        ui->nameEdit->setStyleSheet("background : red");
        return;
    }else{
        ui->nameEdit->setStyleSheet("background : white");
    }


    //The tables become property of the uncertainty dialog.
    uncertaintyDialog->launch(valueTable, expressionTable, uncertainTable, false);
    uncertaintyDialog->show();
    uncertaintyEntered = true;
}

void PlantDialog::on_freeFormRadio_clicked()
{
    ui->formStack->setCurrentIndex(1);
}


bool PlantDialog::getTodoCorrecto(){
    return todoCorrecto;
}

