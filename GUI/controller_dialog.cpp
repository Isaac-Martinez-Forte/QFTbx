#include <vector>
#include "controller_dialog.h"
#include "src/core/text_tokens.h"
#include "ui_controller_dialog.h"

#include "GUI/error_message.h"
#include "src/core/math/expression_cache.h"
#include "GUI/plot_palette.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"

using namespace tools;
using namespace mup;

namespace {

//The three parallel parse tables are freed together (they used to be
//abandoned with clear() or simply lost on the error paths).
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

ControllerDialog::ControllerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ControllerDialog)
{
    ui->setupUi(this);


    setWindowTitle(tr("Controller structure input"));

    QPixmap imagen1 (":/figures/kgan.png");
    ui->zpkImage->setPixmap(imagen1);

    QPixmap imagen2 (":/figures/knogan.png");
    ui->tcgImage->setPixmap(imagen2);

    QPixmap imagen3 (":/figures/copol.png");
    ui->polyImage->setPixmap(imagen3);

    ui->gainStart->setText("1");
    ui->gainEnd->setText("1");

    uncertaintyEntered = false;

    //The uncertainty dialog is created up front and reused.
    uncertaintyDialog = new UncertaintyDialog (this);

    todoCorrecto = false;
}

ControllerDialog::~ControllerDialog()
{
    delete ui;

    //Not taken (cancelled, or never asked for): the dialog frees it.
    delete controllerSystem;
}

void ControllerDialog::on_polynomialRadio_clicked()
{
    ui->figureStack->setCurrentIndex(1);
}

void ControllerDialog::on_zpkRadio_clicked()
{
    ui->figureStack->setCurrentIndex(2);
}

void ControllerDialog::on_tcgRadio_clicked()
{
    ui->figureStack->setCurrentIndex(3);
}

void ControllerDialog::on_uncertaintyButton_clicked()
{
    QVector <QVector <QString> * > * expressionTable  = new QVector <QVector <QString> * > ();
    QVector <QVector <bool> * > *  uncertainTable = new QVector <QVector <bool> * >  ();
    QVector <QVector <QString> * > * valueTable = readTables(expressionTable, uncertainTable);

    if (valueTable == NULL){
        qDeleteAll(*expressionTable);
        delete expressionTable;
        qDeleteAll(*uncertainTable);
        delete uncertainTable;
        errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
        return;
    }


    //The tables become property of the uncertainty dialog.
    uncertaintyDialog->launch(valueTable, expressionTable, uncertainTable, true);
    uncertaintyDialog->show();
    uncertaintyEntered = true;
}

QVector<QVector <QString> * > * ControllerDialog::readTables(QVector <QVector <QString> * > * expressionTable,
                                                            QVector <QVector <bool> * > * uncertainTable){

    bool valid = true;
    QVector <QVector <QString> * > * tables = new QVector <QVector <QString> * > ();
    tables->reserve(4);

    //&& on the RIGHT so every parse still runs and every problem is reported:
    //the results used to overwrite each other, so only the gain range decided.
    if (ui->freeFormRadio->isChecked()){
        valid = parseFreeForm(ui->numeratorEdit, tables, expressionTable, uncertainTable) && valid;
        valid = parseFreeForm(ui->denominatorEdit, tables, expressionTable, uncertainTable) && valid;
        valid = parseGainRange(tables, ui->gainStart, ui->gainEnd, expressionTable, uncertainTable) && valid;
    }else {
        valid = parseCoefficients(tables, ui->numeratorEdit, expressionTable, uncertainTable) && valid;
        valid = parseCoefficients(tables, ui->denominatorEdit, expressionTable, uncertainTable) && valid;
        valid = parseGainRange(tables, ui->gainStart, ui->gainEnd, expressionTable, uncertainTable) && valid;
    }

    if (!valid){
        qDeleteAll(*tables);
        delete tables;
        return NULL;
    }

    return tables;
}

