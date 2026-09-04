#include <cmath>
#include <QDoubleValidator>
#include "templates_dialog.h"
#include "src/core/math/sequences.h"
#include "src/core/text_tokens.h"
#include "ui_templates_dialog.h"

#include "src/gui/error_message.h"

#include <QMessageBox>
#include "src/gui/plot_palette.h"

using namespace tools;
using namespace mup;

namespace {

//A point count is read from an expression the user typed, so it can be
//anything muParserX accepts: fractional, negative, enormous, or not finite
//at all. Casting that straight to a size_t is what turned a "-5" into a
//request for 1.8e19 doubles: reserve() threw a length_error, which is not
//the mup::ParserError caught below, so the application went down instead
//of complaining. A non-finite value was worse - converting an infinity to
//an integer type is undefined behaviour.
//The ceiling is generous and arbitrary: a template grid needs hundreds of
//points, and a million already multiplies out to more plants than a sweep
//can hold. Truncation of a fractional count is what the implicit
//conversion did before, and it is kept, so every input that worked before
//still behaves the same - only the crashing ones changed.
constexpr double kMaxPointCount = 1.0e6;

bool asPointCount(double value, std::size_t & count)
{
    if (!std::isfinite(value) || value < 1.0 || value > kMaxPointCount) {
        return false;
    }
    count = static_cast<std::size_t>(value);
    return true;
}

}

TemplatesDialog::TemplatesDialog(QWidget *parent) :
    StepDialog(parent),
    ui(std::make_unique<Ui::TemplatesDialog>())
{
    ui->setupUi(this);
    ui->globalPointCount->setValidator(new QDoubleValidator(this));

    setWindowTitle(tr("Template input"));

    ui->globalPointCount->setText("10");

    //Wire the cancel button.
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
    connect (this, SIGNAL(close_ok()), this,SLOT(close()));

#ifndef CUDA_AVAILABLE
    ui->cudaCheck->setVisible(false);
#endif

    parser = std::make_unique<ParserX>(pckALL_NON_COMPLEX);

}

TemplatesDialog::~TemplatesDialog()
{
    clearTables();
    gridMap.clear();
}

//Variable rows: each ParLineEdit and its tab page belong to the dialog;
//they used to be abandoned with clear() and the pages piled up.
void TemplatesDialog::clearTables(){
    if (!rowsBuilt){
        return;
    }

    //Qt's own mechanism, and the only reason there is a delete here:
    //destroying the tab page is how the three line edits of a row leave the
    //dialog. The pages used to pile up.
    for (const ParLineEdit & par : numeratorRows){
        delete par.getX()->parentWidget();
    }
    numeratorRows.clear();

    for (const ParLineEdit & par : denominatorRows){
        delete par.getX()->parentWidget();
    }
    denominatorRows.clear();

    numeratorRadios.clear();
    denominatorRadios.clear();

    rowsBuilt = false;
}

//The grid map belongs to the dialog (the engine reads it without taking
//ownership): the old clear() leaked every computation's grids.
std::vector<double> TemplatesDialog::takeEpsilon(){
    return std::move(epsilonValues);
}

void TemplatesDialog::launch(LtiSystem *plant, qint32 frequencyCount){

    this->plant = plant;
    this->frequencyCount = frequencyCount;


    buildTables(plant->numerator(), plant->denominator());
}

void TemplatesDialog::buildTables(std::vector<Parameter> & numerator, std::vector<Parameter> & denominator){

    this->numerator = numerator;
    this->denominator = denominator;

    clearTables();


    rowsBuilt = true;


    qint32 tabIndex = 0;
    for (Parameter & variable : numerator){
        QString name_text = QString::fromStdString(variable.name());
        if(variable.isUncertain()){
            QWidget * widget = new QWidget(ui->variablesStack);
            buildRow(widget,numeratorRows, numeratorRadios);
            ui->numeratorTabs->insertTab(tabIndex,widget,name_text);
            tabIndex++;
        }
        ui->numeratorTabs->removeTab(tabIndex);
        ui->numeratorTabs->removeTab(tabIndex+1);
    }
    tabIndex = 0;
    for (Parameter & variable : denominator){
        QString name_text = QString::fromStdString(variable.name());
        if(variable.isUncertain()){
            QWidget * widget = new QWidget(ui->variablesStack);
            buildRow(widget, denominatorRows, denominatorRadios);
            ui->denominatorTabs->insertTab(tabIndex,widget, name_text);
            tabIndex++;
        }
        ui->denominatorTabs->removeTab(tabIndex);
        ui->denominatorTabs->removeTab(tabIndex+1);
    }
}

