#include "plant_dialog.h"
#include "src/core/text_tokens.h"
#include "ui_plant_dialog.h"

#include "GUI/error_message.h"
#include "src/core/math/expression_cache.h"
#include "GUI/plot_palette.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"

using namespace tools;

PlantDialog::PlantDialog(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::PlantDialog>())
{
    ui->setupUi(this);
    setWindowTitle(tr("Plant input"));
    
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
    //The uncertainty dialog is a Qt child of this one, so Qt frees it: the
    //controller dialog, which builds the same child, already relied on
    //that.
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

bool PlantDialog::parseCoefficients(CoefficientTable & tabla, QLineEdit *linea,
                                    CoefficientTable & expressionTable,
                                    UncertainTable & uncertainTable){

    CoefficientRow vec1 = qftbx::text::tokens(linea->text());
    CoefficientRow vec;
    UncertainRow vec2;

    if (linea->text().isEmpty()){
        vec1.append("1");
        vec2.append(false);
    } else{

        foreach (QString e, vec1) {

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
                                 tr("Plant input"));
                    return false;
                }

                if (!p.IsFunDefined(capture.toStdString())){
                    vec.append(capture);
                    capture = QString();
                    isUncertain = true;
                    break;
                }
                match = re.match(e);
                capture = match.captured(0);
                e.remove(capture);
            }

            vec2.append(isUncertain);

            if (!isUncertain){
                vec.append(e);
            }
        }
    }

    tabla.append(vec);
    uncertainTable.append(vec2);
    expressionTable.append(vec1);

    return true;
}

bool PlantDialog::parseScalar(CoefficientTable & tabla, QLineEdit *linea,
                              CoefficientTable & expressionTable,
                              UncertainTable & uncertainTable){


    QString aux = linea->text();
    aux = aux.trimmed();

    CoefficientRow vec1(1, aux);
    CoefficientRow vec;
    UncertainRow vec2;

    QRegularExpression re("[a-zA-Z]+");
    QRegularExpressionMatch match = re.match(aux);
    qint32 i = 0;
    QString capture = match.captured(i);

    bool isUncertain = false;

    while (!capture.isNull()){

        if (!p.IsFunDefined(capture.toStdString())){
            vec.append(capture);
            capture = QString();
            isUncertain = true;
            break;
        }
        match = re.match(aux);
        capture = match.captured(0);
        aux.remove(capture);
    }

    vec2.append(isUncertain);

    if (!isUncertain){
        vec.append(aux);
    }

    tabla.append(vec);
    expressionTable.append(vec1);
    uncertainTable.append(vec2);

    return true;
}

bool PlantDialog::parseFreeForm(QLineEdit * linea, CoefficientTable & tabla,
                                CoefficientTable & expressionTable,
                                UncertainTable & uncertainTable){

    CoefficientRow expressions;
    CoefficientRow values;
    UncertainRow flags;

    QString text = linea->text();

    QRegularExpression re("[a-zA-Z]+");
    QRegularExpressionMatch match = re.match(text);
    QString capture = match.captured(0);

    text.remove(capture);

    while (!capture.isNull()){

        if (!p.IsFunDefined(capture.toStdString()) && capture != "s"){

            expressions.append(capture);
            values.append(capture);
            flags.append(true);

            capture = QString();
        }
        match = re.match(text);
        capture = match.captured(0);
        text.remove(capture);
    }


    tabla.append(values);
    expressionTable.append(expressions);
    uncertainTable.append(flags);

    return true;
}


