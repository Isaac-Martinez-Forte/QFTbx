#include "specifications_dialog.h"

#include "src/core/exception.h"
#include "src/core/text_tokens.h"

#include <optional>

#include "src/core/specifications/specification.h"
#include "ui_specifications_dialog.h"

#include "GUI/error_message.h"
#include "GUI/plot_palette.h"
#include "src/core/system/free_form.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"

using namespace tools;
using namespace mup;

SpecificationsDialog::SpecificationsDialog(const QVector<qreal> * frequencies,
                                           const QVector<qftbx::SpecificationRecord *> * loaded,
                                           QWidget *parent) :
    QDialog(parent)
{
    //The step order of the main window guarantees a frequency set here, but
    //an empty one used to reach first()/last() below and take the whole
    //application down instead of saying anything.
    //
    //This runs BEFORE the widget tree is built on purpose: a constructor
    //that throws gets no destructor, so anything allocated before the throw
    //would be lost.
    if (frequencies == nullptr || frequencies->isEmpty()) {
        throw qftbx::InvalidInput("The design frequencies must be entered "
                                  "before the specifications.");
    }

    this->frequencies = frequencies;

    ui = new Ui::SpecificationsDialog();
    ui->setupUi(this);

    setWindowTitle(tr("Specifications input"));

    published = nullptr;

    //Plant figure images:

    QPixmap imagen1 (":/figures/kgan.png");
    ui->zpkImage->setPixmap(imagen1);
    ui->lowerZpkImage->setPixmap(imagen1);
    ui->upperZpkImage->setPixmap(imagen1);

    QPixmap trackingImage (":/figures/knogan.png");
    ui->tcgImage->setPixmap(trackingImage);
    ui->lowerTcgImage->setPixmap(trackingImage);
    ui->upperTcgImage->setPixmap(trackingImage);

    QPixmap imagen3 (":/figures/copol.png");
    ui->polyImage->setPixmap(imagen3);
    ui->lowerPolyImage->setPixmap(imagen3);
    ui->upperPolyImage->setPixmap(imagen3);

    activeTab = 0;

    ui->k->setText("1");
    ui->delayEdit->setText("0");

    tracking = new qftbx::SpecificationRecord();
    trackingUpper = new qftbx::SpecificationRecord();
    stability = new qftbx::SpecificationRecord();
    sensorNoise = new qftbx::SpecificationRecord();
    outputDisturbance = new qftbx::SpecificationRecord();
    inputDisturbance = new qftbx::SpecificationRecord();
    controlEffort = new qftbx::SpecificationRecord();

    //figureStack
    trackingImagePixmap = QPixmap (":/figures/seguimiento.png");
    controlEffortPixmap = QPixmap (":/figures/EC.png");
    outputDisturbancePixmap= QPixmap (":/figures/RPS.png");
    inputDisturbancePixmap= QPixmap (":/figures/RPE.png");
    sensorNoisePixmap= QPixmap (":/figures/ruidosensor.png");
    stabilityPixmap= QPixmap (":/figures/estabilidad.png");

    ui->startFrequencyEdit->setText(QString::number(frequencies->first()));
    ui->endFrequencyEdit->setText(QString::number(frequencies->last()));

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
    if (loaded != nullptr && loaded->size() == 7){
        delete tracking;
        delete trackingUpper;
        delete stability;
        delete sensorNoise;
        delete outputDisturbance;
        delete inputDisturbance;
        delete controlEffort;

        tracking = loaded->at(0)->clone();
        trackingUpper = loaded->at(1)->clone();
        stability = loaded->at(2)->clone();
        sensorNoise = loaded->at(3)->clone();
        outputDisturbance = loaded->at(4)->clone();
        inputDisturbance = loaded->at(5)->clone();
        controlEffort = loaded->at(6)->clone();
    }

    //Tracking tab selected and restored from startup: without this no
    //radio was checked and accept published the 7 empty records.
    ui->trackingRadio->setChecked(true);
    on_trackingRadio_clicked();

    todoCorrecto = false;
}

SpecificationsDialog::~SpecificationsDialog()
{
    //The 7 working records (and their plants) belong to the dialog: the
    //DAO received deep clones.
    qftbx::SpecificationRecord * records[] = {tracking, trackingUpper, stability,
                          sensorNoise, outputDisturbance, inputDisturbance, controlEffort};
    for (qftbx::SpecificationRecord * record : records) {
        delete record->system;
        delete record;
    }

    //Published but not taken (cancelled afterwards, or never asked for):
    //the clones are the dialog's until somebody claims them.
    discardPublished();

    delete ui;
}

