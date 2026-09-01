#include "frequencies_dialog.h"
#include "src/core/text_tokens.h"
#include "ui_frequencies_dialog.h"

#include <QMessageBox>

#include "src/core/exception.h"

using namespace tools;


FrequenciesDialog::FrequenciesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FrequenciesDialog)
{

    ui->setupUi(this);
    setWindowTitle(tr("Design frequencies input"));

    //The linspace/logspace line edits only accept real numbers.
    ui->logEnd->setValidator(new QDoubleValidator(this));
    ui->logStart->setValidator(new QDoubleValidator(this));
    ui->logCount->setValidator(new QDoubleValidator(this));

    ui->linEnd->setValidator(new QDoubleValidator(this));
    ui->linStart->setValidator(new QDoubleValidator(this));
    ui->linCount->setValidator(new QDoubleValidator(this));

    todoCorrecto = false;

    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
    connect (this, SIGNAL(close_ok()), this,SLOT(close()));
}

FrequenciesDialog::FrequenciesDialog(ProjectController *controlador, QWidget *parent):FrequenciesDialog(parent){
    this->controlador = controlador;
}

FrequenciesDialog::~FrequenciesDialog()
{
    delete ui;
}
void FrequenciesDialog::on_fileButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()){
        filePath=fileName;
        ui->filePathLabel->setText(filePath);
    }
}

void FrequenciesDialog::on_okButton_clicked()
{
    qreal start = 0;
    qreal end = 0;
    Omega::GenerationType type;
    QVector <qreal> * frequencies;

    if (ui->modeStack->currentIndex() == 0){ //manual
        frequencies = qftbx::text::reals(ui->manualValues->text());
        type = Omega::Manual;
        if (frequencies == NULL){
            //Invalid input: it used to carry on and dereference the null
            //pointer a few lines below.
            ui->manualValues->setStyleSheet("background : red");
            return;
        }
        ui->manualValues->setStyleSheet("background : white");

    } else if (ui->modeStack->currentIndex() == 1) { //logspace
        frequencies = logspace(ui->logStart->text().toDouble(),ui->logEnd->text().toDouble(),
                               ui->logCount->text().toDouble());
        start = ui->logStart->text().toDouble();
        end = ui->logEnd->text().toDouble();
        type = Omega::LogSpace;

    }else if (ui->modeStack->currentIndex() == 2) { //linspace

        frequencies = linspace(ui->linStart->text().toDouble(),ui->linEnd->text().toDouble(),
                               ui->linCount->text().toDouble());

        start = ui->linStart->text().toDouble();
        //linStart used to be re-read: every linear Omega was stored with
        //end == start (and travelled like that into the .qft and Bode).
        end = ui->linEnd->text().toDouble();
        type = Omega::LinSpace;

    } else {
        try {
            frequencies = Omega::valuesFromFile(filePath);
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Design frequencies input"), e.what());
            return;
        }
        type = Omega::File;
    }

    Omega * omega = new Omega(start, end, frequencies->size(),frequencies,type);
    controlador->setOmega(omega);

    todoCorrecto = true;

    emit (close_ok());
}


bool FrequenciesDialog::getTodoCorrecto(){
    return todoCorrecto;
}
