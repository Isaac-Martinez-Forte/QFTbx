#include "uncertainty_dialog.h"

#include <optional>
#include "ui_uncertainty_dialog.h"

#include "src/gui/error_message.h"
#include "src/gui/plot_palette.h"

using namespace tools;

UncertaintyDialog::UncertaintyDialog(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::UncertaintyDialog>())
{
    rowsBuilt = false;
    ui->setupUi(this);

    ui->numeratorBox->setVisible(false);
    ui->denominatorBox->setVisible(false);

    rowsBuilt = false;

    setWindowTitle(tr("Plant uncertainty input"));

    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
}

UncertaintyDialog::~UncertaintyDialog(){


    //Nothing to free: the layouts and the row widgets are Qt children of
    //the scroll areas, which are children of this dialog.
}

//The input tables arrive from PlantDialog or ControllerDialog and become
//the dialog's. They used to be three pointers to vectors of pointers, with
//a releaseTables() to walk them: every launch had to call it first or the
//previous tables leaked.
bool UncertaintyDialog::launch(CoefficientTable valueTable, CoefficientTable expressionTable,
                               UncertainTable uncertainTable, bool rowsBuilt){

    accepted_ok = false;

    this->valueTable = std::move(valueTable);
    this->expressionTable = std::move(expressionTable);
    this->uncertainTable = std::move(uncertainTable);

    this->rangeOnlyMode = rowsBuilt;

    if (rowsBuilt){
        ui->label->setVisible(false);
        ui->label_2->setVisible(false);
        ui->label_3->setVisible(false);
        ui->gainStart->setVisible(false);
        ui->gainEnd->setVisible(false);

        ui->delayStart->setVisible(false);
        ui->delayEnd->setVisible(false);
        ui->label_4->setVisible(false);
        ui->label_5->setVisible(false);
        ui->label_6->setVisible(false);
    }

    buildRows();

    return true;
}

void UncertaintyDialog::buildRows(){

    //Rows of the value table, by name: they used to be two members aliasing
    //into it.
    const CoefficientRow & numeratorTokens = valueTable.at(0);
    const CoefficientRow & denominatorTokens = valueTable.at(1);



    if (rowsBuilt == true){
        //Qt's own mechanism, and the only reason there are deletes here: a
        //widget holds exactly one layout, and destroying the row widget is
        //how it leaves the box.
        delete numeratorLayout;
        delete denominatorLayout;
        for (QWidget * row : rowWidgets) {
            delete row;
        }
    }

    numeratorRows.clear();
    denominatorRows.clear();
    rowWidgets.clear();

    //On the scroll AREA's inner widget, which is what holds the content of
    //a QScrollArea. This used to build the layouts on the scroll areas
    //themselves and then setLayout() them onto the inner widgets, which
    //works only because Qt STEALS a layout from its old widget parent and
    //reparents the widgets it manages: the widget named here was not the
    //one that ended up owning them.
    numeratorLayout = new QVBoxLayout(ui->numeratorArea);
    denominatorLayout = new QVBoxLayout(ui->denominatorArea);

    this->numeratorParameters.clear();
    this->numeratorParameters.reserve(numeratorParameters.size());
    this->denominatorParameters.clear();
    this->denominatorParameters.reserve(denominatorParameters.size());

    rowsBuilt = true;

    QVector <QString> seenNames;

    qint32 i = 0;

    foreach (const QString &value, numeratorTokens){
        if(uncertainTable.at(0).at(i)){
            if (!seenNames.contains(value)){
                QWidget * widget = new QWidget(ui->numeratorArea);
                buildRow(widget, value, numeratorRows, rangeOnlyMode);
                numeratorLayout->addWidget(widget);
                rowWidgets.append(widget);
                seenNames.append(value);
            }
        }
        i++;
    }

    i = 0;
    foreach (const QString &value, denominatorTokens){
        if(uncertainTable.at(1).at(i)){
            if (!seenNames.contains(value)){
                QWidget * widget = new QWidget(ui->denominatorArea);
                buildRow(widget, value, denominatorRows, rangeOnlyMode);
                denominatorLayout->addWidget(widget);
                rowWidgets.append(widget);
                seenNames.append(value);
            }
        }
        i++;
    }


    ui->denominatorArea->setAutoFillBackground(true);
    ui->numeratorArea->setAutoFillBackground(true);
}