//Nominal coefficients in the format buildParameters expects (space
//separated). The non-FreeForm types have no textual representation of
//their own: numeratorString()=="" used to be painted and the
//specification silently vanished on reopen.
QString SpecificationsDialog::coefficientsText(std::vector<Parameter> & parametros)
{
    QString texto;
    for (Parameter & parametro : parametros) {
        texto += QString::number(parametro.nominal()) + " ";
    }
    return texto.trimmed();
}

QString SpecificationsDialog::numeratorText(LtiSystem * sistema)
{
    if (sistema->type() == LtiSystem::SystemType::FreeForm){
        return sistema->numeratorString();
    }
    return coefficientsText(sistema->numerator());
}

QString SpecificationsDialog::denominatorText(LtiSystem * sistema)
{
    if (sistema->type() == LtiSystem::SystemType::FreeForm){
        return sistema->denominatorString();
    }
    return coefficientsText(sistema->denominator());
}

void SpecificationsDialog::setDatos(qftbx::SpecificationRecord * record_in)
{
    if (record_in->used){

        ui->startFrequencyEdit->setText(QString::number(record_in->omegaStart));
        ui->endFrequencyEdit->setText(QString::number(record_in->omegaEnd));

        if (record_in->constant){
            ui->constantRadio->setChecked(true);
            on_constantRadio_clicked();
            //The stored magnitude is linear: painted as-is with the linear
            //radio checked so accept does not reread it as dB.
            ui->linearRadio->setChecked(true);
            ui->magnitudeEdit->setText(QString::number(record_in->height));
        } else{
            ui->systemRadio->setChecked(true);
            on_systemRadio_clicked();

            switch (record_in->system->type()){
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

            ui->numeratorEdit->setText(numeratorText(record_in->system));
            ui->denominatorEdit->setText(denominatorText(record_in->system));
            ui->k->setText(QString::number(record_in->system->gain().nominal()));
            ui->delayEdit->setText(QString::number(record_in->system->delay().nominal()));
        }
    } else {
        ui->magnitudeEdit->setText("");
        ui->numeratorEdit->setText("");
        ui->denominatorEdit->setText("");
        ui->k->setText("1");
        ui->delayEdit->setText("0");
    }
}


void SpecificationsDialog::setDatos(qftbx::SpecificationRecord *record_in, qftbx::SpecificationRecord *upperRecord){

    if (record_in->used){

        ui->startFrequencyEdit->setText(QString::number(record_in->omegaStart));
        ui->endFrequencyEdit->setText(QString::number(record_in->omegaEnd));

        if (record_in->constant){
            ui->constantRadio->setChecked(true);
            on_constantRadio_clicked();
            ui->lowerLinearRadio->setChecked(true);
            ui->upperLinearRadio->setChecked(true);
            ui->lowerMagnitudeEdit->setText(QString::number(record_in->height));

            ui->upperMagnitudeEdit->setText(QString::number(upperRecord->height));
        }else{
            ui->systemRadio->setChecked(true);
            on_systemRadio_clicked();

            switch (record_in->system->type()){
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

            switch (upperRecord->system->type()){
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

            ui->lowerNumeratorEdit->setText(numeratorText(record_in->system));
            ui->lowerDenominatorEdit->setText(denominatorText(record_in->system));
            ui->lowerGainEdit->setText(QString::number(record_in->system->gain().nominal()));
            ui->lowerDelayEdit->setText(QString::number(record_in->system->delay().nominal()));

            ui->upperNumeratorEdit->setText(numeratorText(upperRecord->system));
            ui->upperDenominatorEdit->setText(denominatorText(upperRecord->system));
            ui->upperGainEdit->setText(QString::number(upperRecord->system->gain().nominal()));
            ui->upperDelayEdit->setText(QString::number(upperRecord->system->delay().nominal()));
        }
    } else {
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

bool SpecificationsDialog::getDatos(qftbx::SpecificationRecord * record_in, QString name_in)
{

    if (record_in->used && !record_in->constant){
        //sistema must end up null: when this read finishes as not-used,
        //the clone() on accept used to clone a dangling pointer.
        delete record_in->system;
        record_in->system = nullptr;
        record_in->constant = false;
        record_in->used = false;
    }

    if (ui->startFrequencyEdit->text().isEmpty()){
        record_in->omegaStart = frequencies->first();
    } else {
        ParserX p (pckALL_NON_COMPLEX);
        p.SetExpr(ui->startFrequencyEdit->text().toStdString());

        try {
            record_in->omegaStart = p.Eval().GetFloat();
            ui->startFrequencyEdit->setStyleSheet("background : white");
        }catch (ParserError &e){
            record_in->used = false;
            ui->startFrequencyEdit->setStyleSheet("background : red");
            errorMessage(tr("Invalid frequency band."), tr("Specifications input"));
            return false;
        }
    }

    if (ui->endFrequencyEdit->text().isEmpty()){
        record_in->omegaEnd = frequencies->last();
    } else {
        ParserX p (pckALL_NON_COMPLEX);
        p.SetExpr(ui->endFrequencyEdit->text().toStdString());

        try {
            record_in->omegaEnd = p.Eval().GetFloat();
            ui->endFrequencyEdit->setStyleSheet("background : white");
        }catch (ParserError &e){
            record_in->used = false;
            ui->endFrequencyEdit->setStyleSheet("background : red");
            errorMessage(tr("Invalid frequency band."), tr("Specifications input"));
            return false;
        }
    }

    if (ui->constantRadio->isChecked()){

        if (!ui->magnitudeEdit->text().isEmpty()){

            ParserX p (pckALL_NON_COMPLEX);
            p.SetExpr(ui->magnitudeEdit->text().toStdString());

            record_in->constant = true;

            try {

                qreal alt = p.Eval().GetFloat();

                if (ui->decibelsRadio->isChecked()){
                    record_in->height = qftbx::dbToLinear(alt);
                }else {
                    record_in->height = alt;
                }

                ui->magnitudeEdit->setStyleSheet("background : white");
            }catch (ParserError &e){
                record_in->used = false;
                ui->magnitudeEdit->setStyleSheet("background : red");
                errorMessage(tr("Invalid magnitude value."), tr("Specifications input"));
                return false;
            }

            ui->magnitudeEdit->setStyleSheet("background : white");
            record_in->used = true;
        } else {
            record_in->used = false;
        }
    }else {

        if (ui->denominatorEdit->text().isEmpty()){
            record_in->used = false;
            return true;
        }

        std::optional<std::vector<Parameter>> numeratorEdit;
        std::optional<std::vector<Parameter>> denominatorEdit;

        //Gain and delay are ALWAYS validated: the free-form branch used to
        //build the FreeForm from unchecked buildScalar results (nullptr on a
        //syntax error, crashing later).
        std::optional<Parameter> k = buildScalar(ui->k->text(), true);

        if (!k.has_value()){
            errorMessage(tr("Invalid gain."), tr("Specifications input"));
            ui->k->setStyleSheet("background : red");
            record_in->used = false;
            return false;
        }
        ui->k->setStyleSheet("background : white");

        std::optional<Parameter> delayEdit = buildScalar(ui->delayEdit->text(), false);

        if (!delayEdit.has_value()){
            errorMessage(tr("Invalid delay."), tr("Specifications input"));
            ui->delayEdit->setStyleSheet("background : red");
            record_in->used = false;
            return false;
        }
        ui->delayEdit->setStyleSheet("background : white");

        if (!ui->freeFormRadio->isChecked()){

            numeratorEdit = buildParameters(ui->numeratorEdit->text());

            if (!numeratorEdit.has_value()){
                errorMessage(tr("Invalid numerator."), tr("Specifications input"));
                ui->numeratorEdit->setStyleSheet("background : red");
                record_in->used = false;
                return false;
            }
            ui->numeratorEdit->setStyleSheet("background : white");

            denominatorEdit = buildParameters(ui->denominatorEdit->text());

            if (!denominatorEdit.has_value()){
                errorMessage(tr("Invalid denominator."), tr("Specifications input"));
                ui->denominatorEdit->setStyleSheet("background : red");
                record_in->used = false;
                return false;
            }
            ui->denominatorEdit->setStyleSheet("background : white");
        }


        record_in->constant = false;
        if(ui->zpkRadio->isChecked()){
            record_in->system = new ZeroPoleGain (name_in, std::move(*numeratorEdit), std::move(*denominatorEdit), std::move(*k), std::move(*delayEdit));
        }else if (ui->tcgRadio->isChecked()){
            record_in->system = new TimeConstantGain (name_in, std::move(*numeratorEdit), std::move(*denominatorEdit), std::move(*k), std::move(*delayEdit));
        }else if (ui->polynomialRadio->isChecked()) {
            record_in->system = new PolynomialForm (name_in, std::move(*numeratorEdit), std::move(*denominatorEdit), std::move(*k), std::move(*delayEdit));
        }else {
            //No type radio checked: the free-form record carries its
            //expressions and no coefficient vectors (whatever was parsed
            //above simply goes out of scope now).
            record_in->system = new FreeForm (name_in, {}, {}, std::move(*k), std::move(*delayEdit),
                                              ui->numeratorEdit->text(), ui->denominatorEdit->text());
        }
        record_in->used = true;
    }

    record_in->name = name_in;

    return true;
}

bool SpecificationsDialog::getDatos(qftbx::SpecificationRecord *record_in, qftbx::SpecificationRecord *upperRecord, QString name_in){

    if (record_in->used && !record_in->constant){
        delete record_in->system;
        record_in->system = nullptr;
        record_in->constant = false;
        record_in->used = false;
    }

    if (upperRecord->used && !upperRecord->constant){
        delete upperRecord->system;
        upperRecord->system = nullptr;
        upperRecord->constant = false;
        upperRecord->used = false;
    }

    if (ui->startFrequencyEdit->text().isEmpty()){
        record_in->omegaStart = frequencies->first();
        upperRecord->omegaStart = frequencies->first();
    } else {
        ParserX p (pckALL_NON_COMPLEX);
        p.SetExpr(ui->startFrequencyEdit->text().toStdString());

        try {
            record_in->omegaStart = p.Eval().GetFloat();
            upperRecord->omegaStart = record_in->omegaStart;
            ui->startFrequencyEdit->setStyleSheet("background : white");
        }catch (ParserError &e){
            record_in->used = false;
            upperRecord->used = false;
            ui->startFrequencyEdit->setStyleSheet("background : red");
            errorMessage(tr("Invalid frequency band."), tr("Specifications input"));
            return false;
        }
    }

    if (ui->endFrequencyEdit->text().isEmpty()){
        record_in->omegaEnd = frequencies->last();
        upperRecord->omegaEnd = frequencies->last();
    } else {
        ParserX p (pckALL_NON_COMPLEX);
        p.SetExpr(ui->endFrequencyEdit->text().toStdString());

        try {
            record_in->omegaEnd = p.Eval().GetFloat();
            upperRecord->omegaEnd = record_in->omegaEnd;
            ui->endFrequencyEdit->setStyleSheet("background : white");
        }catch (ParserError &e){
            record_in->used = false;
            upperRecord->used = false;
            ui->endFrequencyEdit->setStyleSheet("background : red");
            errorMessage(tr("Invalid frequency band."), tr("Specifications input"));
            return false;
        }
    }

    if (ui->constantRadio->isChecked()){

        if (!ui->lowerMagnitudeEdit->text().isEmpty()){

            ParserX p (pckALL_NON_COMPLEX);
            p.SetExpr(ui->lowerMagnitudeEdit->text().toStdString());

            record_in->constant = true;

            try {

                qreal alt = p.Eval().GetFloat();

                //qftbx::SpecificationRecord::altura is a LINEAR magnitude: this path used to have
                //both branches swapped relative to the simple path.
                if (ui->lowerDecibelsRadio->isChecked()){
                    record_in->height = qftbx::dbToLinear(alt);
                }else {
                    record_in->height = alt;
                }

                ui->lowerMagnitudeEdit->setStyleSheet("background : white");
            }catch (ParserError &e){
                record_in->used = false;
                ui->lowerMagnitudeEdit->setStyleSheet("background : red");
                errorMessage(tr("Invalid magnitude value."), tr("Specifications input"));
                return false;
            }

            ui->lowerMagnitudeEdit->setStyleSheet("background : white");
            record_in->used = true;
        } else {
            record_in->used = false;
        }
    }else {

        if (ui->lowerDenominatorEdit->text().isEmpty()){
            record_in->used = false;
            return true;
        }

        std::optional<std::vector<Parameter>> numeratorEdit;
        std::optional<std::vector<Parameter>> denominatorEdit;

        //Gain and delay are ALWAYS validated: the free-form branch used to
        //build the FreeForm from unchecked buildScalar results (nullptr on a
        //syntax error, crashing later).
        std::optional<Parameter> k = buildScalar(ui->lowerGainEdit->text(), true);

        if (!k.has_value()){
            errorMessage(tr("Invalid gain."), tr("Specifications input"));
            ui->lowerGainEdit->setStyleSheet("background : red");
            record_in->used = false;
            return false;
        }
        ui->lowerGainEdit->setStyleSheet("background : white");

        std::optional<Parameter> delayEdit = buildScalar(ui->lowerDelayEdit->text(), false);

        if (!delayEdit.has_value()){
            errorMessage(tr("Invalid delay."), tr("Specifications input"));
            ui->lowerDelayEdit->setStyleSheet("background : red");
            record_in->used = false;
            return false;
        }
        ui->lowerDelayEdit->setStyleSheet("background : white");

        if (!ui->lowerFreeFormRadio->isChecked()){

            numeratorEdit = buildParameters(ui->lowerNumeratorEdit->text());

            if (!numeratorEdit.has_value()){
                errorMessage(tr("Invalid numerator."), tr("Specifications input"));
                ui->lowerNumeratorEdit->setStyleSheet("background : red");
                record_in->used = false;
                return false;
            }
            ui->lowerNumeratorEdit->setStyleSheet("background : white");

            denominatorEdit = buildParameters(ui->lowerDenominatorEdit->text());

            if (!denominatorEdit.has_value()){
                errorMessage(tr("Invalid denominator."), tr("Specifications input"));
                ui->lowerDenominatorEdit->setStyleSheet("background : red");
                record_in->used = false;
                return false;
            }
            ui->lowerDenominatorEdit->setStyleSheet("background : white");
        }


        record_in->constant = false;
        if(ui->lowerZpkRadio->isChecked()){
            record_in->system = new ZeroPoleGain (name_in, std::move(*numeratorEdit), std::move(*denominatorEdit), std::move(*k), std::move(*delayEdit));
        }else if (ui->lowerTcgRadio->isChecked()){
            record_in->system = new TimeConstantGain (name_in, std::move(*numeratorEdit), std::move(*denominatorEdit), std::move(*k), std::move(*delayEdit));
        }else if (ui->lowerPolynomialRadio->isChecked()) {
            record_in->system = new PolynomialForm (name_in, std::move(*numeratorEdit), std::move(*denominatorEdit), std::move(*k), std::move(*delayEdit));
        }else {
            //No type radio checked: the free-form record carries its
            //expressions and no coefficient vectors.
            record_in->system = new FreeForm (name_in, {}, {}, std::move(*k), std::move(*delayEdit),
                                              ui->lowerNumeratorEdit->text(), ui->lowerDenominatorEdit->text());
        }
        record_in->used = true;
    }


    if (ui->constantRadio->isChecked()){

        if (!ui->upperMagnitudeEdit->text().isEmpty()){

            ParserX p (pckALL_NON_COMPLEX);
            p.SetExpr(ui->upperMagnitudeEdit->text().toStdString());

            upperRecord->constant = true;

            try {

                qreal alt = p.Eval().GetFloat();

                if (ui->upperDecibelsRadio->isChecked()){
                    upperRecord->height = qftbx::dbToLinear(alt);
                }else {
                    upperRecord->height = alt;
                }

                ui->upperMagnitudeEdit->setStyleSheet("background : white");
            }catch (ParserError &e){
                upperRecord->used = false;
                ui->upperMagnitudeEdit->setStyleSheet("background : red");
                errorMessage(tr("Invalid magnitude value."), tr("Specifications input"));
                return false;
            }

            ui->upperMagnitudeEdit->setStyleSheet("background : white");
            upperRecord->used = true;
        } else {
            upperRecord->used = false;
        }
    }else {

        if (ui->upperDenominatorEdit->text().isEmpty()){
            upperRecord->used = false;
            return true;
        }

        std::optional<std::vector<Parameter>> numeratorEdit;
        std::optional<std::vector<Parameter>> denominatorEdit;

        //Gain and delay are ALWAYS validated: the free-form branch used to
        //build the FreeForm from unchecked buildScalar results (nullptr on a
        //syntax error, crashing later).
        std::optional<Parameter> k = buildScalar(ui->upperGainEdit->text(), true);

        if (!k.has_value()){
            errorMessage(tr("Invalid gain."), tr("Specifications input"));
            ui->upperGainEdit->setStyleSheet("background : red");
            upperRecord->used = false;
            return false;
        }
        ui->upperGainEdit->setStyleSheet("background : white");

        std::optional<Parameter> delayEdit = buildScalar(ui->upperDelayEdit->text(), false);

        if (!delayEdit.has_value()){
            errorMessage(tr("Invalid delay."), tr("Specifications input"));
            ui->upperDelayEdit->setStyleSheet("background : red");
            upperRecord->used = false;
            return false;
        }
        ui->upperDelayEdit->setStyleSheet("background : white");

        if (!ui->upperFreeFormRadio->isChecked()){

            numeratorEdit = buildParameters(ui->upperNumeratorEdit->text());

            if (!numeratorEdit.has_value()){
                errorMessage(tr("Invalid numerator."), tr("Specifications input"));
                ui->upperNumeratorEdit->setStyleSheet("background : red");
                upperRecord->used = false;
                return false;
            }
            ui->upperNumeratorEdit->setStyleSheet("background : white");

            denominatorEdit = buildParameters(ui->upperDenominatorEdit->text());

            if (!denominatorEdit.has_value()){
                errorMessage(tr("Invalid denominator."), tr("Specifications input"));
                ui->upperDenominatorEdit->setStyleSheet("background : red");
                upperRecord->used = false;
                return false;
            }
            ui->upperDenominatorEdit->setStyleSheet("background : white");
        }

        upperRecord->constant = false;
        if(ui->upperZpkRadio->isChecked()){
            upperRecord->system = new ZeroPoleGain (name_in, std::move(*numeratorEdit), std::move(*denominatorEdit), std::move(*k), std::move(*delayEdit));
        }else if (ui->upperTcgRadio->isChecked()){
            upperRecord->system = new TimeConstantGain (name_in, std::move(*numeratorEdit), std::move(*denominatorEdit), std::move(*k), std::move(*delayEdit));
        }else if (ui->upperPolynomialRadio->isChecked()) {
            upperRecord->system = new PolynomialForm (name_in, std::move(*numeratorEdit), std::move(*denominatorEdit), std::move(*k), std::move(*delayEdit));
        }else {
            //No type radio checked: the free-form record carries its
            //expressions and no coefficient vectors.
            upperRecord->system = new FreeForm (name_in, {}, {}, std::move(*k), std::move(*delayEdit),
                                                ui->upperNumeratorEdit->text(), ui->upperDenominatorEdit->text());
        }
        upperRecord->used = true;
    }

    record_in->name = name_in;
    upperRecord->name = QStringLiteral("TrackingUpper");

    return true;
}

std::optional<Parameter> SpecificationsDialog::buildScalar(QString linea, bool isK){
    ParserX p (pckALL_NON_COMPLEX);

    if (linea.isEmpty()){
        if (isK){
            return Parameter(1);
        }else{
            return Parameter(qreal(0));
        }
    }else{
        qreal res;
        p.SetExpr(linea.toStdString());
        try {
            res = p.Eval().GetFloat();
        }catch (ParserError &e){
            return std::nullopt;
        }

        return Parameter(res);
    }
}

std::optional<std::vector<Parameter>> SpecificationsDialog::buildParameters(QString linea){

    ParserX p (pckALL_NON_COMPLEX);

    QVector <QString> * numeros = qftbx::text::tokens(linea);

    std::vector<Parameter> var;

    if (numeros->isEmpty()){
        delete numeros;
        return var;
    }

    var.reserve(numeros->size());

    foreach (const QString &string, *numeros) {
        p.SetExpr(string.toStdString());
        qreal res;
        try {
            res = p.Eval().GetFloat();
        }catch (ParserError &e){
            //The already-built Parameters and both vectors used to leak.
            delete numeros;
            return std::nullopt;
        }

        var.push_back(Parameter(res));
    }

    delete numeros;

    return var;
}

void SpecificationsDialog::saveActiveTab()
{
    if (activeTab == 1){
        getDatos(tracking, trackingUpper, "TrackingLower");
    }else if (activeTab == 2){
        getDatos(stability, "Stability");
    }else if (activeTab == 3){
        getDatos(sensorNoise, "SensorNoise");
    }else if (activeTab == 4){
        getDatos(outputDisturbance, "OutputDisturbance");
    }else if (activeTab == 5){
        getDatos(inputDisturbance, "InputDisturbance");
    }else if (activeTab == 6){
        getDatos(controlEffort, "ControlEffort");
    }
}

void SpecificationsDialog::on_trackingRadio_clicked()
{
    ui->pageStack->setCurrentIndex(1);
    saveActiveTab();
    activeTab = 1;
    setDatos(tracking, trackingUpper);
    this->resize(867, 363);
    ui->buttonsWidget->move(670, 320);
}

void SpecificationsDialog::on_stabilityRadio_clicked()
{
    ui->pageStack->setCurrentIndex(0);
    saveActiveTab();
    activeTab = 2;
    setDatos(stability);
    ui->specificationImage->setPixmap(stabilityPixmap);
    this->resize(647, 363);
    ui->buttonsWidget->move(450, 320);
}

void SpecificationsDialog::on_noiseRadio_clicked()
{
    ui->pageStack->setCurrentIndex(0);
    saveActiveTab();
    activeTab = 3;
    setDatos(sensorNoise);
    ui->specificationImage->setPixmap(sensorNoisePixmap);
    this->resize(647, 363);
    ui->buttonsWidget->move(450, 320);
}

void SpecificationsDialog::on_outputDisturbanceRadio_clicked()
{
    ui->pageStack->setCurrentIndex(0);
    saveActiveTab();
    activeTab = 4;
    setDatos(outputDisturbance);
    ui->specificationImage->setPixmap(outputDisturbancePixmap);
    this->resize(647, 363);
}

void SpecificationsDialog::on_inputDisturbanceRadio_clicked()
{
    ui->pageStack->setCurrentIndex(0);
    saveActiveTab();
    activeTab = 5;
    setDatos(inputDisturbance);
    ui->specificationImage->setPixmap(inputDisturbancePixmap);
    this->resize(647, 363);
    ui->buttonsWidget->move(450, 320);
}

void SpecificationsDialog::on_controlEffortRadio_clicked()
{
    ui->pageStack->setCurrentIndex(0);
    saveActiveTab();
    activeTab = 6;
    setDatos(controlEffort);
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
    emit(close());
}

void SpecificationsDialog::on_okButton_clicked()
{
    //A rejected accept must not leave the previous answer behind, and the
    //vector used to leak on every accept.
    discardPublished();

    bool correcto = true;

    if (ui->trackingRadio->isChecked()){
        correcto = getDatos(tracking, trackingUpper, "TrackingLower");
    }else if (ui->stabilityRadio->isChecked()){
        correcto = getDatos(stability, "Stability");
    }else if (ui->noiseRadio->isChecked()){
        correcto = getDatos(sensorNoise, "SensorNoise");
    }else if (ui->outputDisturbanceRadio->isChecked()){
        correcto = getDatos(outputDisturbance, "OutputDisturbance");
    }else if (ui->inputDisturbanceRadio->isChecked()){
        correcto = getDatos(inputDisturbance, "InputDisturbance");
    }else if (ui->controlEffortRadio->isChecked()){
        correcto =  getDatos(controlEffort, "ControlEffort");
    }

    if (!correcto){
        return;
    }

    published = new QVector <qftbx::SpecificationRecord *> ();

    //The DAO takes ownership: it receives deep clones and the dialog keeps
    //its originals for further editing.
    published->append(tracking->clone());
    published->append(trackingUpper->clone());
    published->append(stability->clone());
    published->append(sensorNoise->clone());
    published->append(outputDisturbance->clone());
    published->append(inputDisturbance->clone());
    published->append(controlEffort->clone());

    todoCorrecto = true;

    emit (close());
}

bool SpecificationsDialog::getTodoCorrecto(){
    return todoCorrecto;
}

QVector<qftbx::SpecificationRecord *> * SpecificationsDialog::takeSpecifications(){
    QVector<qftbx::SpecificationRecord *> * built = published;
    published = nullptr;

    return built;
}

void SpecificationsDialog::discardPublished(){
    if (published == nullptr){
        return;
    }

    for (qftbx::SpecificationRecord * record : *published) {
        delete record->system;
        delete record;
    }

    delete published;
    published = nullptr;
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