bool ControllerDialog::parseCoefficients(QVector<QVector <QString> * > * tabla, QLineEdit *linea,
                                      QVector<QVector <QString> * > * expressionTable, QVector <QVector <bool> * > * uncertainTable){

    QVector <QString> * vec1 = qftbx::text::tokens(linea->text());
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

                //A name the parser already owns cannot become a parameter:
                //the binding fails later and the plant stops evaluating.
                //Only FUNCTION names were checked here, which let through
                //the constants (e, pi, i) and the unit operators - "k",
                //"m", "u" - where "k" is the obvious name for a gain and
                //would otherwise be read as the multiplier 1e3.
                if (qftbx::math::isReservedName(capture)){
                    errorMessage(tr("\"%1\" cannot be used as a parameter name: "
                                    "the expression parser already defines it.").arg(capture),
                                 tr("Controller input"));
                    //The three row vectors are still ours on this path.
                    delete vec1;
                    delete vec;
                    delete vec2;

                    return false;
                }

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

bool ControllerDialog::parseGainRange(QVector<QVector <QString> * > * tabla, QLineEdit *linea1, QLineEdit * linea2,
                                          QVector <QVector <QString> * > * expressionTable, QVector <QVector <bool> * > * uncertainTable){


    QString aux = linea1->text().trimmed();
    QString aux1 = linea2->text().trimmed();

    QVector <QString> * vec1 = new QVector <QString> ();
    QVector <QString> * vec = new QVector <QString> ();
    QVector <bool> * vec2 = new QVector <bool> ();
    vec2->append(true);

    vec->append(aux);
    vec->append(aux1);

    vec1->append(aux);
    vec1->append(aux1);

    tabla->append(vec);
    expressionTable->append(vec1);
    uncertainTable->append(vec2);

    return true;
}

bool ControllerDialog::parseFreeForm(QLineEdit * linea, QVector<QVector <QString> * > * tabla, QVector<QVector<QString> *> *expressionTable,
                                         QVector <QVector <bool> * > * uncertainTable){

    QVector <QString> * expressions = new QVector <QString> ();
    QVector <QString> * values = new QVector <QString> ();
    QVector <bool> * flags  = new QVector <bool> ();

    QString text = linea->text();

    QRegularExpression re("[a-zA-Z]+");
    QRegularExpressionMatch match = re.match(text);
    QString capture = match.captured(0);

    //The first capture must leave the string BEFORE the loop (as in
    //PlantDialog): without this every uncertain variable was registered
    //TWICE on the free-form path.
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


void ControllerDialog::on_cancelButton_clicked()
{
    emit(close());
}

void ControllerDialog::on_okButton_clicked()
{

    QVector <QVector <QString> * > * expressionTable = new QVector <QVector <QString> * > ();
    QVector <QVector <bool> * > *  uncertainTable = new QVector <QVector <bool> * >  ();
    QVector <QVector <QString> * > * valueTable = readTables(expressionTable, uncertainTable);

    if (valueTable == NULL){
        qDeleteAll(*expressionTable);
        delete expressionTable;
        qDeleteAll(*uncertainTable);
        delete uncertainTable;
        errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
        return;
    }


    Parameter kv;
    Parameter retv;

    //User expressions: a syntax error used to throw and bring the
    //application down.
    try {
        if (valueTable->at(2)->size() == 0){
            kv = Parameter(1);
        }else{

            Range range_point;
            p.SetExpr(expressionTable->at(2)->at(0).toStdString());
            range_point.min = p.Eval().GetFloat();

            p.SetExpr(expressionTable->at(2)->at(1).toStdString());
            range_point.max = p.Eval().GetFloat();

            if (range_point.min == range_point.max){
                kv = Parameter(range_point.min);
            }else {

                if (range_point.min > range_point.max){
                    qreal a = range_point.min;
                    range_point.min = range_point.max;
                    range_point.max = a;
                }

                kv = Parameter("kv", range_point, (range_point.min + range_point.max) / 2);
            }
        }
    } catch (mup::ParserError &) {
        releaseTables(valueTable, expressionTable, uncertainTable);
        errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
        return;
    }

    retv = Parameter(0.0);


    //A second accept replaces the answer of the first one; whoever took it
    //already owns that one.
    delete controllerSystem;
    controllerSystem = nullptr;

    //The uncertainty only counts if its dialog was ACCEPTED.
    if (uncertaintyEntered && uncertaintyDialog->getTodoCorrecto()){
        //The controller receives COPIES: the uncertainty dialog keeps its
        //own parameters for further editing.
        std::vector<Parameter> numeratorEdit = uncertaintyDialog->numerator();
        std::vector<Parameter> denominatorEdit = uncertaintyDialog->denominator();

        if (ui->zpkRadio->isChecked()){
            controllerSystem = new ZeroPoleGain("",numeratorEdit, denominatorEdit,kv,retv);
        }else if(ui->tcgRadio->isChecked()){
            controllerSystem = new TimeConstantGain("",numeratorEdit, denominatorEdit,kv,retv);
        }else if (ui->polynomialRadio->isChecked()){
            controllerSystem = new PolynomialForm("", numeratorEdit, denominatorEdit,kv,retv);
        }else{
            controllerSystem = new FreeForm("", numeratorEdit, denominatorEdit,kv,retv,
                                      ui->numeratorEdit->text(), ui->denominatorEdit->text());
        }
    }else{
        std::optional<std::vector<Parameter>> nume = buildParameters(valueTable->at(0));
        std::optional<std::vector<Parameter>> deno = buildParameters(valueTable->at(1));

        if (!nume.has_value() || !deno.has_value()){
            releaseTables(valueTable, expressionTable, uncertainTable);
            errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
            return;
        }

        if (ui->zpkRadio->isChecked()){
            controllerSystem = new ZeroPoleGain("", *nume, *deno, kv, retv);
        }else if(ui->tcgRadio->isChecked()){
            controllerSystem = new TimeConstantGain("", *nume, *deno, kv, retv);
        }else if (ui->polynomialRadio->isChecked()){
            controllerSystem = new PolynomialForm("", *nume, *deno, kv, retv);
        }else {
            controllerSystem = new FreeForm("", *nume, *deno, kv, retv,
                                      ui->numeratorEdit->text(), ui->denominatorEdit->text());
        }


    }
    releaseTables(valueTable, expressionTable, uncertainTable);

    todoCorrecto = true;

    this->close();
}


std::optional<std::vector<Parameter>> ControllerDialog::buildParameters(QVector <QString> * numeros){
    std::vector<Parameter> var;
    var.reserve(numeros->size());

    if (numeros->isEmpty()){
        return var;
    }

    foreach (const QString &string, *numeros) {
        p.SetExpr(string.toStdString());
        try {
            var.push_back(Parameter(p.Eval().GetFloat()));
        } catch (mup::ParserError &) {
            //An invalid coefficient used to become 0 here, silently: the
            //controller that got designed was not the one the user typed.
            //parseCoefficients only tokenises, so this is where it has to be
            //said.
            return std::nullopt;
        }
    }

    return var;
}

bool ControllerDialog::getTodoCorrecto(){
    return todoCorrecto;
}

LtiSystem * ControllerDialog::takeControllerStructure(){
    LtiSystem * built = controllerSystem;
    controllerSystem = nullptr;

    return built;
}
