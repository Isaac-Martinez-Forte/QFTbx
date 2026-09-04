#include "loop_shaping_dialog.h"
#include "ui_loop_shaping_dialog.h"

#include "src/gui/error_message.h"
#include "src/gui/plot_palette.h"

#include <QLineEdit>
#include <QRadioButton>

#include <cmath>
#include <limits>

using namespace mup;

namespace {

//Evaluating one field, checking the result and painting it green or red was
//written out once per field, which is also why the check was missing from
//every one of them: an expression that PARSES can still be useless. The
//point count went straight into the std::int32_t that linspace takes, so a
//"10^30" was undefined behaviour, and an infinity or a NaN travelled into
//the plot to surface later as an empty diagram.
//The bounds are the caller's: the start and end frequencies are only
//required to be finite, because whether they are frequencies or exponents
//in logarithmic mode is still an open question and this is not the place
//to settle it.
bool readField(ParserX & parser, QLineEdit * field, const QString & complaint,
               double lowest, double highest, double & value)
{
    double parsed = std::numeric_limits<double>::quiet_NaN();

    parser.SetExpr(field->text().toStdString());
    try {
        parsed = parser.Eval().GetFloat();
    } catch (const ParserError &) {
        //Left as a NaN: an expression that does not parse and one that
        //evaluates to nonsense are the same answer to the caller.
    }

    if (!std::isfinite(parsed) || parsed < lowest || parsed > highest) {
        field->setStyleSheet("background : red");
        //The complaint is the caller's words, not a range printed from the
        //bounds: a lower bound of "the smallest positive double" reads as
        //4.94066e-324, which tells a user nothing.
        tools::errorMessage(complaint, QObject::tr("Loop-shaping input"));
        return false;
    }

    field->setStyleSheet("background : white");
    value = parsed;
    return true;
}

}

LoopShapingDialog::LoopShapingDialog(QWidget *parent) :
    StepDialog(parent),
    ui(std::make_unique<Ui::LoopShapingDialog>())
{
    ui->setupUi(this);

    setWindowTitle(tr("Loop-shaping input"));

    //Prefilled from the settings; the window applies them right after
    //construction, and these are what stands until it does.
    applyDefaults(qftbx::Settings().defaults);

    //The epsilon is one field with one label that read only "Epsilon:", and
    //it is not one quantity: every algorithm follows the stopping criterion
    //of ITS paper, which is correct, but for NT/NK/MC1/MC that criterion is
    //the diameter of the NICHOLS box and for MR it is the width of the
    //CONTROLLER's parameter box. With the same figure entered, the Nichols
    //reading is four decades tighter on a plant with |P| = 1e4, and nothing
    //on screen said which one was being asked for.
    for (QRadioButton * radio : {ui->ntRadio, ui->nkRadio, ui->mc1Radio,
                                 ui->mcThesisRadio, ui->mrRadio}) {
        connect(radio, &QRadioButton::toggled,
                this, &LoopShapingDialog::updateEpsilonLabel);
    }

    updateEpsilonLabel();

    linLogSpace = false;

}

LoopShapingDialog::~LoopShapingDialog()
{
}

void LoopShapingDialog::showEvent(QShowEvent * event)
{
    //Reopening and cancelling must not relaunch the computation with the old data.
    QDialog::showEvent(event);
}

void LoopShapingDialog::setEpsilonValue(qreal epsilonEdit){
    ui->epsilonEdit->setText(QString::number(epsilonEdit));
}

void LoopShapingDialog::updateEpsilonLabel()
{
    //MR bisects the controller's parameter box (Rambabu & Nataraj, FDA-10);
    //the other four bisect the Nichols box.
    ui->epsilonLabel->setText(ui->mrRadio->isChecked()
                                  ? tr("Epsilon (controller parameter box width):")
                                  : tr("Epsilon (Nichols box diameter):"));
}

void LoopShapingDialog::on_cancelButton_clicked()
{
    this->close();
}

void LoopShapingDialog::on_okButton_clicked()
{
    ParserX p (mup::pckALL_NON_COMPLEX);

    if (!readField(p, ui->epsilonEdit,
                   tr("The epsilon must be a positive real number."),
                   std::numeric_limits<double>::denorm_min(), m_maxMagnitude,
                   epsilonEdit)){
        return;
    }

    if (!readField(p, ui->startEdit,
                   tr("The start frequency must be a real number."),
                   -m_maxMagnitude, m_maxMagnitude, plotRange.min)){
        return;
    }

    if (!readField(p, ui->endEdit,
                   tr("The end frequency must be a real number."),
                   -m_maxMagnitude, m_maxMagnitude, plotRange.max)){
        return;
    }

    if (!readField(p, ui->pointCountEdit,
                   tr("The point count must be a whole number of at least 1."),
                   1.0, m_maxPointCount, pointCountEdit)){
        return;
    }

    if (ui->nkRadio->isChecked()){

        alg = tools::nk;

        initialisation = ui->upperInit->isChecked() ? 1 : 0;

    } else if (ui->mrRadio->isChecked()){
        alg = tools::mr;
    }else if (ui->mc1Radio->isChecked()){
        alg = tools::mc1;
    } else if (ui->mcThesisRadio->isChecked()){
        alg = tools::mc_thesis;
    } else {
        alg = tools::nt;
    }

    //Direct read: the old latch left linspace selected forever once
    //checked.
    linLogSpace = ui->linspaceRadio->isChecked();

    markAccepted();

    this->close();
}


qreal LoopShapingDialog::epsilonValue(){
    return epsilonEdit;
}

tools::LoopShapingAlgorithm LoopShapingDialog::algorithmValue(){
    return alg;
}

qftbx::Range LoopShapingDialog::range(){
    return plotRange;
}

qreal LoopShapingDialog::pointCountValue(){
    return pointCountEdit;
}

bool LoopShapingDialog::isLinSpace(){
    return linLogSpace;
}

qint32 LoopShapingDialog::initialisationValue(){
    return initialisation;
}

//Both modes prefill the SAME range, which is the configured one. They used
//to write two different hardcoded sets, differing from each other and from
//the one on opening with no reason recorded - and either of them threw away
//whatever the user had typed.
void LoopShapingDialog::on_linspaceRadio_clicked()
{
    applyDefaults(m_defaults);
}

void LoopShapingDialog::on_logspaceRadio_clicked()
{
    applyDefaults(m_defaults);
}

void LoopShapingDialog::applyDefaults(const qftbx::Settings::Defaults & defaults)
{
    m_defaults = defaults;

    ui->startEdit->setText(QString::number(defaults.loopStart));
    ui->endEdit->setText(QString::number(defaults.loopEnd));
    ui->pointCountEdit->setText(QString::number(defaults.loopPointCount));
}

void LoopShapingDialog::on_ntRadio_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

void LoopShapingDialog::on_nkRadio_clicked()
{
    //Page 1 is page_nand, which holds NK's starting-point choice. This
    //asked for page 2 and there are only two pages: Qt ignores an
    //out-of-range index in silence, so the panel was never shown and the
    //choice could not be made - the local search always started at the
    //centre.
    ui->algorithmStack->setCurrentIndex(1);
}

void LoopShapingDialog::on_mrRadio_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

//MC1 and MC (thesis) have no options of their own yet: page 0 is the empty
//one. Without these two the panel kept whatever the previous algorithm had
//put there.
void LoopShapingDialog::on_mc1Radio_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

void LoopShapingDialog::on_mcThesisRadio_clicked()
{
    ui->algorithmStack->setCurrentIndex(0);
}

