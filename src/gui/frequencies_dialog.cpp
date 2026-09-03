#include <QIntValidator>
#include "frequencies_dialog.h"
#include "src/core/text_tokens.h"
#include "ui_frequencies_dialog.h"

#include <vector>
#include <cmath>

#include <QMessageBox>

#include "src/gui/error_message.h"
#include "src/core/exception.h"

using namespace tools;


namespace {
//Enough for any design frequency sweep, and small enough that the count
//cannot overflow the std::int32_t that logspace and linspace take.
constexpr int kMaxFrequencyCount = 1000000;
}

FrequenciesDialog::FrequenciesDialog(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::FrequenciesDialog>())
{

    ui->setupUi(this);
    setWindowTitle(tr("Design frequencies input"));

    //The linspace/logspace line edits only accept real numbers.
    ui->logEnd->setValidator(new QDoubleValidator(this));
    ui->logStart->setValidator(new QDoubleValidator(this));
    //A count of frequencies is a whole number. With a QDoubleValidator here
    //a "10.5" was accepted and silently truncated, and a "1e12" reached a
    //std::int32_t parameter, where converting a double that far out of range
    //is undefined behaviour. The boundary grid dialog already does this.
    ui->logCount->setValidator(new QIntValidator(1, kMaxFrequencyCount, this));

    ui->linEnd->setValidator(new QDoubleValidator(this));
    ui->linStart->setValidator(new QDoubleValidator(this));
    ui->linCount->setValidator(new QIntValidator(1, kMaxFrequencyCount, this));

    accepted = false;

    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
    connect (this, SIGNAL(close_ok()), this,SLOT(close()));
}

FrequenciesDialog::~FrequenciesDialog()
{
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
    Omega::GenerationType type = Omega::Manual;
    std::vector<double> frequencies;

    if (ui->modeStack->currentIndex() == 0){ //manual
        const std::optional<std::vector<double>> parsed =
                qftbx::text::reals(ui->manualValues->text().toStdString());
        type = Omega::Manual;
        if (!parsed.has_value()){
            //Invalid input: it used to carry on and dereference the null
            //pointer a few lines below.
            ui->manualValues->setStyleSheet("background : red");
            return;
        }
        frequencies = parsed.value();
        ui->manualValues->setStyleSheet("background : white");

    } else if (ui->modeStack->currentIndex() == 1) { //logspace
        //Both dialogs that ask for a frequency range now ask for it in rad/s,
        //and this is where the conversion happens: tools::logspace takes
        //EXPONENTS. The two used opposite conventions and neither label said
        //which - this one read the field as an exponent while the
        //loop-shaping dialog had its defaults written as values - so the same
        //"0.01" meant 0.01 rad/s in one and 1.02 rad/s in the other.
        //Nothing had to be migrated: every .qft on disk stores a Manual
        //frequency set, for which these two numbers regenerate nothing.
        start = ui->logStart->text().toDouble();
        end = ui->logEnd->text().toDouble();

        //A logarithmic sweep has no zero and no negative end: log10 would
        //answer -infinity or a NaN and the whole set would come out
        //non-finite, several lines below and without saying why.
        if (start <= 0.0 || end <= 0.0){
            errorMessage(tr("A logarithmic range needs both ends greater "
                            "than zero, in rad/s."),
                         tr("Design frequencies input"));
            return;
        }

        frequencies = logspace(std::log10(start), std::log10(end),
                               ui->logCount->text().toInt());
        type = Omega::LogSpace;

    }else if (ui->modeStack->currentIndex() == 2) { //linspace

        frequencies = linspace(ui->linStart->text().toDouble(),ui->linEnd->text().toDouble(),
                               ui->linCount->text().toInt());

        start = ui->linStart->text().toDouble();
        //linStart used to be re-read: every linear Omega was stored with
        //end == start (and travelled like that into the .qft and Bode).
        end = ui->linEnd->text().toDouble();
        type = Omega::LinSpace;

    } else {
        try {
            frequencies = Omega::valuesFromFile(filePath.toStdString());
        } catch (const qftbx::Exception & e) {
            QMessageBox::critical(this, tr("Design frequencies input"), e.what());
            return;
        }
        type = Omega::File;
    }

    //Validated HERE, on the result, so the four modes are covered by one
    //check: a design frequency set must be non-empty and every value must be
    //a positive real. Without it the Omega constructor refused the set by
    //throwing, and that exception escaped this slot and ABORTED the
    //application - pressing OK on a freshly opened dialog was enough.
    if (frequencies.empty()){
        errorMessage(tr("Enter at least one design frequency."),
                     tr("Design frequencies input"));
        return;
    }

    for (const qreal frequency : frequencies){
        if (!std::isfinite(frequency) || frequency <= 0.0){
            errorMessage(tr("A design frequency must be a positive real, and "
                            "%1 is not.").arg(frequency),
                         tr("Design frequencies input"));
            return;
        }
    }

    const qint32 pointCount = frequencies.size();

    m_omega = std::make_unique<Omega>(start, end, pointCount, std::move(frequencies), type);

    accepted = true;

    emit (close_ok());
}


bool FrequenciesDialog::wasAccepted(){
    return accepted;
}

std::unique_ptr<Omega> FrequenciesDialog::takeOmega(){
    return std::move(m_omega);
}
