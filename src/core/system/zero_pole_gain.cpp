/**
 * @file
 * @brief Evaluation and poles of the zero-pole-gain form.
 *
 * Evaluation multiplies the factors s + z at s = j omega, the empty product
 * being 1, times the gain and the delay. The stored coefficients are the
 * negated roots, so each denominator entry p is the pole at -p.
 */

#include <string>
#include <vector>
#include <cstdint>
#include <cmath>
#include <complex>

#include "src/core/common/text_tokens.h"
#include "src/core/system/zero_pole_gain.h"

#include "src/core/math/polynomial.h"

namespace qftbx {

ZeroPoleGain::ZeroPoleGain(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay):
    TransferFunction(name, numerator, denominator,k,delay)
{
}

std::unique_ptr<LtiSystem> ZeroPoleGain::create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                             Parameter k, Parameter delay, [[maybe_unused]] std::string numeratorExpr, [[maybe_unused]] std::string denominatorExpr){
    return std::make_unique<ZeroPoleGain>(name, std::move(numerator), std::move(denominator),
                                          std::move(k), std::move(delay));
}

LtiSystem::SystemType ZeroPoleGain::type(){
    return SystemType::ZeroPoleGain;
}

std::string ZeroPoleGain::expression(){
    std::size_t sizeDen = m_denominator.size();
    std::size_t sizeNum = m_numerator.size();

    std::string expr;

    if (m_gain.isUncertain()){
        expr += m_gain.name() + "*(";
    }else {
        expr += qftbx::text::number(m_gain.nominal()) + "*(";
    }

    if (m_numerator.empty()){
        expr += "1) / (";
    } else {
        for (std::size_t i = 0; i + 1 < sizeNum; i++){

            if (m_numerator[i].isUncertain()){
                expr += "(s + " + m_numerator[i].name() + ") *";
            } else {
                expr += "(s +" + qftbx::text::number(m_numerator[i].nominal()) + ") *";
            }
        }

        if(m_numerator.back().isUncertain()){
            expr += "(s + " + m_numerator.back().name() + ")) / (";
        } else {
            expr += "(s + " + qftbx::text::number(m_numerator.back().nominal()) + ")) / (";
        }
    }

    if (m_denominator.empty()){
        expr += "1)";
    }else {
        for (std::size_t i = 0; i + 1 < sizeDen; i++){

            if (m_denominator[i].isUncertain()){
                expr += "(s + " + m_denominator[i].name() + ") *";
            } else {
                expr += "(s + " + qftbx::text::number(m_denominator[i].nominal()) + ") *";
            }
        }

        if (m_denominator.back().isUncertain()){
            expr += "(s + " + m_denominator.back().name() + "))";
        }else{
            expr += "(s + " + qftbx::text::number(m_denominator.back().nominal()) + "))";
        }
    }

    if (m_delay.isUncertain()){
        expr += " * e^(-s*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += " * e^(-s*" + qftbx::text::number(m_delay.nominal()) +")";
    }

    return expr;
}

std::complex <double> ZeroPoleGain::valueAt(double w, const std::vector<double> & numerator,
                                           const std::vector<double> & denominator,
                                           double gain, double delay)
{
    const std::complex<double> s(0.0, w);

    std::complex<double> num(1.0, 0.0);
    for (const double zero : numerator) {
        num *= s + zero;
    }

    std::complex<double> den(1.0, 0.0);
    for (const double pole : denominator) {
        den *= s + pole;
    }

    return gain * num / den * std::exp(-s * delay);
}

std::optional<std::vector<std::complex<double>>> ZeroPoleGain::polesAt(const std::vector<double> &,
                                                                       const std::vector<double> & denominator)
{
    std::vector<std::complex<double>> poles;
    poles.reserve(denominator.size());
    for (const double p : denominator) {
        poles.emplace_back(-p, 0.0);
    }
    return poles;
}

std::optional<LtiSystem::Polynomials> ZeroPoleGain::polynomialsAt(const std::vector<double> & numerator,
                                                                  const std::vector<double> & denominator,
                                                                  double gain)
{
    Polynomials polynomials{{gain}, {1.0}};
    for (const double zero : numerator) {
        polynomials.numerator = math::polynomialProduct(polynomials.numerator, {1.0, zero});
    }
    for (const double pole : denominator) {
        polynomials.denominator = math::polynomialProduct(polynomials.denominator, {1.0, pole});
    }
    return polynomials;
}

}