std::optional<CoefficientTable> PlantDialog::readTables(CoefficientTable & expressionTable,
                                                        UncertainTable & uncertainTable){

    CoefficientTable tables;

    //Every parse result used to be discarded and the check below was
    //commented out, so a rejected coefficient - a reserved parameter name,
    //say - was only noticed much later, as a generic evaluation failure.
    //&& on the RIGHT so every parse still runs and every problem is reported.
    bool valid = true;

    if (ui->polynomialRadio->isChecked()){
        valid = parseCoefficients(tables, ui->polyNumerator, expressionTable, uncertainTable) && valid;
        valid = parseCoefficients(tables, ui->polyDenominator, expressionTable, uncertainTable) && valid;
        valid = parseScalar(tables, ui->polyGain, expressionTable, uncertainTable) && valid;
        valid = parseScalar(tables, ui->polyDelay, expressionTable, uncertainTable) && valid;
    }else if (ui->zpkRadio->isChecked()){
        valid = parseCoefficients(tables, ui->zpkNumerator, expressionTable, uncertainTable) && valid;
        valid = parseCoefficients(tables, ui->zpkDenominator, expressionTable, uncertainTable) && valid;
        valid = parseScalar(tables, ui->zpkGain, expressionTable, uncertainTable) && valid;
        valid = parseScalar(tables, ui->zpkDelay, expressionTable, uncertainTable) && valid;
    }else if (ui->tcgRadio->isChecked()){
        valid = parseCoefficients(tables, ui->tcgNumerator, expressionTable, uncertainTable) && valid;
        valid = parseCoefficients(tables, ui->tcgDenominator, expressionTable, uncertainTable) && valid;
        valid = parseScalar(tables, ui->tcgGain, expressionTable, uncertainTable) && valid;
        valid = parseScalar(tables, ui->tcgDelay, expressionTable, uncertainTable) && valid;
    }else if (ui->freeFormRadio->isChecked()){
        valid = parseFreeForm(ui->freeNumerator, tables, expressionTable, uncertainTable) && valid;
        valid = parseFreeForm(ui->freeDenominator, tables, expressionTable, uncertainTable) && valid;
        valid = parseScalar(tables, ui->freeGain, expressionTable, uncertainTable) && valid;
        valid = parseScalar(tables, ui->freeDelay, expressionTable, uncertainTable) && valid;
    }else{
        tables.clear();
        expressionTable.clear();
        uncertainTable.clear();
        return std::nullopt;
    }

    if (!valid){
        return std::nullopt;
    }

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

    CoefficientTable expressionTable;
    UncertainTable uncertainTable;
    const std::optional<CoefficientTable> valueTable = readTables(expressionTable, uncertainTable);

    if (!valueTable.has_value()){
        errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
        return;
    }

    Parameter kv;
    Parameter retv;

    //The expressions come from the user: a muParserX syntax error used to
    //throw and bring the application down.
    try {
        if (valueTable->at(2).size() == 0){
            kv = Parameter(1);
        }else{

            Range range_point = uncertaintyDialog->gain();
            p.SetExpr(expressionTable.at(2).at(0).toStdString());
            qreal d = p.Eval().GetFloat();

            if (d == range_point.min && d == range_point.max){
                kv = Parameter(d);
            }else {
                kv = Parameter("kv", range_point, d, "kv");
            }
        }

        if (valueTable->at(3).size() == 0){
            retv = Parameter(qreal(0));
        }else{

            Range range_point = uncertaintyDialog->delay();
            p.SetExpr(expressionTable.at(3).at(0).toStdString());
            qreal d = p.Eval().GetFloat();

            if (d == range_point.min && d == range_point.max){
                retv = Parameter(d);
            }else {
                retv = Parameter("ret", range_point, d, "ret");
            }
        }
    } catch (mup::ParserError &) {
        errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
        return;
    }

    //A second accept replaces the answer of the first one; whoever took it
    //already owns that one.
    plant.reset();

    //The uncertainty only counts if its dialog was ACCEPTED (opening and
    //cancelling used to leave the flag set and a half-built state in use).
    if (uncertaintyEntered && uncertaintyDialog->getTodoCorrecto()){
        //The plant receives COPIES: the uncertainty dialog keeps its own
        //parameters for further editing.
        std::vector<Parameter> nume = uncertaintyDialog->numerator();
        std::vector<Parameter> deno = uncertaintyDialog->denominator();

        if (ui->zpkRadio->isChecked()){
            plant = std::make_unique<ZeroPoleGain>(ui->nameEdit->text(),nume, deno,kv,retv);
        }else if(ui->tcgRadio->isChecked()){
            plant = std::make_unique<TimeConstantGain>(ui->nameEdit->text(),nume, deno,kv,retv);
        }else if (ui->polynomialRadio->isChecked()){
            plant = std::make_unique<PolynomialForm>(ui->nameEdit->text(), nume, deno,kv,retv);
        }else{
            plant = std::make_unique<FreeForm>(ui->nameEdit->text(), nume, deno,kv,retv,
                                      ui->freeNumerator->text(), ui->freeDenominator->text());
        }
    }else{
        std::optional<std::vector<Parameter>> nume = buildParameters(valueTable->at(0));
        std::optional<std::vector<Parameter>> deno = buildParameters(valueTable->at(1));

        if (!nume.has_value() || !deno.has_value()){
            errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
            return;
        }

        if (ui->zpkRadio->isChecked()){
            plant = std::make_unique<ZeroPoleGain>(ui->nameEdit->text(), *nume, *deno, kv, retv);
        }else if(ui->tcgRadio->isChecked()){
            plant = std::make_unique<TimeConstantGain>(ui->nameEdit->text(), *nume, *deno, kv, retv);
        }else if (ui->polynomialRadio->isChecked()){
            plant = std::make_unique<PolynomialForm>(ui->nameEdit->text(), *nume, *deno, kv, retv);
        }else {
            plant = std::make_unique<FreeForm>(ui->nameEdit->text(), *nume, *deno, kv, retv,
                                      ui->freeNumerator->text(), ui->freeDenominator->text());
        }


    }


    todoCorrecto = true;

    this->close();
}

std::optional<std::vector<Parameter>> PlantDialog::buildParameters(const CoefficientRow & numeros){
    std::vector<Parameter> var;

    if (numeros.isEmpty()){
        return var;
    }

    var.reserve(numeros.size());

    foreach (const QString &string, numeros) {
        p.SetExpr(string.toStdString());
        try {
            var.push_back(Parameter(p.Eval().GetFloat()));
        } catch (mup::ParserError &) {
            //An invalid coefficient used to become 0 here, silently: the
            //plant that got designed for was not the one the user typed.
            //readTables does not validate the coefficients either, so this
            //is where it has to be said.
            return std::nullopt;
        }
    }

    return var;
}

void PlantDialog::on_uncertaintyButton_clicked()
{
    CoefficientTable expressionTable;
    UncertainTable uncertainTable;
    std::optional<CoefficientTable> valueTable = readTables(expressionTable, uncertainTable);

    if (!valueTable.has_value()){
        errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
        return;
    }

    if (ui->nameEdit->text().isEmpty()){
        errorMessage(tr("The plant name is missing."), tr("Plant input"));
        ui->nameEdit->setStyleSheet("background : red");
        return;
    }else{
        ui->nameEdit->setStyleSheet("background : white");
    }


    uncertaintyDialog->launch(std::move(*valueTable), std::move(expressionTable),
                              std::move(uncertainTable), false);
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

std::unique_ptr<LtiSystem> PlantDialog::takePlant(){
    return std::move(plant);
}
