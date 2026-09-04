#include <string>
#include <vector>
#include <cstdint>
#include <cmath>
#include <complex>

#include "src/core/text_tokens.h"
#include "time_constant_gain.h"

#include "src/core/exception.h"

namespace qftbx {

namespace {

//Every factor of this form is s/z + 1, so a corner frequency of zero divides
//by zero at every frequency - and 0 is finite, so nothing upstream refuses
//it. For an uncertain corner the whole range has to stay clear of zero, or
//the template sweep meets the division at some grid point.
void requireNonZeroCorners(const std::vector<Parameter> & corners, const char * side)
{
    for (const Parameter & corner : corners) {
        const Range range = corner.rawRange();
        const bool touchesZero = corner.isUncertain()
                ? (range.min <= 0.0 && 0.0 <= range.max)
                : corner.rawNominal() == 0.0;
        if (touchesZero) {
            throw InvalidInput(std::string("A time constant in the ") + side
                               + " cannot be zero: every factor is s/z + 1.");
        }
    }
}

} // namespace

TimeConstantGain::TimeConstantGain(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay):
    TransferFunction(name, numerator, denominator,k,delay)
{
    requireNonZeroCorners(m_numerator, "numerator");
    requireNonZeroCorners(m_denominator, "denominator");
}

std::unique_ptr<LtiSystem> TimeConstantGain::create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay, [[maybe_unused]] std::string numeratorExpr, [[maybe_unused]] std::string denominatorExpr){
    return std::make_unique<TimeConstantGain>(name, std::move(numerator), std::move(denominator),
                                              std::move(k), std::move(delay));
}

std::string TimeConstantGain::expression(){
    std::size_t sizeDen = m_denominator.size();
    std::size_t sizeNum = m_numerator.size();

    std::string expr;

    if (m_gain.isUncertain()){
        expr += m_gain.name() + "*(";
    }else {
        expr += qftbx::text::number(m_gain.nominal()) + "*(";
    }


    for (std::size_t i = 0; i + 1 < sizeNum; i++){

        if (m_numerator[i].isUncertain()){
            expr += "(s / " + m_numerator[i].name() + "+1) *";
        } else {
            expr += "(s /" + qftbx::text::number(m_numerator[i].nominal()) + "+1) *";
        }
    }

    if (sizeNum == 0){
        expr += "(1)) / (";
    }else {

        if(m_numerator.back().isUncertain()){
            expr += "(s / " + m_numerator.back().name() + "+1)) / (";
        } else {
            expr += "(s / " + qftbx::text::number(m_numerator.back().nominal()) + "+1)) / (";
        }
    }

    for (std::size_t i = 0; i + 1 < sizeDen; i++){

        if (m_denominator[i].isUncertain()){
            expr += "(s / " + m_denominator[i].name() + "+1) *";
        } else {
            expr += "(s / " + qftbx::text::number(m_denominator[i].nominal()) + "+1) *";
        }
    }

    if (sizeDen == 0){
        expr += "(1))";
    } else {

        if (m_denominator.back().isUncertain()){
            expr += "(s / " + m_denominator.back().name() + "+1))";
        }else{
            expr += "(s /" + qftbx::text::number(m_denominator.back().nominal()) + "+1))";
        }
    }

    if (m_delay.isUncertain()){
        expr += "* e^(-s*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += "* e^(-s*" + qftbx::text::number(m_delay.nominal()) +")";
    }

    return expr;
}

LtiSystem::SystemType TimeConstantGain::type(){
    return SystemType::TimeConstantGain;
}


//P(s) = k * prod(s/tau_n[i] + 1) / prod(s/tau_d[i] + 1) at s = j*w, times
//the pure delay. An empty list is the constant 1, as the expression
//generator writes it. A zero time constant divides by zero here exactly as
//it did in the text.
std::complex <double> TimeConstantGain::valueAt(double w, const std::vector<double> & numerator,
                                               const std::vector<double> & denominator,
                                               double gain, double delay)
{
    const std::complex<double> s(0.0, w);

    std::complex<double> num(1.0, 0.0);
    for (const double constant : numerator) {
        num *= s / constant + 1.0;
    }

    std::complex<double> den(1.0, 0.0);
    for (const double constant : denominator) {
        den *= s / constant + 1.0;
    }

    return gain * num / den * std::exp(-s * delay);
}

} // namespace qftbx
