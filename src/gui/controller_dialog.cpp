#include <vector>
#include "controller_dialog.h"
#include "src/core/text_tokens.h"
#include "ui_controller_dialog.h"

#include "src/gui/error_message.h"
#include "src/core/math/expression_cache.h"
#include "src/gui/plot_palette.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"

using namespace tools;
using namespace mup;

ControllerDialog::ControllerDialog(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::ControllerDialog>())
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

    accepted = false;
}

ControllerDialog::~ControllerDialog()
{
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
    CoefficientTable expressionTable;
    UncertainTable uncertainTable;
    std::optional<CoefficientTable> valueTable = readTables(expressionTable, uncertainTable);

    if (!valueTable.has_value()){
        errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
        return;
    }

    uncertaintyDialog->launch(std::move(*valueTable), std::move(expressionTable),
                              std::move(uncertainTable), true);
    uncertaintyDialog->show();
    uncertaintyEntered = true;
}

std::optional<CoefficientTable> ControllerDialog::readTables(CoefficientTable & expressionTable,
                                                             UncertainTable & uncertainTable){

    bool valid = true;
    CoefficientTable tables;
    tables.reserve(4);

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
        return std::nullopt;
    }

    return tables;
}

bool ControllerDialog::parseCoefficients(CoefficientTable & tabla, QLineEdit *linea,
                                         CoefficientTable & expressionTable,
                                         UncertainTable & uncertainTable){

    CoefficientRow vec1;
    for (const std::string & token : qftbx::text::tokens(linea->text().toStdString())) {
        vec1.push_back(QString::fromStdString(token));
    }
    CoefficientRow vec;
    UncertainRow vec2;

    if (linea->text().isEmpty()){
        vec1.push_back("1");
        vec2.push_back(false);
    } else{

        for (QString e : vec1) {

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
                if (qftbx::math::isReservedName(capture.toStdString())){
                    errorMessage(tr("\"%1\" cannot be used as a parameter name: "
                                    "the expression parser already defines it.").arg(capture),
                                 tr("Controller input"));
                    return false;
                }

                if (!p.IsFunDefined(capture.toStdString())){
                    vec.push_back(capture);
                    capture = QString();
                    isUncertain = true;
                    break;
                }
                match = re.match(e);
                capture = match.captured(0);
                e.remove(capture);
            }

            vec2.push_back(isUncertain);

            if (!isUncertain){
                vec.push_back(e);
            }
        }
    }

    tabla.push_back(vec);
    uncertainTable.push_back(vec2);
    expressionTable.push_back(vec1);

    return true;
}

bool ControllerDialog::parseGainRange(CoefficientTable & tabla, QLineEdit *linea1, QLineEdit * linea2,
                                      CoefficientTable & expressionTable,
                                      UncertainTable & uncertainTable){


    QString aux = linea1->text().trimmed();
    QString aux1 = linea2->text().trimmed();

    CoefficientRow vec1;
    CoefficientRow vec;
    UncertainRow vec2;
    vec2.push_back(true);

    vec.push_back(aux);
    vec.push_back(aux1);

    vec1.push_back(aux);
    vec1.push_back(aux1);

    tabla.push_back(vec);
    expressionTable.push_back(vec1);
    uncertainTable.push_back(vec2);

    return true;
}

bool ControllerDialog::parseFreeForm(QLineEdit * linea, CoefficientTable & tabla,
                                     CoefficientTable & expressionTable,
                                     UncertainTable & uncertainTable){

    CoefficientRow expressions;
    CoefficientRow values;
    UncertainRow flags;

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

            expressions.push_back(capture);
            values.push_back(capture);
            flags.push_back(true);

            capture = QString();
        }
        match = re.match(text);
        capture = match.captured(0);
        text.remove(capture);
    }


    tabla.push_back(values);
    expressionTable.push_back(expressions);
    uncertainTable.push_back(flags);

    return true;
}


void ControllerDialog::on_cancelButton_clicked()
{
    emit(close());
}

void ControllerDialog::on_okButton_clicked()
{

    CoefficientTable expressionTable;
    UncertainTable uncertainTable;
    const std::optional<CoefficientTable> valueTable = readTables(expressionTable, uncertainTable);

    if (!valueTable.has_value()){
        errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
        return;
    }


    Parameter kv;
    Parameter retv;

    //User expressions: a syntax error used to throw and bring the
    //application down.
    try {
        if (valueTable->at(2).size() == 0){
            kv = Parameter(1);
        }else{

            Range range_point;
            p.SetExpr(expressionTable.at(2).at(0).toStdString());
            range_point.min = p.Eval().GetFloat();

            p.SetExpr(expressionTable.at(2).at(1).toStdString());
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
        errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
        return;
    }

    retv = Parameter(0.0);


    //A second accept replaces the answer of the first one; whoever took it
    //already owns that one.
    controllerSystem.reset();

    //The uncertainty only counts if its dialog was ACCEPTED.
    if (uncertaintyEntered && uncertaintyDialog->wasAccepted()){
        //The controller receives COPIES: the uncertainty dialog keeps its
        //own parameters for further editing.
        std::vector<Parameter> numeratorEdit = uncertaintyDialog->numerator();
        std::vector<Parameter> denominatorEdit = uncertaintyDialog->denominator();

        if (ui->zpkRadio->isChecked()){
            controllerSystem = std::make_unique<ZeroPoleGain>("",numeratorEdit, denominatorEdit,kv,retv);
        }else if(ui->tcgRadio->isChecked()){
            controllerSystem = std::make_unique<TimeConstantGain>("",numeratorEdit, denominatorEdit,kv,retv);
        }else if (ui->polynomialRadio->isChecked()){
            controllerSystem = std::make_unique<PolynomialForm>("", numeratorEdit, denominatorEdit,kv,retv);
        }else{
            controllerSystem = std::make_unique<FreeForm>("", numeratorEdit, denominatorEdit,kv,retv,
                                      ui->numeratorEdit->text().toStdString(), ui->denominatorEdit->text().toStdString());
        }
    }else{
        std::optional<std::vector<Parameter>> nume = buildParameters(valueTable->at(0));
        std::optional<std::vector<Parameter>> deno = buildParameters(valueTable->at(1));

        if (!nume.has_value() || !deno.has_value()){
            errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
            return;
        }

        if (ui->zpkRadio->isChecked()){
            controllerSystem = std::make_unique<ZeroPoleGain>("", *nume, *deno, kv, retv);
        }else if(ui->tcgRadio->isChecked()){
            controllerSystem = std::make_unique<TimeConstantGain>("", *nume, *deno, kv, retv);
        }else if (ui->polynomialRadio->isChecked()){
            controllerSystem = std::make_unique<PolynomialForm>("", *nume, *deno, kv, retv);
        }else {
            controllerSystem = std::make_unique<FreeForm>("", *nume, *deno, kv, retv,
                                      ui->numeratorEdit->text().toStdString(), ui->denominatorEdit->text().toStdString());
        }


    }

    accepted = true;

    this->close();
}


std::optional<std::vector<Parameter>> ControllerDialog::buildParameters(const CoefficientRow & numbers){
    std::vector<Parameter> var;
    var.reserve(numbers.size());

    if (numbers.empty()){
        return var;
    }

    for (const QString &string : numbers) {
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

bool ControllerDialog::wasAccepted(){
    return accepted;
}

std::unique_ptr<LtiSystem> ControllerDialog::takeControllerStructure(){
    return std::move(controllerSystem);
}