void TemplatesDialog::buildRow(QWidget *widget, QVector <ParLineEdit> & par,
                               QVector <ThreeRadioButtons> & rowRadios){

    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QRadioButton *rLin;
    QLineEdit *lin;
    QHBoxLayout *horizontalLayout_2;
    QRadioButton *rLog;
    QLineEdit *log;
    QHBoxLayout *horizontalLayout_3;
    QRadioButton *rManual;
    QLineEdit *manual;

    widget->setObjectName(QString::fromUtf8("widget"));
    widget->setGeometry(QRect(10, 10, 141, 86));
    verticalLayout = new QVBoxLayout(widget);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));

    rLin = new QRadioButton(widget);
    rLin->setObjectName(QString::fromUtf8("rLin"));

    horizontalLayout->addWidget(rLin);

    lin = new QLineEdit(widget);
    lin->setValidator(new QDoubleValidator(widget));
    lin->setObjectName(QString::fromUtf8("lin"));

    horizontalLayout->addWidget(lin);


    verticalLayout->addLayout(horizontalLayout);

    horizontalLayout_2 = new QHBoxLayout();
    horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
    rLog = new QRadioButton(widget);
    rLog->setObjectName(QString::fromUtf8("rLog"));

    horizontalLayout_2->addWidget(rLog);

    log = new QLineEdit(widget);
    log->setValidator(new QDoubleValidator(widget));
    log->setObjectName(QString::fromUtf8("log"));

    horizontalLayout_2->addWidget(log);


    verticalLayout->addLayout(horizontalLayout_2);

    horizontalLayout_3 = new QHBoxLayout();
    horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
    rManual = new QRadioButton(widget);
    rManual->setObjectName(QString::fromUtf8("rManual"));

    horizontalLayout_3->addWidget(rManual);

    manual = new QLineEdit(widget);
    manual->setObjectName(QString::fromUtf8("manual"));

    horizontalLayout_3->addWidget(manual);


    verticalLayout->addLayout(horizontalLayout_3);

    rLin->setText(QApplication::translate("Template", "LinSpace", 0));
    rLog->setText(QApplication::translate("Template", "LogSpace", 0));
    rManual->setText(QApplication::translate("Template", "Manual", 0));

    par.push_back(ParLineEdit(lin, log, manual));

    struct ThreeRadioButtons radio;
    radio.uno = rLin;
    radio.dos = rLog;
    radio.tres = rManual;

    rowRadios.push_back(radio);

}

void TemplatesDialog::on_allVariablesRadio_clicked()
{
    ui->modeStack->setCurrentIndex(0);
}

void TemplatesDialog::on_oneByOneRadio_clicked()
{
    ui->modeStack->setCurrentIndex(2);
}

void TemplatesDialog::on_numeratorRadio_clicked()
{
    ui->variablesStack->setCurrentIndex(1);
}

void TemplatesDialog::on_denominatorRadio_clicked()
{
    ui->variablesStack->setCurrentIndex(2);
}

