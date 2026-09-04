#include "plant_dialog.h"
#include "src/core/text_tokens.h"
#include "ui_plant_dialog.h"

#include "src/core/exception.h"
#include "src/gui/error_message.h"
#include "src/core/math/expression_cache.h"
#include "src/gui/plot_palette.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"


namespace qftbx {

PlantDialog::PlantDialog(QWidget *parent) :
    StepDialog(parent),
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


    //The uncertainty dialog is created up front and reused.
    uncertaintyDialog = new UncertaintyDialog (this);

    //Plant figure images:

    QPixmap zpkPixmap (":/figures/kgan.png");
    ui->zpkImage->setPixmap(zpkPixmap);

    QPixmap tcgPixmap (":/figures/knogan.png");
    ui->tcgImage->setPixmap(tcgPixmap);

    QPixmap polyPixmap (":/figures/copol.png");
    ui->polyImage->setPixmap(polyPixmap);

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

bool PlantDialog::parseCoefficients(CoefficientTable & table, QLineEdit *field,
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
                                 tr("Plant input"));
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

bool PlantDialog::parseScalar(CoefficientTable & table, QLineEdit *field,
                              CoefficientTable & expressionTable,
                              UncertainTable & uncertainTable){


    QString text = field->text();
    text = text.trimmed();

    CoefficientRow expressions(1, text);
    CoefficientRow vec;
    UncertainRow uncertainFlags;

    QRegularExpression re("[a-zA-Z]+");
    QRegularExpressionMatch match = re.match(text);
    qint32 i = 0;
    QString capture = match.captured(i);

    bool isUncertain = false;

    while (!capture.isNull()){

        if (!p.IsFunDefined(capture.toStdString())){
            vec.push_back(capture);
            capture = QString();
            isUncertain = true;
            break;
        }
        match = re.match(text);
        capture = match.captured(0);
        text.remove(capture);
    }

    uncertainFlags.push_back(isUncertain);

    if (!isUncertain){
        vec.push_back(text);
    }

    table.push_back(vec);
    expressionTable.push_back(expressions);
    uncertainTable.push_back(uncertainFlags);

    return true;
}

bool PlantDialog::parseFreeForm(QLineEdit * field, CoefficientTable & table,
                                CoefficientTable & expressionTable,
                                UncertainTable & uncertainTable){

    CoefficientRow expressions;
    CoefficientRow values;
    UncertainRow flags;

    QString text = field->text();

    QRegularExpression re("[a-zA-Z]+");
    QRegularExpressionMatch match = re.match(text);
    QString capture = match.captured(0);

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
    Parameter delayParameter;

    //The expressions come from the user: a muParserX syntax error used to
    //throw and bring the application down.
    try {
        if (valueTable->at(2).size() == 0){
            kv = Parameter(1);
        }else{

            Range range = uncertaintyDialog->gain();
            p.SetExpr(expressionTable.at(2).at(0).toStdString());
            qreal d = p.Eval().GetFloat();

            if (d == range.min && d == range.max){
                kv = Parameter(d);
            }else {
                kv = Parameter("kv", range, d, "kv");
            }
        }

        if (valueTable->at(3).size() == 0){
            delayParameter = Parameter(qreal(0));
        }else{

            Range range = uncertaintyDialog->delay();
            p.SetExpr(expressionTable.at(3).at(0).toStdString());
            qreal d = p.Eval().GetFloat();

            if (d == range.min && d == range.max){
                delayParameter = Parameter(d);
            }else {
                delayParameter = Parameter("ret", range, d, "ret");
            }
        }
    } catch (mup::ParserError &) {
        errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
        return;
    } catch (const qftbx::Exception & e) {
        //And the same treatment for a value that parses but is not a number
        //a model can use: muParserX answers "0/0" and "1/0" with a NaN and
        //an infinity instead of complaining, and Parameter refuses those.
        errorMessage(e.what(), tr("Plant input"));
        return;
    }

    //A second accept replaces the answer of the first one; whoever took it
    //already owns that one.
    plant.reset();

    //The uncertainty only counts if its dialog was ACCEPTED (opening and
    //cancelling used to leave the flag set and a half-built state in use).
    if (uncertaintyEntered && uncertaintyDialog->wasAccepted()){
        //The plant receives COPIES: the uncertainty dialog keeps its own
        //parameters for further editing.
        std::vector<Parameter> numeratorParameters = uncertaintyDialog->numerator();
        std::vector<Parameter> denominatorParameters = uncertaintyDialog->denominator();

        if (ui->zpkRadio->isChecked()){
            plant = std::make_unique<ZeroPoleGain>(ui->nameEdit->text().toStdString(),numeratorParameters, denominatorParameters,kv,delayParameter);
        }else if(ui->tcgRadio->isChecked()){
            plant = std::make_unique<TimeConstantGain>(ui->nameEdit->text().toStdString(),numeratorParameters, denominatorParameters,kv,delayParameter);
        }else if (ui->polynomialRadio->isChecked()){
            plant = std::make_unique<PolynomialForm>(ui->nameEdit->text().toStdString(), numeratorParameters, denominatorParameters,kv,delayParameter);
        }else{
            plant = std::make_unique<FreeForm>(ui->nameEdit->text().toStdString(), numeratorParameters, denominatorParameters,kv,delayParameter,
                                      ui->freeNumerator->text().toStdString(), ui->freeDenominator->text().toStdString());
        }
    }else{
        std::optional<std::vector<Parameter>> numeratorParameters = buildParameters(valueTable->at(0));
        std::optional<std::vector<Parameter>> denominatorParameters = buildParameters(valueTable->at(1));

        if (!numeratorParameters.has_value() || !denominatorParameters.has_value()){
            errorMessage(tr("There is an error in the plant data"), tr("Plant input"));
            return;
        }

        if (ui->zpkRadio->isChecked()){
            plant = std::make_unique<ZeroPoleGain>(ui->nameEdit->text().toStdString(), *numeratorParameters, *denominatorParameters, kv, delayParameter);
        }else if(ui->tcgRadio->isChecked()){
            plant = std::make_unique<TimeConstantGain>(ui->nameEdit->text().toStdString(), *numeratorParameters, *denominatorParameters, kv, delayParameter);
        }else if (ui->polynomialRadio->isChecked()){
            plant = std::make_unique<PolynomialForm>(ui->nameEdit->text().toStdString(), *numeratorParameters, *denominatorParameters, kv, delayParameter);
        }else {
            plant = std::make_unique<FreeForm>(ui->nameEdit->text().toStdString(), *numeratorParameters, *denominatorParameters, kv, delayParameter,
                                      ui->freeNumerator->text().toStdString(), ui->freeDenominator->text().toStdString());
        }


    }


    markAccepted();

    this->close();
}

std::optional<std::vector<Parameter>> PlantDialog::buildParameters(const CoefficientRow & numbers){
    std::vector<Parameter> var;

    if (numbers.empty()){
        return var;
    }

    var.reserve(numbers.size());

    for (const QString &string : numbers) {
        p.SetExpr(string.toStdString());
        try {
            var.push_back(Parameter(p.Eval().GetFloat()));
        } catch (const qftbx::Exception &) {
            //A coefficient that is not a finite number: same answer.
            return std::nullopt;
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


std::unique_ptr<LtiSystem> PlantDialog::takePlant(){
    return std::move(plant);
}

} // namespace qftbx
