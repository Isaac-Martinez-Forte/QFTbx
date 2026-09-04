#include <cmath>
#include "src/gui/number_text.h"
#include "specifications_dialog.h"

#include "src/core/exception.h"
#include "src/core/text_tokens.h"

#include <vector>
#include <optional>

#include "src/core/specifications/specification.h"
#include "ui_specifications_dialog.h"

#include "src/gui/error_message.h"
#include "src/gui/plot_palette.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"

using namespace mup;

namespace qftbx {

namespace {

//The band is checked here, where it was typed. Specification::constant and
//fromSystem both refuse an inverted or non-finite band, but that throw only
//happens when the records are turned into specifications, which is when the
//BOUNDARIES are computed: the dialog accepted "start 10, end 1" without a
//word and the complaint arrived several steps later, naming no
//specification. Worse than the message was the silence when it did not
//throw at all - a band is only used through appliesAt(), which answers
//min <= omega && omega <= max, so an inverted one quietly applied to no
//frequency and the requirement did nothing.
bool bandIsUsable(double start, double end)
{
    return std::isfinite(start) && std::isfinite(end) &&
            start >= 0.0 && end >= start;
}

//A bound's magnitude, in linear units by the time it gets here. Same story
//as the band: Specification::constant refuses a non-finite or non-positive
//magnitude, but only when the records become specifications, which is when
//the boundaries are computed. And a NaN gets here easily - muParserX
//evaluates "0/0" quietly to one.
bool magnitudeIsUsable(double magnitude)
{
    return std::isfinite(magnitude) && magnitude > 0.0;
}

}

SpecificationsDialog::SpecificationsDialog(const std::vector<double> * frequencies,
                                           const qftbx::SpecificationRecords * loaded,
                                           QWidget *parent) :
    StepDialog(parent),
    m_reader(tr("Specifications input"))
{
    //The step order of the main window guarantees a frequency set here, but
    //an empty one used to reach first()/last() below and take the whole
    //application down instead of saying anything.
    //
    //This runs BEFORE the widget tree is built on purpose: a constructor
    //that throws gets no destructor, so anything allocated before the throw
    //would be lost.
    if (frequencies == nullptr || frequencies->empty()) {
        throw qftbx::InvalidInput("The design frequencies must be entered "
                                  "before the specifications.");
    }

    this->frequencies = frequencies;

    ui = std::make_unique<Ui::SpecificationsDialog>();
    ui->setupUi(this);

    setWindowTitle(tr("Specifications input"));

    //Plant figure images:

    QPixmap zpkPixmap (":/figures/kgan.png");
    ui->zpkImage->setPixmap(zpkPixmap);
    ui->lowerZpkImage->setPixmap(zpkPixmap);
    ui->upperZpkImage->setPixmap(zpkPixmap);

    QPixmap trackingImage (":/figures/knogan.png");
    ui->tcgImage->setPixmap(trackingImage);
    ui->lowerTcgImage->setPixmap(trackingImage);
    ui->upperTcgImage->setPixmap(trackingImage);

    QPixmap polyPixmap (":/figures/copol.png");
    ui->polyImage->setPixmap(polyPixmap);
    ui->lowerPolyImage->setPixmap(polyPixmap);
    ui->upperPolyImage->setPixmap(polyPixmap);

    activeTab = 0;

    ui->k->setText("1");
    ui->delayEdit->setText("0");

    //figureStack
    trackingImagePixmap = QPixmap (":/figures/seguimiento.png");
    controlEffortPixmap = QPixmap (":/figures/EC.png");
    outputDisturbancePixmap= QPixmap (":/figures/RPS.png");
    inputDisturbancePixmap= QPixmap (":/figures/RPE.png");
    sensorNoisePixmap= QPixmap (":/figures/ruidosensor.png");
    stabilityPixmap= QPixmap (":/figures/estabilidad.png");

    ui->startFrequencyEdit->setText(qftbx::numberText(frequencies->front()));
    ui->endFrequencyEdit->setText(qftbx::numberText(frequencies->back()));

    ui->trackingImage->setPixmap(trackingImagePixmap);

    //Default radio states (the .ui checks none): constant, polynomial form
    //and linear units. With no type checked, the reading cascade used to
    //fall into an accidental FreeForm.
    ui->constantRadio->setChecked(true);
    on_constantRadio_clicked();
    ui->linearRadio->setChecked(true);
    ui->lowerLinearRadio->setChecked(true);
    ui->upperLinearRadio->setChecked(true);
    ui->polynomialRadio->setChecked(true);
    on_polynomialRadio_clicked();
    ui->lowerPolynomialRadio->setChecked(true);
    on_lowerPolynomialRadio_clicked();
    ui->upperPolynomialRadio->setChecked(true);
    on_upperPolynomialRadio_clicked();

    //If the project carries specifications (a loaded file), the dialog
    //starts from THEM: it used to start from 7 empty records and the first
    //accept silently wiped whatever was loaded.
    if (loaded != nullptr){
        tracking = loaded->at(0).clone();
        trackingUpper = loaded->at(1).clone();
        stability = loaded->at(2).clone();
        sensorNoise = loaded->at(3).clone();
        outputDisturbance = loaded->at(4).clone();
        inputDisturbance = loaded->at(5).clone();
        controlEffort = loaded->at(6).clone();
    }

    //Tracking tab selected and restored from startup: without this no
    //radio was checked and accept published the 7 empty records.
    ui->trackingRadio->setChecked(true);
    on_trackingRadio_clicked();

}

SpecificationsDialog::~SpecificationsDialog()
{
    //The 7 working records (and their plants) are members, and so is
    //anything published but never taken: nothing to free by hand.
}

//Nominal coefficients in the format buildParameters expects (space
//separated). The non-FreeForm types have no textual representation of
//their own: numeratorString()=="" used to be painted and the
//specification silently vanished on reopen.
QString SpecificationsDialog::coefficientsText(std::vector<Parameter> & parameters)
{
    QString text;
    for (Parameter & parameter : parameters) {
        text += qftbx::numberText(parameter.nominal()) + " ";
    }
    return text.trimmed();
}

QString SpecificationsDialog::numeratorText(LtiSystem * system)
{
    if (system->type() == LtiSystem::SystemType::FreeForm){
        return QString::fromStdString(system->numeratorString());
    }
    return coefficientsText(system->numerator());
}

QString SpecificationsDialog::denominatorText(LtiSystem * system)
{
    if (system->type() == LtiSystem::SystemType::FreeForm){
        return QString::fromStdString(system->denominatorString());
    }
    return coefficientsText(system->denominator());
}

void SpecificationsDialog::setData(qftbx::SpecificationRecord & record)
{
    if (record.used){

        ui->startFrequencyEdit->setText(qftbx::numberText(record.omegaStart));
        ui->endFrequencyEdit->setText(qftbx::numberText(record.omegaEnd));

        if (record.constant){
            ui->constantRadio->setChecked(true);
            on_constantRadio_clicked();
            //The stored magnitude is linear: painted as-is with the linear
            //radio checked so accept does not reread it as dB.
            ui->linearRadio->setChecked(true);
            ui->magnitudeEdit->setText(qftbx::numberText(record.height));
        } else{
            ui->systemRadio->setChecked(true);
            on_systemRadio_clicked();

            switch (record.system->type()){
            case LtiSystem::SystemType::ZeroPoleGain:
                ui->zpkRadio->setChecked(true);
                on_zpkRadio_clicked();
                break;
            case LtiSystem::SystemType::TimeConstantGain:
                ui->tcgRadio->setChecked(true);
                on_tcgRadio_clicked();
                break;
            case LtiSystem::SystemType::PolynomialForm:
                ui->polynomialRadio->setChecked(true);
                on_polynomialRadio_clicked();
                break;
            default:
                ui->freeFormRadio->setChecked(true);
                on_freeFormRadio_clicked();
                break;
            }

            ui->numeratorEdit->setText(numeratorText(record.system.get()));
            ui->denominatorEdit->setText(denominatorText(record.system.get()));
            ui->k->setText(qftbx::numberText(record.system->gain().nominal()));
            ui->delayEdit->setText(qftbx::numberText(record.system->delay().nominal()));
        }
    } else {
        //The band too: it used to keep the PREVIOUS tab's values, and an
        //empty band means the whole design range, so the user could save a
        //band they never chose for this specification.
        ui->startFrequencyEdit->setText("");
        ui->endFrequencyEdit->setText("");
        ui->magnitudeEdit->setText("");
        ui->numeratorEdit->setText("");
        ui->denominatorEdit->setText("");
        ui->k->setText("1");
        ui->delayEdit->setText("0");
    }
}


void SpecificationsDialog::setData(qftbx::SpecificationRecord & record,
                                    qftbx::SpecificationRecord & upperRecord){

    if (record.used){

        ui->startFrequencyEdit->setText(qftbx::numberText(record.omegaStart));
        ui->endFrequencyEdit->setText(qftbx::numberText(record.omegaEnd));

        if (record.constant){
            ui->constantRadio->setChecked(true);
            on_constantRadio_clicked();
            ui->lowerLinearRadio->setChecked(true);
            ui->upperLinearRadio->setChecked(true);
            ui->lowerMagnitudeEdit->setText(qftbx::numberText(record.height));

            ui->upperMagnitudeEdit->setText(qftbx::numberText(upperRecord.height));
        }else{
            ui->systemRadio->setChecked(true);
            on_systemRadio_clicked();

            switch (record.system->type()){
            case LtiSystem::SystemType::ZeroPoleGain:
                ui->lowerZpkRadio->setChecked(true);
                on_lowerZpkRadio_clicked();
                break;
            case LtiSystem::SystemType::TimeConstantGain:
                ui->lowerTcgRadio->setChecked(true);
                on_lowerTcgRadio_clicked();
                break;
            case LtiSystem::SystemType::PolynomialForm:
                ui->lowerPolynomialRadio->setChecked(true);
                on_lowerPolynomialRadio_clicked();
                break;
            default:
                ui->lowerFreeFormRadio->setChecked(true);
                on_lowerFreeFormRadio_clicked();
                break;
            }

            switch (upperRecord.system->type()){
            case LtiSystem::SystemType::ZeroPoleGain:
                ui->upperZpkRadio->setChecked(true);
                on_upperZpkRadio_clicked();
                break;
            case LtiSystem::SystemType::TimeConstantGain:
                ui->upperTcgRadio->setChecked(true);
                on_upperTcgRadio_clicked();
                break;
            case LtiSystem::SystemType::PolynomialForm:
                ui->upperPolynomialRadio->setChecked(true);
                on_upperPolynomialRadio_clicked();
                break;
            default:
                ui->upperFreeFormRadio->setChecked(true);
                on_upperFreeFormRadio_clicked();
                break;
            }

            ui->lowerNumeratorEdit->setText(numeratorText(record.system.get()));
            ui->lowerDenominatorEdit->setText(denominatorText(record.system.get()));
            ui->lowerGainEdit->setText(qftbx::numberText(record.system->gain().nominal()));
            ui->lowerDelayEdit->setText(qftbx::numberText(record.system->delay().nominal()));

            ui->upperNumeratorEdit->setText(numeratorText(upperRecord.system.get()));
            ui->upperDenominatorEdit->setText(denominatorText(upperRecord.system.get()));
            ui->upperGainEdit->setText(qftbx::numberText(upperRecord.system->gain().nominal()));
            ui->upperDelayEdit->setText(qftbx::numberText(upperRecord.system->delay().nominal()));
        }
    } else {
        //The band too, for the same reason as the single specifications: an
        //empty band means the whole design range.
        ui->startFrequencyEdit->setText("");
        ui->endFrequencyEdit->setText("");

        ui->lowerMagnitudeEdit->setText("");
        ui->lowerNumeratorEdit->setText("");
        ui->lowerDenominatorEdit->setText("");
        ui->lowerGainEdit->setText("1");
        ui->lowerDelayEdit->setText("0");

        ui->upperMagnitudeEdit->setText("");
        ui->upperNumeratorEdit->setText("");
        ui->upperDenominatorEdit->setText("");
        ui->upperGainEdit->setText("1");
        ui->upperDelayEdit->setText("0");
    }
}

bool SpecificationsDialog::data(qftbx::SpecificationRecord & record, QString name)
{

    if (record.used && !record.constant){
        //The record's system must end up null: when this read finishes as not-used,
        //the clone() on accept used to clone a dangling pointer.
        record.system.reset();
        record.constant = false;
        record.used = false;
    }

    if (ui->startFrequencyEdit->text().isEmpty()){
        record.omegaStart = frequencies->front();
    } else {
        ParserX p (pckALL_NON_COMPLEX);
        p.SetExpr(ui->startFrequencyEdit->text().toStdString());

        try {
            record.omegaStart = p.Eval().GetFloat();
            ui->startFrequencyEdit->setStyleSheet("background : white");
        }catch (ParserError &){
            record.used = false;
            ui->startFrequencyEdit->setStyleSheet("background : red");
            errorMessage(tr("Invalid frequency band."), tr("Specifications input"));
            return false;
        }
    }

    if (ui->endFrequencyEdit->text().isEmpty()){
        record.omegaEnd = frequencies->back();
    } else {
        ParserX p (pckALL_NON_COMPLEX);
        p.SetExpr(ui->endFrequencyEdit->text().toStdString());

        try {
            record.omegaEnd = p.Eval().GetFloat();
            ui->endFrequencyEdit->setStyleSheet("background : white");
        }catch (ParserError &){
            record.used = false;
            ui->endFrequencyEdit->setStyleSheet("background : red");
            errorMessage(tr("Invalid frequency band."), tr("Specifications input"));
            return false;
        }
    }

    if (!bandIsUsable(record.omegaStart, record.omegaEnd)){
        ui->startFrequencyEdit->setStyleSheet("background : red");
        ui->endFrequencyEdit->setStyleSheet("background : red");
        record.used = false;
        errorMessage(tr("The frequency band needs 0 <= start <= end."),
                     tr("Specifications input"));
        return false;
    }

    if (ui->constantRadio->isChecked()){

        if (!ui->magnitudeEdit->text().isEmpty()){

            ParserX p (pckALL_NON_COMPLEX);
            p.SetExpr(ui->magnitudeEdit->text().toStdString());

            record.constant = true;

            try {

                qreal entered = p.Eval().GetFloat();

                if (ui->decibelsRadio->isChecked()){
                    record.height = qftbx::dbToLinear(entered);
                }else {
                    record.height = entered;
                }

                if (!magnitudeIsUsable(record.height)){
                    record.used = false;
                    ui->magnitudeEdit->setStyleSheet("background : red");
                    errorMessage(tr("The magnitude must be a finite number, "
                                    "and positive in linear units."),
                                 tr("Specifications input"));
                    return false;
                }

                ui->magnitudeEdit->setStyleSheet("background : white");
            }catch (ParserError &){
                record.used = false;
                ui->magnitudeEdit->setStyleSheet("background : red");
                errorMessage(tr("Invalid magnitude value."), tr("Specifications input"));
                return false;
            }

            ui->magnitudeEdit->setStyleSheet("background : white");
            record.used = true;
        } else {
            record.used = false;
        }
    }else {

        switch (readSystemFields(record, name, {ui->numeratorEdit, ui->denominatorEdit, ui->k, ui->delayEdit,
                                                ui->freeFormRadio, ui->zpkRadio, ui->tcgRadio, ui->polynomialRadio})) {
        case SystemRead::Failed:
            return false;
        case SystemRead::Unused:
            return true;
        case SystemRead::Read:
            break;
        }
    }

    record.name = name.toStdString();

    return true;
}

bool SpecificationsDialog::data(qftbx::SpecificationRecord & record,
                                    qftbx::SpecificationRecord & upperRecord, QString name){

    if (record.used && !record.constant){
        record.system.reset();
        record.constant = false;
        record.used = false;
    }

    if (upperRecord.used && !upperRecord.constant){
        upperRecord.system.reset();
        upperRecord.constant = false;
        upperRecord.used = false;
    }

    if (ui->startFrequencyEdit->text().isEmpty()){
        record.omegaStart = frequencies->front();
        upperRecord.omegaStart = frequencies->front();
    } else {
        ParserX p (pckALL_NON_COMPLEX);
        p.SetExpr(ui->startFrequencyEdit->text().toStdString());

        try {
            record.omegaStart = p.Eval().GetFloat();
            upperRecord.omegaStart = record.omegaStart;
            ui->startFrequencyEdit->setStyleSheet("background : white");
        }catch (ParserError &){
            record.used = false;
            upperRecord.used = false;
            ui->startFrequencyEdit->setStyleSheet("background : red");
            errorMessage(tr("Invalid frequency band."), tr("Specifications input"));
            return false;
        }
    }

    if (ui->endFrequencyEdit->text().isEmpty()){
        record.omegaEnd = frequencies->back();
        upperRecord.omegaEnd = frequencies->back();
    } else {
        ParserX p (pckALL_NON_COMPLEX);
        p.SetExpr(ui->endFrequencyEdit->text().toStdString());

        try {
            record.omegaEnd = p.Eval().GetFloat();
            upperRecord.omegaEnd = record.omegaEnd;
            ui->endFrequencyEdit->setStyleSheet("background : white");
        }catch (ParserError &){
            record.used = false;
            upperRecord.used = false;
            ui->endFrequencyEdit->setStyleSheet("background : red");
            errorMessage(tr("Invalid frequency band."), tr("Specifications input"));
            return false;
        }
    }

    if (!bandIsUsable(record.omegaStart, record.omegaEnd)){
        ui->startFrequencyEdit->setStyleSheet("background : red");
        ui->endFrequencyEdit->setStyleSheet("background : red");
        record.used = false;
        upperRecord.used = false;
        errorMessage(tr("The frequency band needs 0 <= start <= end."),
                     tr("Specifications input"));
        return false;
    }

    if (ui->constantRadio->isChecked()){

        if (!ui->lowerMagnitudeEdit->text().isEmpty()){

            ParserX p (pckALL_NON_COMPLEX);
            p.SetExpr(ui->lowerMagnitudeEdit->text().toStdString());

            record.constant = true;

            try {

                qreal entered = p.Eval().GetFloat();

                //The record's height is a LINEAR magnitude: this path used to
                //have both branches swapped relative to the simple path.
                if (ui->lowerDecibelsRadio->isChecked()){
                    record.height = qftbx::dbToLinear(entered);
                }else {
                    record.height = entered;
                }

                if (!magnitudeIsUsable(record.height)){
                    record.used = false;
                    upperRecord.used = false;
                    ui->lowerMagnitudeEdit->setStyleSheet("background : red");
                    errorMessage(tr("The lower magnitude must be a finite "
                                    "number, and positive in linear units."),
                                 tr("Specifications input"));
                    return false;
                }

                ui->lowerMagnitudeEdit->setStyleSheet("background : white");
            }catch (ParserError &){
                record.used = false;
                ui->lowerMagnitudeEdit->setStyleSheet("background : red");
                errorMessage(tr("Invalid magnitude value."), tr("Specifications input"));
                return false;
            }

            ui->lowerMagnitudeEdit->setStyleSheet("background : white");
            record.used = true;
        } else {
            record.used = false;
        }
    }else {

        switch (readSystemFields(record, name, {ui->lowerNumeratorEdit, ui->lowerDenominatorEdit, ui->lowerGainEdit, ui->lowerDelayEdit,
                                                ui->lowerFreeFormRadio, ui->lowerZpkRadio, ui->lowerTcgRadio, ui->lowerPolynomialRadio})) {
        case SystemRead::Failed:
            return false;
        case SystemRead::Unused:
            return true;
        case SystemRead::Read:
            break;
        }
    }


    if (ui->constantRadio->isChecked()){

        if (!ui->upperMagnitudeEdit->text().isEmpty()){

            ParserX p (pckALL_NON_COMPLEX);
            p.SetExpr(ui->upperMagnitudeEdit->text().toStdString());

            upperRecord.constant = true;

            try {

                qreal entered = p.Eval().GetFloat();

                if (ui->upperDecibelsRadio->isChecked()){
                    upperRecord.height = qftbx::dbToLinear(entered);
                }else {
                    upperRecord.height = entered;
                }

                if (!magnitudeIsUsable(upperRecord.height)){
                    record.used = false;
                    upperRecord.used = false;
                    ui->upperMagnitudeEdit->setStyleSheet("background : red");
                    errorMessage(tr("The upper magnitude must be a finite "
                                    "number, and positive in linear units."),
                                 tr("Specifications input"));
                    return false;
                }

                ui->upperMagnitudeEdit->setStyleSheet("background : white");
            }catch (ParserError &){
                upperRecord.used = false;
                ui->upperMagnitudeEdit->setStyleSheet("background : red");
                errorMessage(tr("Invalid magnitude value."), tr("Specifications input"));
                return false;
            }

            ui->upperMagnitudeEdit->setStyleSheet("background : white");
            upperRecord.used = true;
        } else {
            upperRecord.used = false;
        }
    }else {

        switch (readSystemFields(upperRecord, name, {ui->upperNumeratorEdit, ui->upperDenominatorEdit, ui->upperGainEdit, ui->upperDelayEdit,
                                                     ui->upperFreeFormRadio, ui->upperZpkRadio, ui->upperTcgRadio, ui->upperPolynomialRadio})) {
        case SystemRead::Failed:
            return false;
        case SystemRead::Unused:
            return true;
        case SystemRead::Read:
            break;
        }
    }

    record.name = name.toStdString();
    upperRecord.name = "TrackingUpper";

    return true;
}

std::optional<Parameter> SpecificationsDialog::scalarFrom(const QString & text, double fallback)
{
    if (text.isEmpty()) {
        return Parameter(fallback);
    }

    const std::optional<double> value = m_reader.evaluate(text);
    if (!value.has_value()) {
        return std::nullopt;
    }

    try {
        return Parameter(*value);
    } catch (const qftbx::Exception &) {
        //Parses, but is not a finite number a model can use.
        return std::nullopt;
    }
}

std::optional<std::vector<Parameter>> SpecificationsDialog::parametersFrom(const QString & text)
{
    CoefficientRow numbers;
    for (const std::string & token : qftbx::text::tokens(text.toStdString())) {
        numbers.push_back(QString::fromStdString(token));
    }

    return m_reader.buildParameters(numbers);
}

//The system a tab describes, read once for the three tabs that take one.
//Gain and delay are ALWAYS validated: the free-form branch used to build
//the FreeForm from unchecked results (nullptr on a syntax error, crashing
//later).
SpecificationsDialog::SystemRead SpecificationsDialog::readSystemFields(qftbx::SpecificationRecord & record,
                                                                        const QString & name,
                                                                        const SystemFields & fields)
{
    if (fields.denominator->text().isEmpty()) {
        record.used = false;
        return SystemRead::Unused;
    }

    const auto mark = [](QLineEdit * field, bool valid) {
        field->setStyleSheet(valid ? "background : white" : "background : red");
    };

    const auto refuse = [&](QLineEdit * field, const QString & complaint) {
        errorMessage(complaint, tr("Specifications input"));
        mark(field, false);
        record.used = false;
        return SystemRead::Failed;
    };

    const std::optional<Parameter> gain = scalarFrom(fields.gain->text(), 1.0);
    if (!gain.has_value()) {
        return refuse(fields.gain, tr("Invalid gain."));
    }
    mark(fields.gain, true);

    const std::optional<Parameter> delay = scalarFrom(fields.delay->text(), 0.0);
    if (!delay.has_value()) {
        return refuse(fields.delay, tr("Invalid delay."));
    }
    mark(fields.delay, true);

    LtiSystem::SystemType type = LtiSystem::SystemType::FreeForm;
    if (fields.zpk->isChecked()) {
        type = LtiSystem::SystemType::ZeroPoleGain;
    } else if (fields.tcg->isChecked()) {
        type = LtiSystem::SystemType::TimeConstantGain;
    } else if (fields.polynomial->isChecked()) {
        type = LtiSystem::SystemType::PolynomialForm;
    }

    //A free-form record carries its expressions and no coefficient vectors.
    std::vector<Parameter> numerator;
    std::vector<Parameter> denominator;

    if (type != LtiSystem::SystemType::FreeForm) {
        std::optional<std::vector<Parameter>> readNumerator = parametersFrom(fields.numerator->text());
        if (!readNumerator.has_value()) {
            return refuse(fields.numerator, tr("Invalid numerator."));
        }
        mark(fields.numerator, true);

        std::optional<std::vector<Parameter>> readDenominator = parametersFrom(fields.denominator->text());
        if (!readDenominator.has_value()) {
            return refuse(fields.denominator, tr("Invalid denominator."));
        }
        mark(fields.denominator, true);

        numerator = std::move(*readNumerator);
        denominator = std::move(*readDenominator);
    }

    record.constant = false;
    record.system = SystemDescriptionReader::makeSystem(type, name.toStdString(),
                                                        std::move(numerator), std::move(denominator),
                                                        *gain, *delay,
                                                        fields.numerator->text().toStdString(),
                                                        fields.denominator->text().toStdString());
    record.used = true;

    return SystemRead::Read;
}

bool SpecificationsDialog::saveActiveTab()
{
    if (activeTab == 1){
        return data(tracking, trackingUpper, "TrackingLower");
    }else if (activeTab == 2){
        return data(stability, "Stability");
    }else if (activeTab == 3){
        return data(sensorNoise, "SensorNoise");
    }else if (activeTab == 4){
        return data(outputDisturbance, "OutputDisturbance");
    }else if (activeTab == 5){
        return data(inputDisturbance, "InputDisturbance");
    }else if (activeTab == 6){
        return data(controlEffort, "ControlEffort");
    }

    //No tab selected yet: the constructor's first switch has nothing to save.
    return true;
}

void SpecificationsDialog::restoreActiveTabRadio()
{
    switch (activeTab){
    case 1: ui->trackingRadio->setChecked(true); break;
    case 2: ui->stabilityRadio->setChecked(true); break;
    case 3: ui->noiseRadio->setChecked(true); break;
    case 4: ui->outputDisturbanceRadio->setChecked(true); break;
    case 5: ui->inputDisturbanceRadio->setChecked(true); break;
    case 6: ui->controlEffortRadio->setChecked(true); break;
    default: break;
    }
}

bool SpecificationsDialog::leaveActiveTab()
{
    if (saveActiveTab()){
        return true;
    }

    //The switch used to happen anyway, and the return value was discarded.
    //data() empties the record before rebuilding it, so leaving now
    //lost the whole specification - not just the bad field - and coming
    //back showed a blank tab, because setData() repaints from the record.
    //Staying put keeps the user's text on screen where it can be fixed.
    restoreActiveTabRadio();

    errorMessage(tr("This specification could not be read, so it has not been "
                    "saved. Correct the field marked in red, or empty it to "
                    "leave the specification unused."),
                 tr("Specifications input"));

    return false;
}

void SpecificationsDialog::on_trackingRadio_clicked()
{
    if (!leaveActiveTab()){
        return;
    }

    ui->pageStack->setCurrentIndex(1);
    activeTab = 1;
    setData(tracking, trackingUpper);
    this->resize(867, 363);
    ui->buttonsWidget->move(670, 320);
}

void SpecificationsDialog::on_stabilityRadio_clicked()
{
    if (!leaveActiveTab()){
        return;
    }

    ui->pageStack->setCurrentIndex(0);
    activeTab = 2;
    setData(stability);
    ui->specificationImage->setPixmap(stabilityPixmap);
    this->resize(647, 363);
    ui->buttonsWidget->move(450, 320);
}

void SpecificationsDialog::on_noiseRadio_clicked()
{
    if (!leaveActiveTab()){
        return;
    }

    ui->pageStack->setCurrentIndex(0);
    activeTab = 3;
    setData(sensorNoise);
    ui->specificationImage->setPixmap(sensorNoisePixmap);
    this->resize(647, 363);
    ui->buttonsWidget->move(450, 320);
}

void SpecificationsDialog::on_outputDisturbanceRadio_clicked()
{
    if (!leaveActiveTab()){
        return;
    }

    ui->pageStack->setCurrentIndex(0);
    activeTab = 4;
    setData(outputDisturbance);
    ui->specificationImage->setPixmap(outputDisturbancePixmap);
    this->resize(647, 363);
    //The only one of the six that did not move the buttons back: coming from
    //the tracking tab, which widens the window and puts them at x = 670,
    //they landed outside the 647 this resize leaves.
    ui->buttonsWidget->move(450, 320);
}

void SpecificationsDialog::on_inputDisturbanceRadio_clicked()
{
    if (!leaveActiveTab()){
        return;
    }

    ui->pageStack->setCurrentIndex(0);
    activeTab = 5;
    setData(inputDisturbance);
    ui->specificationImage->setPixmap(inputDisturbancePixmap);
    this->resize(647, 363);
    ui->buttonsWidget->move(450, 320);
}

void SpecificationsDialog::on_controlEffortRadio_clicked()
{
    if (!leaveActiveTab()){
        return;
    }

    ui->pageStack->setCurrentIndex(0);
    activeTab = 6;
    setData(controlEffort);
    ui->specificationImage->setPixmap(controlEffortPixmap);
    this->resize(647, 363);
    ui->buttonsWidget->move(450, 320);
}

void SpecificationsDialog::on_constantRadio_clicked()
{
    ui->modeStack->setCurrentIndex(1);
    ui->lowerModeStack->setCurrentIndex(1);
    ui->upperModeStack->setCurrentIndex(1);
}

void SpecificationsDialog::on_systemRadio_clicked()
{
    ui->modeStack->setCurrentIndex(2);
    ui->lowerModeStack->setCurrentIndex(2);
    ui->upperModeStack->setCurrentIndex(2);
}

void SpecificationsDialog::on_cancelButton_clicked()
{
    close();
}

void SpecificationsDialog::on_okButton_clicked()
{
    //A rejected accept must not leave the previous answer behind, and the
    //vector used to leak on every accept.
    discardPublished();

    bool ok = true;

    if (ui->trackingRadio->isChecked()){
        ok = data(tracking, trackingUpper, "TrackingLower");
    }else if (ui->stabilityRadio->isChecked()){
        ok = data(stability, "Stability");
    }else if (ui->noiseRadio->isChecked()){
        ok = data(sensorNoise, "SensorNoise");
    }else if (ui->outputDisturbanceRadio->isChecked()){
        ok = data(outputDisturbance, "OutputDisturbance");
    }else if (ui->inputDisturbanceRadio->isChecked()){
        ok = data(inputDisturbance, "InputDisturbance");
    }else if (ui->controlEffortRadio->isChecked()){
        ok =  data(controlEffort, "ControlEffort");
    }

    if (!ok){
        return;
    }

    //The project takes ownership: it receives deep clones and the dialog
    //keeps its originals for further editing.
    published = qftbx::SpecificationRecords{tracking.clone(), trackingUpper.clone(),
            stability.clone(), sensorNoise.clone(), outputDisturbance.clone(),
            inputDisturbance.clone(), controlEffort.clone()};

    markAccepted();

    close();
}

void SpecificationsDialog::setFrequencies(const std::vector<double> * frequencies)
{
    if (frequencies == nullptr || frequencies->empty()) {
        throw qftbx::InvalidInput("The design frequencies must be entered "
                                  "before the specifications.");
    }

    this->frequencies = frequencies;
}

std::optional<qftbx::SpecificationRecords> SpecificationsDialog::takeSpecifications(){
    return std::move(published);
}

void SpecificationsDialog::discardPublished(){
    published.reset();
}


void SpecificationsDialog::on_lowerPolynomialRadio_clicked()
{
    ui->lowerFigureStack-> setCurrentIndex(3);
}

void SpecificationsDialog::on_lowerFreeFormRadio_clicked()
{
    ui->lowerFigureStack-> setCurrentIndex(0);
}

void SpecificationsDialog::on_lowerZpkRadio_clicked()
{
    ui->lowerFigureStack-> setCurrentIndex(1);
}

void SpecificationsDialog::on_lowerTcgRadio_clicked()
{
    ui->lowerFigureStack-> setCurrentIndex(2);
}

void SpecificationsDialog::on_upperPolynomialRadio_clicked()
{
    ui->upperFigureStack-> setCurrentIndex(3);
}

void SpecificationsDialog::on_upperZpkRadio_clicked()
{
    ui->upperFigureStack-> setCurrentIndex(1);
}

void SpecificationsDialog::on_upperTcgRadio_clicked()
{
    ui->upperFigureStack-> setCurrentIndex(2);
}

void SpecificationsDialog::on_upperFreeFormRadio_clicked()
{
    ui->upperFigureStack-> setCurrentIndex(0);
}

void SpecificationsDialog::on_polynomialRadio_clicked()
{
    ui->figureStack->setCurrentIndex(1);
}

void SpecificationsDialog::on_tcgRadio_clicked()
{
    ui->figureStack->setCurrentIndex(3);
}

void SpecificationsDialog::on_freeFormRadio_clicked()
{
    ui->figureStack->setCurrentIndex(0);
}

void SpecificationsDialog::on_zpkRadio_clicked()
{
    ui->figureStack->setCurrentIndex(2);
}

} // namespace qftbx
