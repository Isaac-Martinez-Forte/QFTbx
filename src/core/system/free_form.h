#ifndef QFTBX_FREE_FORM_H
#define QFTBX_FREE_FORM_H

#include <string>
#include <vector>
#include <complex>
#include <memory>

#include "src/core/system/transfer_function.h"
#include "src/core/math/expression_tree.h"

/**
 * @brief A plant written as two expressions in the Laplace variable s.
 *
 * The numerator and the denominator are texts the user typed, with the
 * parameters named in them; the expression (numerator)/(denominator) is
 * parsed once, when the system is built, and evaluated at s = j omega with
 * the parameter values bound by position. A parameter cannot be called s.
 */
namespace qftbx {

class FreeForm : public TransferFunction
{
public:
    FreeForm(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay, std::string numeratorExpr,
                 std::string denominatorExpr);

    std::string expression() override;

    std::complex <double> valueAt(double w, const std::vector<double> & numerator,
                                 const std::vector<double> & denominator,
                                 double gain, double delay) override;
    using TransferFunction::evaluate;

    SystemType type() override;

    std::unique_ptr<LtiSystem> create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(double(0)), std::string numeratorExpr = std::string(), std::string denominatorExpr = std::string()) override;

    std::string numeratorString() override;
    std::string denominatorString() override;

    std::unique_ptr<LtiSystem> clone() override;

    /// The Laplace variable as the user writes it: "s".
    static const std::string & laplaceName();

private:
    void bindNames(ExpressionTree & ratio);

    std::string m_numeratorExpr;
    std::string m_denominatorExpr;

    //(numerator)/(denominator), parsed once and bound to the Laplace
    //variable and the distinct parameter names. Shared with the clones,
    //which evaluate the same expression; evaluation reads it only.
    std::shared_ptr<const ExpressionTree> m_ratio;

    //Slot of every numerator and denominator parameter in the value vector
    //valueAt() evaluates with: slot 0 is s, a repeated name shares its slot.
    std::vector<std::size_t> m_numeratorSlots;
    std::vector<std::size_t> m_denominatorSlots;
    std::size_t m_valueCount = 0;
};

} // namespace qftbx

#endif // QFTBX_FREE_FORM_H