void UncertaintyDialog:: buildRow(QWidget *widget, QString parameter,
                                  std::list <ParLineEdit> & vector, bool rowsBuilt){

    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *inicio;
    QLabel *label_2;
    QLineEdit *fin;
    QLabel *label_3;
    QLineEdit *nominal;


    widget->setObjectName(QString::fromUtf8("widget"));
    widget->setGeometry(QRect(20, 30, 221, 25));

    horizontalLayout = new QHBoxLayout(widget);
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    label = new QLabel(widget);
    label->setObjectName(QString::fromUtf8("label"));

    horizontalLayout->addWidget(label);

    inicio = new QLineEdit(widget);
    inicio->setObjectName(QString::fromUtf8("inicio"));
    horizontalLayout->addWidget(inicio);

    label_2 = new QLabel(widget);
    label_2->setObjectName(QString::fromUtf8("label_2"));

    horizontalLayout->addWidget(label_2);

    fin = new QLineEdit(widget);
    fin->setObjectName(QString::fromUtf8("fin"));
    horizontalLayout->addWidget(fin);

    label_3 = new QLabel(widget);
    label_3->setObjectName(QString::fromUtf8("label_3"));

    horizontalLayout->addWidget(label_3);


    nominal = new QLineEdit(widget);
    nominal->setObjectName(QString::fromUtf8("nominal"));
    horizontalLayout->addWidget(nominal);

    if (rowsBuilt){
        nominal->setVisible(false);
    }

    inicio->raise();

    QString cad = parameter + ": [";

    label->setText(QApplication::tr("%1").arg(cad));
    label_2->setText(QApplication::translate("UncertaintyDialog", ",", 0));

    if (!rangeOnlyMode){
        label_3->setText(QApplication::translate("UncertaintyDialog", "] Nominal:", 0));
    } else {
        label_3->setText(QApplication::translate("UncertaintyDialog", "]", 0));
    }

    vector.push_back(ParLineEdit(inicio, fin, nominal));

    //rowWidgets.append(horizontalLayout);
}

void UncertaintyDialog::on_numeratorRadio_clicked()
{
    ui->denominatorBox->setVisible(false);
    ui->numDenStack->setCurrentIndex(0);
    ui->numeratorBox->setVisible(true);
}

void UncertaintyDialog::on_denominatorRadio_clicked()
{
    ui->numeratorBox->setVisible(false);
    ui->numDenStack->setCurrentIndex(1);
    ui->denominatorBox->setVisible(true);
}

std::vector<Parameter> & UncertaintyDialog::numerator(){
    return numeratorParameters;
}

std::vector<Parameter> & UncertaintyDialog::denominator(){
    return denominatorParameters;
}

Range UncertaintyDialog::gain(){

    mup::ParserX p;

    if (ui->gainStart->text().isEmpty() || ui->gainEnd->text().isEmpty()){
        ui->gainStart->setText("1");
        ui->gainEnd->setText("1");
    }

    p.SetExpr(ui->gainStart->text().toStdString());

    Range rowsBuilt;

    rowsBuilt.min = p.Eval().GetFloat();

    p.SetExpr(ui->gainEnd->text().toStdString());

    rowsBuilt.max = p.Eval().GetFloat();

    return rowsBuilt;
}