void TemplatesDialog::on_cancelButton_clicked()
{
    emit (close_ok());
}
void TemplatesDialog::on_okButton_clicked()
{
    if (ui->nyquistRadio->isChecked())
        nicholsDiagram = false;
    else if (ui->nicholsRadio->isChecked())
        nicholsDiagram = true;

    //Direct read: the old latch left CUDA enabled forever once checked.
    cudaEnabled = ui->cudaCheck->isChecked();

    //The previous epsilon already belongs to the DAO; the previous grid
    //map is still the dialog's and is freed here.
    gridMap.clear();
    duplicateNames.clear();

    epsilonValues.clear();

    if (ui->epsilonEdit->text().isEmpty()){
        errorMessage(tr("No epsilonValues value was entered"), tr("Template computation"));
        ui->epsilonEdit->setStyleSheet("background : red");
        epsilonValues.clear();
        return;
    }else {

        ui->epsilonEdit->setStyleSheet("background : white");
        const std::vector<std::string> v = qftbx::text::tokens(ui->epsilonEdit->text().toStdString());

        qreal lastEpsilon = 0;

        qint32 counter = 0;

        //User expressions: an invalid epsilon used to throw and bring the
        //application down.
        try {
            for (const std::string & s : v) {
                parser->SetExpr(s);
                lastEpsilon = parser->Eval().GetFloat();
                epsilonValues.push_back(lastEpsilon);
                counter++;
            }
        } catch (mup::ParserError &) {
            errorMessage(tr("Invalid epsilonValues expression."), tr("Template computation"));
            ui->epsilonEdit->setStyleSheet("background : red");
            epsilonValues.clear();
            return;
        }

        for (; counter < frequencyCount; counter++){
            epsilonValues.push_back(lastEpsilon);
        }
    }

    bool useLinspace = false;
    bool useLogspace = false;

    gridMap.clear();

    if (ui->linspaceRadio->isChecked() && !ui->globalPointCount->text().isEmpty()){
        useLinspace = true;
    }else if (ui->logspaceRadio->isChecked() && !ui->globalPointCount->text().isEmpty()){
        useLogspace = true;
    }else {
        errorMessage(tr("ERROR: select logspace or linspace in the general section."), tr("Template computation"));
        gridMap.clear();
        epsilonValues.clear();
        return;
    }

    try {

    struct ThreeRadioButtons rowRadios;
    ParLineEdit rowEdits;
    qint32 variableIndex = 0;
    for (qint32 i = 0; i < static_cast<qint32>(numerator.size()); i++){
        Parameter & parameter = numerator[i];
        if (parameter.isUncertain()){
            rowEdits = numeratorRows.at(variableIndex);
            rowRadios = numeratorRadios.at(variableIndex);
            variableIndex++;
            if (!readVariable(rowEdits, rowRadios,parameter,useLinspace,useLogspace)){
                errorMessage(m_readReason.isEmpty()
                                 ? tr("ERROR: the values entered for parameter \"%1\" are invalid")
                                       .arg(QString::fromStdString(parameter.name()))
                                 : tr("ERROR: the values entered for parameter \"%1\" are invalid: %2.")
                                       .arg(QString::fromStdString(parameter.name()))
                                       .arg(m_readReason),
                         tr("Template computation"));
                gridMap.clear();
                epsilonValues.clear();
                return;
            }
        }
    }

    variableIndex = 0;

    for (qint32 i = 0; i < static_cast<qint32>(denominator.size()); i++){

        Parameter & parameter = denominator[i];
        if (parameter.isUncertain()){

            rowEdits = denominatorRows.at(variableIndex);
            rowRadios = denominatorRadios.at(variableIndex);
            variableIndex++;
            if (!readVariable(rowEdits, rowRadios,parameter,useLinspace,useLogspace)){
                errorMessage(m_readReason.isEmpty()
                                 ? tr("ERROR: the values entered for parameter \"%1\" are invalid")
                                       .arg(QString::fromStdString(parameter.name()))
                                 : tr("ERROR: the values entered for parameter \"%1\" are invalid: %2.")
                                       .arg(QString::fromStdString(parameter.name()))
                                       .arg(m_readReason),
                         tr("Template computation"));
                gridMap.clear();
                epsilonValues.clear();
                return;
            }
        }
    }

    if (!plant->gain().isUncertain()){
        gridMap[plant->gain().name()] = std::vector<double>(1, plant->gain().nominal());
    }
    else{

        const qreal inicio = plant->gain().range().min;
        const qreal final = plant->gain().range().max;
        parser->SetExpr(ui->globalPointCount->text().toStdString());

        std::size_t pointCount = 0;
        if (!asPointCount(parser->Eval().GetFloat(), pointCount)){
            errorMessage(tr("ERROR: the general point count must be a whole "
                            "number between 1 and %1.").arg(static_cast<qint64>(kMaxPointCount)),
                         tr("Template computation"));
            gridMap.clear();
            epsilonValues.clear();
            return;
        }

        if (useLinspace){
            gridMap[plant->gain().name()] = qftbx::math::linspace(inicio, final, pointCount);
        } else {
            gridMap[plant->gain().name()] = qftbx::math::logspace(inicio, final, pointCount);
        }
    }

    if (!plant->delay().isUncertain()){
        gridMap[plant->delay().name()] = std::vector<double>(1, plant->delay().nominal());
    }else {

        const qreal inicio = plant->delay().range().min;
        const qreal final = plant->delay().range().max;
        parser->SetExpr(ui->globalPointCount->text().toStdString());

        std::size_t pointCount = 0;
        if (!asPointCount(parser->Eval().GetFloat(), pointCount)){
            errorMessage(tr("ERROR: the general point count must be a whole "
                            "number between 1 and %1.").arg(static_cast<qint64>(kMaxPointCount)),
                         tr("Template computation"));
            gridMap.clear();
            epsilonValues.clear();
            return;
        }

        //This branch used to insert the delay grid under the GAIN's key:
        //it clobbered the gain's grid and left the delay without an entry
        //(crashing the sweep with an uncertain delay).
        if (useLinspace){
            gridMap[plant->delay().name()] = qftbx::math::linspace(inicio, final, pointCount);
        } else {
            gridMap[plant->delay().name()] = qftbx::math::logspace(inicio, final, pointCount);
        }
    }

    } catch (mup::ParserError &) {
        //Invalid point count or manual grid: it used to bring the
        //application down.
        errorMessage(tr("Invalid grid expressions."), tr("Template computation"));
        gridMap.clear();
        epsilonValues.clear();
        return;
    }

    if (!duplicateNames.empty()){
        QMessageBox::information(this, tr("Template computation"),
                tr("The parameter name(s) %1 appear more than once: the first "
                   "grid entered is used for every occurrence.")
                    .arg(duplicateNames.join(QStringLiteral(", "))));
    }

    markAccepted();
    emit (close_ok());
}

