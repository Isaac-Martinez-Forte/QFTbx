#include "uncertainty_dialog.h"
#include "ui_uncertainty_dialog.h"

#include "GUI/error_message.h"
#include "GUI/plot_palette.h"

using namespace tools;

UncertaintyDialog::UncertaintyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UncertaintyDialog)
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

    delete ui;

    if (rowsBuilt == true){
        delete numeratorLayout;
        delete denominatorLayout;
        //The working ParLineEdits and Parameters belong to the dialog (the
        //plant receives clones): they used to be abandoned with clear().
        for (ParLineEdit * par : *numeratorRows){
            delete par;
        }
        delete numeratorRows;
        for (ParLineEdit * par : *denominatorRows){
            delete par;
        }
        delete denominatorRows;
        qDeleteAll(*numeratorParameters);
        delete numeratorParameters;
        qDeleteAll(*denominatorParameters);
        delete denominatorParameters;
        delete rowWidgets;
    }

    releaseTables();
}

//The input tables arrive from PlantDialog and become the dialog's.
void UncertaintyDialog::releaseTables(){
    if (valueTable != nullptr){
        qDeleteAll(*valueTable);
        delete valueTable;
        valueTable = nullptr;
    }
    if (expressionTable != nullptr){
        qDeleteAll(*expressionTable);
        delete expressionTable;
        expressionTable = nullptr;
    }
    if (uncertainTable != nullptr){
        qDeleteAll(*uncertainTable);
        delete uncertainTable;
        uncertainTable = nullptr;
    }
}


bool UncertaintyDialog::launch(QVector <QVector <QString> * > * valueTable, QVector <QVector <QString> * > * expressionTable,
                                       QVector<QVector<bool> *> *uncertainTable, bool rowsBuilt){

    //Every launch replaces the previous tables (they used to leak).
    releaseTables();
    accepted_ok = false;

    this->valueTable = valueTable;
    this->expressionTable = expressionTable;
    this->uncertainTable = uncertainTable;

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


    numeratorTokens = valueTable->at(0);
    denominatorTokens = valueTable->at(1);

    if (rowsBuilt == true){
        delete numeratorLayout;
        delete denominatorLayout;
        for (ParLineEdit * par : *numeratorRows){
            delete par;
        }
        delete numeratorRows;
        for (ParLineEdit * par : *denominatorRows){
            delete par;
        }
        delete denominatorRows;
        qDeleteAll(*numeratorParameters);
        delete numeratorParameters;
        qDeleteAll(*denominatorParameters);
        delete denominatorParameters;
        for (qint32 i = 0; i < rowWidgets->size(); i++) {
            delete rowWidgets->at(i);
        }
        delete rowWidgets;
    }

    numeratorRows = new std::list <ParLineEdit*> ();
    denominatorRows = new std::list <ParLineEdit*> ();

    numeratorLayout=new QVBoxLayout(ui->numeratorBox);
    denominatorLayout=new QVBoxLayout(ui->denominatorBox);

    rowWidgets = new QVector <QWidget *> ();

    this->numeratorParameters = new QVector <Parameter*> ();
    this->numeratorParameters->reserve(numeratorParameters->size());
    this->denominatorParameters = new QVector <Parameter*> ();
    this->denominatorParameters->reserve(denominatorParameters->size());

    rowsBuilt = true;

    QVector <QString> * seenNames = new QVector <QString> ();

    qint32 i = 0;

    foreach (const QString &valor, *numeratorTokens){
        if(uncertainTable->at(0)->at(i)){
            if (!seenNames->contains(valor)){
                QWidget * widget = new QWidget(ui->numeratorBox);
                buildRow(widget, valor, numeratorRows, rangeOnlyMode);
                numeratorLayout->addWidget(widget);
                rowWidgets->append(widget);
                seenNames->append(valor);
            }
        }
        i++;
    }

    i = 0;
    foreach (const QString &valor, *denominatorTokens){
        if(uncertainTable->at(1)->at(i)){
            if (!seenNames->contains(valor)){
                QWidget * widget = new QWidget(ui->denominatorBox);
                buildRow(widget, valor, denominatorRows, rangeOnlyMode);
                denominatorLayout->addWidget(widget);
                rowWidgets->append(widget);
                seenNames->append(valor);
            }
        }
        i++;
    }

    delete seenNames;

    ui->denominatorArea->setAutoFillBackground(true);
    ui->numeratorArea->setAutoFillBackground(true);
    ui->denominatorArea->setLayout(denominatorLayout);
    ui->numeratorArea->setLayout(numeratorLayout);
}

