/**
 * @file
 * @brief Reads the parameter grids and the epsilons from the form.
 *
 * A point count comes from an expression, so it is accepted only when
 * finite, at least one and under the settings' ceiling, then truncated to
 * a whole number; a manual grid value may be negative but must be finite,
 * since a NaN would surface much later as an empty plot. A name entered in
 * both polynomials gets the first grid entered and the user is told once;
 * a certain gain or delay is a grid of its one nominal value. Fewer
 * epsilons than frequencies repeat the last one, every one positive and
 * finite, and the decibels-per-degree weight must be positive. The epsilon
 * field is prefilled from the proposer over the grids as they open, and
 * the border sweep needs exactly two distinct uncertain parameters.
 */

#include <algorithm>
#include <cmath>
#include "src/gui/common/number_text.h"
#include "src/gui/common/expression_field.h"
#include <limits>
#include <optional>
#include <QDoubleValidator>
#include "src/gui/templates/templates_form.h"
#include "src/core/math/sequences.h"
#include "src/core/common/text_tokens.h"
#include "src/core/common/exception.h"
#include "ui_templates_form.h"
#include "src/gui/common/field_mark.h"

#include "src/gui/application/error_message.h"

#include <QMessageBox>

namespace qftbx {

namespace {

bool asPointCount(double value, double ceiling, std::size_t & count)
{
    if (!std::isfinite(value) || value < 1.0 || value > ceiling) {
        return false;
    }
    count = static_cast<std::size_t>(value);
    return true;
}

}

TemplatesForm::TemplatesForm(QWidget *parent) :
    StepPanel(parent),
    ui(std::make_unique<Ui::TemplatesForm>())
{
    ui->setupUi(this);
    ui->globalPointCount->setValidator(new QDoubleValidator(this));

    setWindowTitle(tr("Template input"));

    ui->globalPointCount->setText(
        qftbx::numberText(qftbx::Settings().defaults.templatePointCount));

#ifndef CUDA_AVAILABLE
    ui->cudaCheck->setVisible(false);
#endif

    selectDefaultsWhereEmpty();
}

TemplatesForm::~TemplatesForm()
{
}

void TemplatesForm::clearTables(){
    if (!rowsBuilt){
        return;
    }

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

std::vector<double> TemplatesForm::takeEpsilon(){
    return std::move(epsilonValues);
}

qftbx::EpsilonMetric TemplatesForm::epsilonMetric() const {
    qftbx::EpsilonMetric metric;
    metric.metric = ui->metricCombo->currentIndex() == 1 ? qftbx::HullMetric::ComplexPlane
                                                         : qftbx::HullMetric::Nichols;
    const double weight = ui->dbPerDegreeEdit->text().toDouble();
    metric.dbPerDegree = weight > 0.0 ? weight : 1.0;
    return metric;
}

void TemplatesForm::setEpsilonMetric(qftbx::EpsilonMetric metric) {
    ui->metricCombo->setCurrentIndex(metric.metric == qftbx::HullMetric::ComplexPlane ? 1 : 0);
    ui->dbPerDegreeEdit->setText(qftbx::numberText(metric.dbPerDegree));
    ui->dbPerDegreeEdit->setEnabled(metric.metric == qftbx::HullMetric::Nichols);
    ui->dbPerDegreeLabel->setEnabled(metric.metric == qftbx::HullMetric::Nichols);
}

void TemplatesForm::forgetPlant(){

    plant = nullptr;
    frequencyCount = 0;
}

void TemplatesForm::launch(LtiSystem *plant, qint32 frequencyCount){

    this->plant = plant;
    this->frequencyCount = frequencyCount;

    buildTables(plant->numerator(), plant->denominator());

    std::vector<std::string> uncertain;
    const auto count = [&uncertain](const Parameter & p) {
        if (p.isUncertain() && std::find(uncertain.begin(), uncertain.end(), p.name()) == uncertain.end()) {
            uncertain.push_back(p.name());
        }
    };
    for (Parameter & p : plant->numerator()) count(p);
    for (Parameter & p : plant->denominator()) count(p);
    count(plant->gain());
    count(plant->delay());
    ui->borderSweepCheck->setEnabled(uncertain.size() == 2);

    selectDefaultsWhereEmpty();
    proposeEpsilon();
}

bool TemplatesForm::wholeTemplateIfNoContour() const
{
    return ui->wholeTemplateCheck->isChecked();
}

void TemplatesForm::setWholeTemplateIfNoContour(bool standsIn)
{
    ui->wholeTemplateCheck->setChecked(standsIn);
}

bool TemplatesForm::borderSweep() const
{
    return ui->borderSweepCheck->isEnabled() && ui->borderSweepCheck->isChecked();
}

void TemplatesForm::setBorderSweep(bool border)
{
    ui->borderSweepCheck->setChecked(border);
}

bool TemplatesForm::alphaShapeContour() const
{
    return ui->contourCombo->currentIndex() == 1;
}

void TemplatesForm::setAlphaShapeContour(bool alphaShape)
{
    ui->contourCombo->setCurrentIndex(alphaShape ? 1 : 0);
}

void TemplatesForm::setEpsilonProposer(EpsilonProposer propose)
{
    m_propose = std::move(propose);
}

void TemplatesForm::selectDefaultsWhereEmpty()
{
    if (!ui->linspaceRadio->isChecked() && !ui->logspaceRadio->isChecked()){
        ui->linspaceRadio->setChecked(true);
    }
    if (!ui->allVariablesRadio->isChecked() && !ui->oneByOneRadio->isChecked()){
        ui->allVariablesRadio->setChecked(true);
        on_allVariablesRadio_clicked();
    }
    if (!ui->nicholsRadio->isChecked() && !ui->nyquistRadio->isChecked()){
        ui->nicholsRadio->setChecked(true);
    }
}

void TemplatesForm::proposeEpsilon()
{
    m_proposals.clear();
    if (!m_propose || plant == nullptr){
        return;
    }

    QString reason;
    if (!readGrids(reason)){
        return;
    }
    try {
        m_proposals = m_propose(gridMap, epsilonMetric());
    } catch (const qftbx::Exception &) {
        m_proposals.clear();
        return;
    }
    if (m_proposals.empty()){
        return;
    }

    QStringList values;
    QStringList detail;
    double worstGap = 0.0;
    for (std::size_t i = 0; i < m_proposals.size(); ++i){
        const qftbx::TemplateEngine::EpsilonProposal & p = m_proposals[i];
        values.push_back(numberText(p.epsilon));
        detail.push_back(p.closes
                             ? tr("frequency %1: %2 (connected from %3, gap %4%)")
                                   .arg(static_cast<int>(i) + 1)
                                   .arg(numberText(p.epsilon), numberText(p.connected))
                                   .arg(QString::number(100.0 * p.coarseness(), 'f', 1))
                             : tr("frequency %1: %2 (connected, but no epsilon up to the diameter closes the walk; gap %3%)")
                                   .arg(static_cast<int>(i) + 1)
                                   .arg(numberText(p.connected))
                                   .arg(QString::number(100.0 * p.coarseness(), 'f', 1)));
        worstGap = std::max(worstGap, p.coarseness());
    }
    ui->epsilonEdit->setText(values.join(QStringLiteral(" ")));
    markWrong(ui->epsilonEdit, false);
    ui->epsilonEdit->setToolTip(tr("The least epsilon at which the contour of each template closes, over the grids "
                                   "as entered; below the connecting value the template splits. The gap is the "
                                   "largest distance between neighbouring points of the template as a share of its "
                                   "size: above a few per cent the sweep is coarse and asks for more points, not a "
                                   "larger epsilon.\n%1").arg(detail.join(QStringLiteral("\n"))));
}

void TemplatesForm::on_proposeButton_clicked()
{
    proposeEpsilon();
}

void TemplatesForm::buildTables(std::vector<Parameter> & numerator, std::vector<Parameter> & denominator){

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

void TemplatesForm::buildRow(QWidget *widget, QVector <ParLineEdit> & par,
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

    rLin->setText(tr("LinSpace"));
    rLog->setText(tr("LogSpace"));
    rManual->setText(tr("Manual"));

    rLin->setToolTip(tr("Sweep this parameter at evenly spaced values over its range."));
    lin->setToolTip(tr("How many values, evenly spaced, from the lower end of the range to "
                       "the upper one."));
    rLog->setToolTip(tr("Sweep this parameter at values evenly spaced in the logarithm of "
                        "its range, which is what a parameter spanning decades asks for."));
    log->setToolTip(tr("How many values, evenly spaced in the logarithm, from the lower end "
                       "of the range to the upper one."));
    rManual->setToolTip(tr("Sweep this parameter at the values written here, and at no "
                           "others."));
    manual->setToolTip(tr("The values themselves, separated by spaces. They need not lie in "
                          "the range and they need not be evenly spaced."));

    par.push_back(ParLineEdit(lin, log, manual));

    ThreeRadioButtons radio;
    radio.linear = rLin;
    radio.logarithmic = rLog;
    radio.manual = rManual;

    rowRadios.push_back(radio);

}

void TemplatesForm::on_allVariablesRadio_clicked()
{
    ui->modeStack->setCurrentIndex(0);
}

void TemplatesForm::on_metricCombo_currentIndexChanged(int index)
{
    ui->dbPerDegreeEdit->setEnabled(index == 0);
    ui->dbPerDegreeLabel->setEnabled(index == 0);
}

void TemplatesForm::on_oneByOneRadio_clicked()
{
    ui->modeStack->setCurrentIndex(2);
}

void TemplatesForm::on_numeratorRadio_clicked()
{
    ui->variablesStack->setCurrentIndex(1);
}

void TemplatesForm::on_denominatorRadio_clicked()
{
    ui->variablesStack->setCurrentIndex(2);
}

void TemplatesForm::setDefaultPointCount(std::int32_t points)
{
    ui->globalPointCount->setText(qftbx::numberText(points));
}

void TemplatesForm::on_okButton_clicked()
{
    if (plant == nullptr) {
        errorMessage(tr("The plant must be entered before the templates."),
                     tr("Template computation"));
        return;
    }

    if (ui->nyquistRadio->isChecked())
        nicholsDiagram = false;
    else if (ui->nicholsRadio->isChecked())
        nicholsDiagram = true;

    cudaEnabled = ui->cudaCheck->isChecked();

    gridMap.clear();
    duplicateNames.clear();

    epsilonValues.clear();

    if (ui->metricCombo->currentIndex() == 0 && !(ui->dbPerDegreeEdit->text().toDouble() > 0.0)){
        errorMessage(tr("The decibels per degree must be a positive number."), tr("Template computation"));
        markWrong(ui->dbPerDegreeEdit, true, tr("The decibels per degree must be a positive number."));
        return;
    }
    markWrong(ui->dbPerDegreeEdit, false);

    if (ui->epsilonEdit->text().isEmpty()){
        errorMessage(tr("No epsilon value was entered."), tr("Template computation"));
        markWrong(ui->epsilonEdit, true, tr("No epsilon value was entered."));
        epsilonValues.clear();
        return;
    }else {

        markWrong(ui->epsilonEdit, false);
        const std::vector<std::string> v = qftbx::text::tokens(ui->epsilonEdit->text().toStdString());

        qreal lastEpsilon = 0;

        qint32 counter = 0;

        for (const std::string & s : v) {
            const std::optional<double> epsilonValue = evaluateNumber(QString::fromStdString(s));
            if (!epsilonValue.has_value()) {
                errorMessage(tr("Invalid epsilon expression."), tr("Template computation"));
                markWrong(ui->epsilonEdit, true, tr("Invalid epsilon expression."));
                epsilonValues.clear();
                return;
            }
            lastEpsilon = *epsilonValue;
            if (!std::isfinite(lastEpsilon) || lastEpsilon <= 0.0) {
                errorMessage(tr("Every epsilon must be a positive finite number."), tr("Template computation"));
                markWrong(ui->epsilonEdit, true,
                          tr("Every epsilon must be a positive finite number."));
                epsilonValues.clear();
                return;
            }
            epsilonValues.push_back(lastEpsilon);
            counter++;
        }

        for (; counter < frequencyCount; counter++){
            epsilonValues.push_back(lastEpsilon);
        }
    }

    QString reason;
    if (!readGrids(reason)){
        errorMessage(reason, tr("Template computation"));
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
}

bool TemplatesForm::readGrids(QString & reason)
{
    reason.clear();
    duplicateNames.clear();

    bool useLinspace = false;
    bool useLogspace = false;

    gridMap.clear();

    if (ui->linspaceRadio->isChecked() && !ui->globalPointCount->text().isEmpty()){
        useLinspace = true;
    }else if (ui->logspaceRadio->isChecked() && !ui->globalPointCount->text().isEmpty()){
        useLogspace = true;
    }else {
        reason = tr("Select logspace or linspace in the general section.");
        return false;
    }

    try {

    ThreeRadioButtons rowRadios;
    ParLineEdit rowEdits;
    qint32 variableIndex = 0;
    for (qint32 i = 0; i < static_cast<qint32>(numerator.size()); i++){
        Parameter & parameter = numerator[i];
        if (parameter.isUncertain()){
            rowEdits = numeratorRows.at(variableIndex);
            rowRadios = numeratorRadios.at(variableIndex);
            variableIndex++;
            if (!readVariable(rowEdits, rowRadios,parameter,useLinspace,useLogspace)){
                reason = m_readReason.isEmpty()
                             ? tr("The values entered for parameter \"%1\" are invalid.")
                                   .arg(QString::fromStdString(parameter.name()))
                             : tr("The values entered for parameter \"%1\" are invalid: %2.")
                                   .arg(QString::fromStdString(parameter.name()))
                                   .arg(m_readReason);
                return false;
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
                reason = m_readReason.isEmpty()
                             ? tr("The values entered for parameter \"%1\" are invalid.")
                                   .arg(QString::fromStdString(parameter.name()))
                             : tr("The values entered for parameter \"%1\" are invalid: %2.")
                                   .arg(QString::fromStdString(parameter.name()))
                                   .arg(m_readReason);
                return false;
            }
        }
    }

    if (!plant->gain().isUncertain()){
        gridMap[plant->gain().name()] = std::vector<double>(1, plant->gain().nominal());
    }
    else{

        const qreal start = plant->gain().range().min;
        const qreal end = plant->gain().range().max;
        std::size_t pointCount = 0;
        if (!asPointCount(evaluateNumber(ui->globalPointCount->text()).value_or(std::numeric_limits<double>::quiet_NaN()),
                          m_maxPointCount, pointCount)){
            reason = tr("The general point count must be a whole "
                        "number between 1 and %1.").arg(static_cast<qint64>(m_maxPointCount));
            return false;
        }

        if (useLinspace){
            gridMap[plant->gain().name()] = qftbx::math::linspace(start, end, pointCount);
        } else {
            gridMap[plant->gain().name()] = qftbx::math::logspace(start, end, pointCount);
        }
    }

    if (!plant->delay().isUncertain()){
        gridMap[plant->delay().name()] = std::vector<double>(1, plant->delay().nominal());
    }else {

        const qreal start = plant->delay().range().min;
        const qreal end = plant->delay().range().max;
        std::size_t pointCount = 0;
        if (!asPointCount(evaluateNumber(ui->globalPointCount->text()).value_or(std::numeric_limits<double>::quiet_NaN()),
                          m_maxPointCount, pointCount)){
            reason = tr("The general point count must be a whole "
                        "number between 1 and %1.").arg(static_cast<qint64>(m_maxPointCount));
            return false;
        }

        if (useLinspace){
            gridMap[plant->delay().name()] = qftbx::math::linspace(start, end, pointCount);
        } else {
            gridMap[plant->delay().name()] = qftbx::math::logspace(start, end, pointCount);
        }
    }

    } catch (const std::invalid_argument &) {
        reason = tr("Invalid grid expressions.");
        return false;
    }

    return true;
}

bool TemplatesForm::readVariable(const ParLineEdit & rowEdits, ThreeRadioButtons rowRadios,
                                    Parameter & parameter, bool useLinspace, bool useLogspace){

    m_readReason.clear();

    if (gridMap.count(parameter.name()) != 0){
        if (!duplicateNames.contains(QString::fromStdString(parameter.name()))){
            duplicateNames.push_back(QString::fromStdString(parameter.name()));
        }
        return true;
    }

    qreal start;
    qreal end;
    std::size_t pointCount = 0;

    if (rowRadios.linear->isChecked() && !rowEdits.getX()->text().isEmpty()){

        start = parameter.range().min;
        end = parameter.range().max;

        if (!asPointCount(evaluateNumber(rowEdits.getX()->text()).value_or(std::numeric_limits<double>::quiet_NaN()),
                          m_maxPointCount, pointCount)){
            m_readReason = tr("its point count must be a whole number "
                              "between 1 and %1")
                    .arg(static_cast<qint64>(m_maxPointCount));
            return false;
        }

        gridMap[parameter.name()] = qftbx::math::linspace(start, end, pointCount);

    }else if (rowRadios.logarithmic->isChecked() && !rowEdits.getY()->text().isEmpty()){

        start = parameter.range().min;
        end = parameter.range().max;
        if (!asPointCount(evaluateNumber(rowEdits.getY()->text()).value_or(std::numeric_limits<double>::quiet_NaN()),
                          m_maxPointCount, pointCount)){
            m_readReason = tr("its point count must be a whole number "
                              "between 1 and %1")
                    .arg(static_cast<qint64>(m_maxPointCount));
            return false;
        }

        gridMap[parameter.name()] = qftbx::math::logspace(start, end, pointCount);

    }else if(rowRadios.manual->isChecked() && !rowEdits.nominal()->text().isEmpty()){

        const std::vector<std::string> vector = qftbx::text::tokens(rowEdits.nominal()->text().toStdString());
        std::vector<double> values;
        values.reserve(static_cast<std::size_t>(vector.size()));

        for (const std::string & sSymbolCount : vector) {
            const double value = evaluateNumber(QString::fromStdString(sSymbolCount)).value_or(std::numeric_limits<double>::quiet_NaN());
            if (!std::isfinite(value)){
                m_readReason = tr("one of its grid values is not a finite "
                                  "number");
                return false;
            }
            values.push_back(value);
        }

        gridMap[parameter.name()] = std::move(values);
    }else if (useLinspace || useLogspace){

        start = parameter.range().min;
        end = parameter.range().max;

        if (!asPointCount(evaluateNumber(ui->globalPointCount->text()).value_or(std::numeric_limits<double>::quiet_NaN()),
                          m_maxPointCount, pointCount)){
            m_readReason = tr("its point count must be a whole number "
                              "between 1 and %1")
                    .arg(static_cast<qint64>(m_maxPointCount));
            return false;
        }

        if(useLinspace){
            gridMap[parameter.name()] = qftbx::math::linspace(start, end, pointCount);
        }else {
            gridMap[parameter.name()] = qftbx::math::logspace(start, end, pointCount);
        }
    }else{
        return false;
    }

    return true;
}

qftbx::ParameterGrids TemplatesForm::grids() const{
    return gridMap;
}

bool TemplatesForm::nicholsSelected(){
    return nicholsDiagram;
}

bool TemplatesForm::cudaSelected(){
    return cudaEnabled;
}

}
