#include <vector>
#include "controller_dialog.h"
#include "src/core/text_tokens.h"
#include "ui_controller_dialog.h"

#include "src/core/exception.h"
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
    StepDialog(parent),
    ui(std::make_unique<Ui::ControllerDialog>())
{
    ui->setupUi(this);


    setWindowTitle(tr("Controller structure input"));

    QPixmap zpkPixmap (":/figures/kgan.png");
    ui->zpkImage->setPixmap(zpkPixmap);

    QPixmap tcgPixmap (":/figures/knogan.png");
    ui->tcgImage->setPixmap(tcgPixmap);

    QPixmap polyPixmap (":/figures/copol.png");
    ui->polyImage->setPixmap(polyPixmap);

    ui->gainStart->setText("1");
    ui->gainEnd->setText("1");

    //The uncertainty dialog is created up front and reused.
    uncertaintyDialog = new UncertaintyDialog (this);

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

bool ControllerDialog::parseCoefficients(CoefficientTable & table, QLineEdit *field,
                                         CoefficientTable & expressionTable,
                                         UncertainTable & uncertainTable){

    CoefficientRow expressions;
    for (const std::string & token : qftbx::text::tokens(field->text().toStdString())) {
        expressions.push_back(QString::fromStdString(token));
    }
    CoefficientRow vec;
    UncertainRow uncertainFlags;

    if (field->text().isEmpty()){
        expressions.push_back("1");
        uncertainFlags.push_back(false);
    } else{

        for (QString e : expressions) {

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

            uncertainFlags.push_back(isUncertain);

            if (!isUncertain){
                vec.push_back(e);
            }
        }
    }

    table.push_back(vec);
    uncertainTable.push_back(uncertainFlags);
    expressionTable.push_back(expressions);

    return true;
}

bool ControllerDialog::parseGainRange(CoefficientTable & table, QLineEdit *startField, QLineEdit * endField,
                                      CoefficientTable & expressionTable,
                                      UncertainTable & uncertainTable){


    QString text = startField->text().trimmed();
    QString endText = endField->text().trimmed();

    CoefficientRow expressions;
    CoefficientRow vec;
    UncertainRow uncertainFlags;
    uncertainFlags.push_back(true);

    vec.push_back(text);
    vec.push_back(endText);

    expressions.push_back(text);
    expressions.push_back(endText);

    table.push_back(vec);
    expressionTable.push_back(expressions);
    uncertainTable.push_back(uncertainFlags);

    return true;
}

bool ControllerDialog::parseFreeForm(QLineEdit * field, CoefficientTable & table,
                                     CoefficientTable & expressionTable,
                                     UncertainTable & uncertainTable){

    CoefficientRow expressions;
    CoefficientRow values;
    UncertainRow flags;

    QString text = field->text();

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


    table.push_back(values);
    expressionTable.push_back(expressions);
    uncertainTable.push_back(flags);

    return true;
}


void ControllerDialog::on_cancelButton_clicked()
{
    close();
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
    Parameter delayParameter;

    //User expressions: a syntax error used to throw and bring the
    //application down.
    try {
        if (valueTable->at(2).size() == 0){
            kv = Parameter(1);
        }else{

            Range range;
            p.SetExpr(expressionTable.at(2).at(0).toStdString());
            range.min = p.Eval().GetFloat();

            p.SetExpr(expressionTable.at(2).at(1).toStdString());
            range.max = p.Eval().GetFloat();

            if (range.min == range.max){
                kv = Parameter(range.min);
            }else {

                if (range.min > range.max){
                    qreal a = range.min;
                    range.min = range.max;
                    range.max = a;
                }

                kv = Parameter("kv", range, (range.min + range.max) / 2);
            }
        }
    } catch (mup::ParserError &) {
        errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
        return;
    } catch (const qftbx::Exception & e) {
        //And the same treatment for a value that parses but is not a number
        //a model can use: muParserX answers "0/0" and "1/0" with a NaN and
        //an infinity instead of complaining, and Parameter refuses those.
        errorMessage(e.what(), tr("Controller input"));
        return;
    }

    delayParameter = Parameter(0.0);


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
            controllerSystem = std::make_unique<ZeroPoleGain>("",numeratorEdit, denominatorEdit,kv,delayParameter);
        }else if(ui->tcgRadio->isChecked()){
            controllerSystem = std::make_unique<TimeConstantGain>("",numeratorEdit, denominatorEdit,kv,delayParameter);
        }else if (ui->polynomialRadio->isChecked()){
            controllerSystem = std::make_unique<PolynomialForm>("", numeratorEdit, denominatorEdit,kv,delayParameter);
        }else{
            controllerSystem = std::make_unique<FreeForm>("", numeratorEdit, denominatorEdit,kv,delayParameter,
                                      ui->numeratorEdit->text().toStdString(), ui->denominatorEdit->text().toStdString());
        }
    }else{
        std::optional<std::vector<Parameter>> numeratorParameters = buildParameters(valueTable->at(0));
        std::optional<std::vector<Parameter>> denominatorParameters = buildParameters(valueTable->at(1));

        if (!numeratorParameters.has_value() || !denominatorParameters.has_value()){
            errorMessage(tr("There is an error in the controller data"), tr("Controller input"));
            return;
        }

        if (ui->zpkRadio->isChecked()){
            controllerSystem = std::make_unique<ZeroPoleGain>("", *numeratorParameters, *denominatorParameters, kv, delayParameter);
        }else if(ui->tcgRadio->isChecked()){
            controllerSystem = std::make_unique<TimeConstantGain>("", *numeratorParameters, *denominatorParameters, kv, delayParameter);
        }else if (ui->polynomialRadio->isChecked()){
            controllerSystem = std::make_unique<PolynomialForm>("", *numeratorParameters, *denominatorParameters, kv, delayParameter);
        }else {
            controllerSystem = std::make_unique<FreeForm>("", *numeratorParameters, *denominatorParameters, kv, delayParameter,
                                      ui->numeratorEdit->text().toStdString(), ui->denominatorEdit->text().toStdString());
        }


    }

    markAccepted();

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
        } catch (const qftbx::Exception &) {
            //A coefficient that is not a finite number: same answer.
            return std::nullopt;
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

std::unique_ptr<LtiSystem> ControllerDialog::takeControllerStructure(){
    return std::move(controllerSystem);
}
