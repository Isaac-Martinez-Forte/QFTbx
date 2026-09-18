#ifndef QFTBX_MATH_FORMULA_H
#define QFTBX_MATH_FORMULA_H

#include <string>
#include <vector>

namespace qftbx {

struct exp_node;
class ExpressionTree;

/**
 * @brief A formula laid out the way it is written on paper, as against the
 * way it is evaluated.
 *
 * An ExpressionTree is a machine for computing: a division is a node with
 * two branches, and reading it back gives "(s+1)/(s+2)". A reader wants the
 * numerator over the denominator with a rule between them, the exponents
 * raised, the parentheses as tall as what they enclose. That is a different
 * tree, and this is it - built from an expression, from a system of any of
 * the four families, or by hand, and then either drawn (FormulaView) or
 * written out in LaTeX for a paper.
 *
 * Deliberately not a renderer and deliberately free of Qt: the shape of the
 * formula is decided here, once, and the widget only measures fonts and
 * paints.
 */
struct Formula
{
    enum class Kind
    {
        Number,      ///< A literal, upright. text.
        Symbol,      ///< A variable, italic. text, with trailing digits set as a subscript.
        Operator,    ///< A binary operator with air around it. text.
        Juxtaposed,  ///< A product written without a sign: no text, no children.
        Row,         ///< Its parts one after another.
        Fraction,    ///< parts[0] over parts[1].
        Power,       ///< parts[0] raised to parts[1].
        Fenced,      ///< parts[0] inside parentheses that grow with it.
        Function,    ///< text applied to parts[0]: sin, ln, atan.
        Root         ///< The square root of parts[0].
    };

    Kind kind = Kind::Row;
    std::string text;
    std::vector<Formula> parts;
};

/// Builders, so that the composition reads like the formula it builds.
namespace formula {

Formula number(const std::string & text);
/// The value at the significant digits given (see text::number).
Formula number(double value, int digits);
Formula symbol(const std::string & name);
Formula op(const std::string & symbol);
Formula row(std::vector<Formula> parts);
Formula fraction(Formula numerator, Formula denominator);
Formula power(Formula base, Formula exponent);
Formula fenced(Formula inside);
Formula function(const std::string & name, Formula argument);
Formula root(Formula inside);
/// parts[0] between vertical bars, for an absolute value.
Formula bars(Formula inside);
/// parts[0] between square brackets, which is how an interval is written.
Formula bracketed(Formula inside);
/// The interval [min, max] at the digits given.
Formula interval(double minimum, double maximum, int digits);

/**
 * @brief A name split into the stem and the trailing digits drawn as its
 * subscript: "a1" is a with a subscript 1, "kv" is kv, "s" is s.
 *
 * Here and not in the renderer because LaTeX has to make the same cut, and
 * two copies of a rule about names would drift.
 */
void splitName(const std::string & name, std::string & stem, std::string & subscript);

/// a + b, a - b, a * b (juxtaposed unless a sign is needed to be read).
Formula sum(Formula a, Formula b);
Formula difference(Formula a, Formula b);
Formula product(Formula a, Formula b);

/// The product of the factors, empty giving the number 1.
Formula productOf(std::vector<Formula> factors);
/// The sum of the terms, empty giving the number 0. A term that begins
/// with a minus joins with "-" and loses it.
Formula sumOf(std::vector<Formula> terms);

/// Whether the formula has nothing in it (a Row with no parts).
bool isEmpty(const Formula & formula);

} // namespace formula

/**
 * @brief The formula of a parsed expression, with the parentheses the
 * precedence of the operators calls for and no others.
 *
 * A division becomes a fraction, a power an exponent, a square root a sign
 * over its argument: what the text said, not how it was written. Numbers
 * are shown at 'digits' significant digits.
 */
Formula formulaOf(const exp_node & expression, int digits);

/// The formula of a parsed expression, empty when nothing was parsed.
Formula formulaOf(const ExpressionTree & expression, int digits);

/// The formula of an expression as the user typed it, empty when the text
/// is not one the grammar reads.
Formula formulaOfText(const std::string & expression, int digits);

/// The formula written in LaTeX, as the body of a math environment (no $).
std::string latexOf(const Formula & formula);

} // namespace qftbx

#endif // QFTBX_MATH_FORMULA_H
