/**
 * @file
 * @brief A formula as a tree of typeset parts, and its LaTeX.
 *
 * The model behind the drawn formulas of the interface: the formula laid
 * out as it is written on paper, not as it is evaluated. Numbers are set
 * upright, symbols in italic with trailing digits as subscripts, and there
 * are fractions (parts[0] over parts[1]), powers (parts[0] raised to
 * parts[1]), fences that grow with their content, functions (text applied
 * to parts[0]), roots and juxtaposed products. It is free of Qt: the shape
 * is decided here once, and FormulaView only measures fonts and paints.
 *
 * The builders in qftbx::formula compose it; sum, difference and product
 * juxtapose unless a sign is needed to be read, productOf of nothing is 1
 * and sumOf of nothing is 0, and a term that begins with a minus joins
 * with "-" and loses it. An empty formula is a Row with no parts.
 * splitName makes the one cut of a name into stem and subscript ("a1" is a
 * with subscript 1, "kv" stays kv) that both the drawing and LaTeX use.
 * formulaOf converts an expression with the parentheses the precedence
 * requires and no more, numbers at 'digits' significant digits; it and
 * formulaOfText give the empty formula when nothing was parsed. latexOf
 * writes the body of a math environment, without $.
 */

#ifndef QFTBX_MATH_FORMULA_H
#define QFTBX_MATH_FORMULA_H

#include <string>
#include <vector>

namespace qftbx {

struct exp_node;
class ExpressionTree;

struct Formula
{
    enum class Kind
    {
        Number,
        Symbol,
        Operator,
        Juxtaposed,
        Row,
        Fraction,
        Power,
        Fenced,
        Function,
        Root
    };

    Kind kind = Kind::Row;
    std::string text;
    std::vector<Formula> parts;
};

namespace formula {

Formula number(const std::string & text);
Formula number(double value, int digits);
Formula symbol(const std::string & name);
Formula op(const std::string & symbol);
Formula row(std::vector<Formula> parts);
Formula fraction(Formula numerator, Formula denominator);
Formula power(Formula base, Formula exponent);
Formula fenced(Formula inside);
Formula function(const std::string & name, Formula argument);
Formula root(Formula inside);
Formula bars(Formula inside);
Formula bracketed(Formula inside);
Formula interval(double minimum, double maximum, int digits);

void splitName(const std::string & name, std::string & stem, std::string & subscript);

Formula sum(Formula a, Formula b);
Formula difference(Formula a, Formula b);
Formula product(Formula a, Formula b);

Formula productOf(std::vector<Formula> factors);
Formula sumOf(std::vector<Formula> terms);

bool isEmpty(const Formula & formula);

}

Formula formulaOf(const exp_node & expression, int digits);

Formula formulaOf(const ExpressionTree & expression, int digits);

Formula formulaOfText(const std::string & expression, int digits);

std::string latexOf(const Formula & formula);

}

#endif