Range UncertaintyDialog::delay(){

    mup::ParserX p;

    if (ui->delayStart->text().isEmpty() || ui->delayEnd->text().isEmpty()){
        ui->delayStart->setText("0");
        ui->delayEnd->setText("0");
    }

    p.SetExpr(ui->delayStart->text().toStdString());

    Range rowsBuilt;

    rowsBuilt.min = p.Eval().GetFloat();

    p.SetExpr(ui->delayEnd->text().toStdString());

    rowsBuilt.max = p.Eval().GetFloat();

    return rowsBuilt;
}

qreal UncertaintyDialog::parse(QString cadena)
{
    p.SetExpr(cadena.toStdString());

    return p.Eval().GetFloat();
}

bool UncertaintyDialog::readRanges(){

    const CoefficientRow & numeratorTokens = valueTable.at(0);
    const CoefficientRow & denominatorTokens = valueTable.at(1);

    //Idempotent retry: after a partial error, the previous run left
    //parameters inserted and the next accept DUPLICATED them.
    numeratorParameters.clear();
    denominatorParameters.clear();

    QLineEdit * startEdit;
    QLineEdit * endEdit;
    QLineEdit * nominal;

    qreal startValue = 0;
    qreal endValue = 0;
    qreal nominalValue = 0;

    bool allValid = true;
    bool valid = true;

    QVector <QString> seenNames;

    for (qint32 i = 0; i < numeratorTokens.size(); i++){
        //Optional: an invalid row leaves it empty (it used to be a null
        //pointer used as the validity sentinel).
        std::optional<Parameter> parameter;
        valid = true;
        if(uncertainTable.at(0).at(i)){
            if (!seenNames.contains(numeratorTokens.at(i))){

                const ParLineEdit aux = numeratorRows.front();

                startEdit = aux.getX();
                endEdit = aux.getY();
                nominal= aux.nominal();
                if (rangeOnlyMode){
                    nominal->setText(QString::number((startEdit->text().toDouble() + endEdit->text().toDouble()) / 2));
                }

                if (startEdit->text().isEmpty() || endEdit->text().isEmpty() || nominal->text().isEmpty()){
                    startEdit->setStyleSheet("background : red");
                    endEdit->setStyleSheet("background : red");
                    nominal->setStyleSheet("background : red");
                    valid = false;
                }else{
                    try {
                        startValue = parse(startEdit->text());
                        endValue = parse(endEdit->text());
                        nominalValue = parse(nominal->text());
                    } catch (mup::ParserError &) {
                        //Invalid expression: it used to blow the dialog up.
                        startEdit->setStyleSheet("background : red");
                        endEdit->setStyleSheet("background : red");
                        nominal->setStyleSheet("background : red");
                        valid = false;
                        startValue = 1;
                        endValue = 0;
                        nominalValue = 0;
                    }

                    if (valid && (startValue <= nominalValue) && (nominalValue <= endValue)){
                        Range rowsBuilt (startValue, endValue);
                        parameter = Parameter(numeratorTokens.at(i), rowsBuilt, nominalValue, expressionTable.at(0).at(i));

                        startEdit->setStyleSheet("background : white");
                        endEdit->setStyleSheet("background : white");
                        nominal->setStyleSheet("background : white");

                        numeratorRows.pop_front();
                        valid = true;
                    } else {
                        startEdit->setStyleSheet("background : red");
                        endEdit->setStyleSheet("background : red");
                        nominal->setStyleSheet("background : red");
                        valid = false;
                    }
                }
            } else{
                for (qint32 x = 0; x < static_cast<qint32>(numeratorParameters.size()); x++){
                    if (numeratorParameters[x].name() == numeratorTokens.at(i)){
                        Parameter & v = numeratorParameters[x];
                        parameter = Parameter(v.name(), v.rawRange(), v.rawNominal(), v.expression());
                        break;
                    }
                }
            }
        }else {
            parameter = Parameter(numeratorTokens.at(i).toDouble());
        }

        if (valid && parameter.has_value()){
            numeratorParameters.insert(numeratorParameters.begin() + i, *parameter);
            seenNames.append(numeratorTokens.at(i));
        }else{
            allValid = false;
        }
    }

    if (!allValid){
        errorMessage(tr("There are errors in the parameter ranges"), tr("Uncertainty input"));
        return false;
    }

    allValid = true;

    for (qint32 i = 0; i < denominatorTokens.size(); i++){
        //Optional: an invalid row leaves it empty (it used to be a null
        //pointer used as the validity sentinel).
        std::optional<Parameter> parameter;
        if(uncertainTable.at(1).at(i)){
            if (!seenNames.contains(denominatorTokens.at(i))){

                const ParLineEdit aux = denominatorRows.front();

                startEdit = aux.getX();
                endEdit = aux.getY();
                nominal = aux.nominal();

                if (rangeOnlyMode){
                    nominal->setText(QString::number((startEdit->text().toDouble() + endEdit->text().toDouble()) / 2));
                }

                if (startEdit->text().isEmpty() || endEdit->text().isEmpty() || nominal->text().isEmpty()){
                    valid = false;
                    startEdit->setStyleSheet("background : red");
                    endEdit->setStyleSheet("background : red");
                    nominal->setStyleSheet("background : red");
                }else{
                    try {
                        startValue = parse(startEdit->text());
                        endValue = parse(endEdit->text());
                        nominalValue = parse(nominal->text());
                    } catch (mup::ParserError &) {
                        startEdit->setStyleSheet("background : red");
                        endEdit->setStyleSheet("background : red");
                        nominal->setStyleSheet("background : red");
                        valid = false;
                    }

                    if (valid){
                        if ((startValue <= nominalValue) && (nominalValue <= endValue)){
                            Range rowsBuilt (startValue, endValue);
                            parameter = Parameter(denominatorTokens.at(i), rowsBuilt, nominalValue, expressionTable.at(1).at(i));

                            startEdit->setStyleSheet("background : white");
                            endEdit->setStyleSheet("background : white");
                            nominal->setStyleSheet("background : white");

                            denominatorRows.pop_front();
                        } else {
                            valid = false;
                            startEdit->setStyleSheet("background : red");
                            endEdit->setStyleSheet("background : red");
                            nominal->setStyleSheet("background : red");
                        }
                    }
                }
            } else{
                bool elegido = false;
                for (qint32 x = 0; x < static_cast<qint32>(numeratorParameters.size()); x++){
                    if (numeratorParameters[x].name() == denominatorTokens.at(i)){
                        Parameter & v = numeratorParameters[x];
                        parameter = Parameter(v.name(), v.rawRange(), v.rawNominal(), v.expression());
                        break;
                    }
                }

                if (!elegido){
                    for (qint32 x = 0; x < static_cast<qint32>(denominatorParameters.size()); x++){
                        if (denominatorParameters[x].name() == denominatorTokens.at(i)){
                            Parameter & v = denominatorParameters[x];
                            parameter = Parameter(v.name(), v.rawRange(), v.rawNominal(), v.expression());
                            break;
                        }
                    }
                }
            }
        }else {
            parameter = Parameter(denominatorTokens.at(i).toDouble());
        }

        if (valid && parameter.has_value()){
            denominatorParameters.insert(denominatorParameters.begin() + i, *parameter);
            seenNames.append(denominatorTokens.at(i));
        }else{
            allValid = false;
        }
    }


    if (!allValid){
        errorMessage(tr("There are errors in the parameter ranges"), tr("Uncertainty input"));
        return false;
    }

    return true;
}

void UncertaintyDialog::on_okButton_clicked()
{
    if (ui->modeStack->currentIndex() == 0){
        if (readRanges()){
            accepted_ok = true;
            this->close();
        }
    }
}

bool UncertaintyDialog::wasAccepted(){
    return accepted_ok;
}