void UncertaintyDialog:: buildRow(QWidget *widget, QString parameter, std::list <ParLineEdit*> * vector, bool rowsBuilt){

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

    ParLineEdit * par = new ParLineEdit(inicio, fin, nominal);
    vector->push_back(par);

    //rowWidgets->append(horizontalLayout);
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

QVector <Parameter*> * UncertaintyDialog::numerator(){
    return numeratorParameters;
}

QVector<Parameter*> *UncertaintyDialog::denominator(){
    return denominatorParameters;
}

QPointF UncertaintyDialog::gain(){

    mup::ParserX p;

    if (ui->gainStart->text().isEmpty() || ui->gainEnd->text().isEmpty()){
        ui->gainStart->setText("1");
        ui->gainEnd->setText("1");
    }

    p.SetExpr(ui->gainStart->text().toStdString());

    QPointF rowsBuilt;

    rowsBuilt.setX(p.Eval().GetFloat());

    p.SetExpr(ui->gainEnd->text().toStdString());

    rowsBuilt.setY(p.Eval().GetFloat());

    return rowsBuilt;
}

QPointF UncertaintyDialog::delay(){

    mup::ParserX p;

    if (ui->delayStart->text().isEmpty() || ui->delayEnd->text().isEmpty()){
        ui->delayStart->setText("0");
        ui->delayEnd->setText("0");
    }

    p.SetExpr(ui->delayStart->text().toStdString());

    QPointF rowsBuilt;

    rowsBuilt.setX(p.Eval().GetFloat());

    p.SetExpr(ui->delayEnd->text().toStdString());

    rowsBuilt.setY(p.Eval().GetFloat());

    return rowsBuilt;
}

qreal UncertaintyDialog::parse(QString cadena)
{
    p.SetExpr(cadena.toStdString());

    return p.Eval().GetFloat();
}

bool UncertaintyDialog::readRanges(){

    //Idempotent retry: after a partial error, the previous run left
    //parameters inserted and the next accept DUPLICATED them.
    qDeleteAll(*numeratorParameters);
    numeratorParameters->clear();
    qDeleteAll(*denominatorParameters);
    denominatorParameters->clear();

    QLineEdit * startEdit;
    QLineEdit * endEdit;
    QLineEdit * nominal;

    qreal startValue = 0;
    qreal endValue = 0;
    qreal nominalValue = 0;

    bool allValid = true;
    bool valid = true;

    QVector <QString> * seenNames = new QVector <QString> ();

    for (qint32 i = 0; i < numeratorTokens->size(); i++){
        Parameter * parameter = NULL;
        valid = true;
        if(uncertainTable->at(0)->at(i)){
            if (!seenNames->contains(numeratorTokens->at(i))){

                ParLineEdit * aux = numeratorRows->front();

                startEdit = aux->getX();
                endEdit = aux->getY();
                nominal= aux->nominal();
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
                        QPointF rowsBuilt (startValue, endValue);
                        parameter = new Parameter (numeratorTokens->at(i), rowsBuilt, nominalValue, expressionTable->at(0)->at(i));

                        startEdit->setStyleSheet("background : white");
                        endEdit->setStyleSheet("background : white");
                        nominal->setStyleSheet("background : white");

                        numeratorRows->pop_front();
                        delete aux;
                        valid = true;
                    } else {
                        startEdit->setStyleSheet("background : red");
                        endEdit->setStyleSheet("background : red");
                        nominal->setStyleSheet("background : red");
                        valid = false;
                    }
                }
            } else{
                for (qint32 x = 0; x < numeratorParameters->size(); x++){
                    if (numeratorParameters->at(x)->name() == numeratorTokens->at(i)){
                        Parameter * v = numeratorParameters->at(x);
                        parameter = new Parameter(v->name(),v->rawRange(),v->rawNominal(),v->expression());
                        break;
                    }
                }
            }
        }else {
            parameter = new Parameter (numeratorTokens->at(i).toDouble());
        }

        if (valid && parameter != NULL){
            numeratorParameters->insert(i,parameter);
            seenNames->append(numeratorTokens->at(i));
        }else{
            allValid = false;
        }
    }

    if (!allValid){
        delete seenNames;
        errorMessage(tr("There are errors in the parameter ranges"), tr("Uncertainty input"));
        return false;
    }

    allValid = true;

    for (qint32 i = 0; i < denominatorTokens->size(); i++){
        Parameter * parameter = NULL;
        if(uncertainTable->at(1)->at(i)){
            if (!seenNames->contains(denominatorTokens->at(i))){

                ParLineEdit * aux = denominatorRows->front();

                startEdit = aux->getX();
                endEdit = aux->getY();
                nominal = aux->nominal();

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
                            QPointF rowsBuilt (startValue, endValue);
                            parameter = new Parameter (denominatorTokens->at(i), rowsBuilt, nominalValue, expressionTable->at(1)->at(i));

                            startEdit->setStyleSheet("background : white");
                            endEdit->setStyleSheet("background : white");
                            nominal->setStyleSheet("background : white");

                            denominatorRows->pop_front();
                            delete aux;
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
                for (qint32 x = 0; x < numeratorParameters->size(); x++){
                    if (numeratorParameters->at(x)->name() == denominatorTokens->at(i)){
                        Parameter * v = numeratorParameters->at(x);
                        parameter = new Parameter(v->name(),v->rawRange(),v->rawNominal(),v->expression());
                        break;
                    }
                }

                if (!elegido){
                    for (qint32 x = 0; x < denominatorParameters->size(); x++){
                        if (denominatorParameters->at(x)->name() == denominatorTokens->at(i)){
                            Parameter * v = denominatorParameters->at(x);
                            parameter = new Parameter(v->name(),v->rawRange(),v->rawNominal(),v->expression());
                            break;
                        }
                    }
                }
            }
        }else {
            parameter = new Parameter (denominatorTokens->at(i).toDouble());
        }

        if (valid && parameter != NULL){
            denominatorParameters->insert(i,parameter);
            seenNames->append(denominatorTokens->at(i));
        }else{
            allValid = false;
        }
    }

    delete seenNames;

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

bool UncertaintyDialog::getTodoCorrecto(){
    return accepted_ok;
}