bool TemplatesDialog::readVariable(const ParLineEdit & rowEdits, ThreeRadioButtons rowRadios,
                                    Parameter & parameter, bool useLinspace, bool useLogspace){

    m_readReason.clear();

    //Policy for repeated names (e.g. the same 'a' in numerator and
    //denominator): the FIRST entered grid wins and the user is told once
    //which names were unified (with the name key, the last one would
    //silently win otherwise).
    if (gridMap.count(parameter.name()) != 0){
        if (!duplicateNames.contains(QString::fromStdString(parameter.name()))){
            duplicateNames.push_back(QString::fromStdString(parameter.name()));
        }
        return true;
    }

    qreal inicio;
    qreal final;
    std::size_t pointCount = 0;


    if (rowRadios.uno->isChecked() && !rowEdits.getX()->text().isEmpty()){

        inicio = parameter.range().min;
        final = parameter.range().max;

        parser->SetExpr(rowEdits.getX()->text().toStdString());
        if (!asPointCount(parser->Eval().GetFloat(), pointCount)){
            m_readReason = tr("its point count must be a whole number "
                              "between 1 and %1")
                    .arg(static_cast<qint64>(kMaxPointCount));
            return false;
        }

        gridMap[parameter.name()] = qftbx::math::linspace(inicio, final, pointCount);

    }else if (rowRadios.dos->isChecked() && !rowEdits.getY()->text().isEmpty()){

        inicio = parameter.range().min;
        final = parameter.range().max;
        parser->SetExpr(rowEdits.getY()->text().toStdString());
        if (!asPointCount(parser->Eval().GetFloat(), pointCount)){
            m_readReason = tr("its point count must be a whole number "
                              "between 1 and %1")
                    .arg(static_cast<qint64>(kMaxPointCount));
            return false;
        }

        gridMap[parameter.name()] = qftbx::math::logspace(inicio, final, pointCount);

    }else if(rowRadios.tres->isChecked() && !rowEdits.nominal()->text().isEmpty()){

        const std::vector<std::string> vector = qftbx::text::tokens(rowEdits.nominal()->text().toStdString());
        std::vector<double> values;
        values.reserve(static_cast<std::size_t>(vector.size()));

        for (const std::string & sSymbolCount : vector) {
            parser->SetExpr(sSymbolCount);
            const double value = parser->Eval().GetFloat();
            //A manual grid value may be negative - a parameter range can be -
            //but an infinity or a NaN would spread through every template
            //computed from it and only surface much later, as an empty plot.
            if (!std::isfinite(value)){
                m_readReason = tr("one of its grid values is not a finite "
                                  "number");
                return false;
            }
            values.push_back(value);
        }

        gridMap[parameter.name()] = std::move(values);
    }else if (useLinspace || useLogspace){

        inicio = parameter.range().min;
        final = parameter.range().max;

        parser->SetExpr(ui->globalPointCount->text().toStdString());
        if (!asPointCount(parser->Eval().GetFloat(), pointCount)){
            m_readReason = tr("its point count must be a whole number "
                              "between 1 and %1")
                    .arg(static_cast<qint64>(kMaxPointCount));
            return false;
        }

        if(useLinspace){
            gridMap[parameter.name()] = qftbx::math::linspace(inicio, final, pointCount);
        }else {
            gridMap[parameter.name()] = qftbx::math::logspace(inicio, final, pointCount);
        }
    }else{
        return false;
    }

    return true;
}

qftbx::ParameterGrids TemplatesDialog::grids() const{
    return gridMap;
}


bool TemplatesDialog::nicholsSelected(){
    return nicholsDiagram;
}

bool TemplatesDialog::cudaSelected(){
    return cudaEnabled;
}

