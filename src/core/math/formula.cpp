#include "src/core/math/formula.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "src/core/common/text_tokens.h"
#include "src/core/math/expression_tree.h"

namespace qftbx {

namespace {

//The constants are Numbers and not Symbols because they are set upright:
//pi and e stand for themselves, they are not somebody's variable.
const char kPi[] = "π";

Formula node(Formula::Kind kind, std::string text, std::vector<Formula> parts)
{
    Formula formula;
    formula.kind = kind;
    formula.text = std::move(text);
    formula.parts = std::move(parts);
    return formula;
}

bool startsWithMinus(const Formula & formula)
{
    if (formula.kind == Formula::Kind::Number) {
        return !formula.text.empty() && formula.text.front() == '-';
    }
    if (formula.kind == Formula::Kind::Row) {
        return !formula.parts.empty() && startsWithMinus(formula.parts.front());
    }
    return false;
}

Formula withoutMinus(Formula formula)
{
    if (formula.kind == Formula::Kind::Number) {
        formula.text.erase(formula.text.begin());
        return formula;
    }
    formula.parts.front() = withoutMinus(std::move(formula.parts.front()));
    return formula;
}

//Where the parentheses go when an expression tree becomes a formula: the
//higher the level, the tighter the operator binds. A division is a
//fraction and a function is its own enclosure, so both are as atomic as a
//number - nothing written inside them can need a parenthesis around them.
const int kSum = 1;
const int kProduct = 2;
const int kPower = 3;
//A fraction needs no parentheses among sums and products - it is its own
//enclosure - but it does under an exponent, where the exponent would sit
//against the denominator and read as part of it.
const int kFraction = 4;
const int kAtom = 9;

int levelOf(const exp_node & node)
{
    switch (node.type) {
    case ADD:
    case SUBTRACT:
        return kSum;
    case MULTIPLY:
        return kProduct;
    case POWER:
        return kPower;
    case DIVIDE:
        return kFraction;
    default:
        return kAtom;
    }
}

Formula convert(const exp_node & node, int digits);

//An operand of an operator that binds at 'minimum': parenthesised when it
//binds more loosely, and when it is a negative literal that would otherwise
//end up written against the sign of the operator before it. The FIRST
//operand has no sign before it, so it keeps its minus bare: the parser
//reads a unary minus as a product by -1, and -s must not come out as
//(-1)s.
Formula operand(const exp_node & node, int digits, int minimum, bool leading = false)
{
    Formula inside = convert(node, digits);

    if (levelOf(node) < minimum
            || (!leading && minimum > kSum && startsWithMinus(inside))) {
        return formula::fenced(std::move(inside));
    }

    return inside;
}

std::string functionName(type_node type)
{
    switch (type) {
    case SIN:  return "sin";
    case COS:  return "cos";
    case TAN:  return "tan";
    case SINH: return "sinh";
    case COSH: return "cosh";
    case TANH: return "tanh";
    case ASIN: return "arcsin";
    case ACOS: return "arccos";
    case ATAN: return "arctan";
    case LN:   return "ln";
    case LG:   return "log10";
    case LOG2: return "log2";
    default:   break;
    }

    return std::string();
}

Formula convert(const exp_node & node, int digits)
{
    switch (node.type) {
    case CONSTANT:
        return formula::number(node.c_const, digits);
    case PI:
        return formula::number(kPi);
    case E:
        return formula::number("e");
    case VAR:
        return formula::symbol(node.var);

    case ADD:
        return formula::sum(operand(*node.left, digits, kSum),
                            operand(*node.right, digits, kSum));
    case SUBTRACT:
        //The right operand of a subtraction needs its parentheses even at
        //its own level: a - (b - c) is not a - b - c.
        return formula::difference(operand(*node.left, digits, kSum),
                                   operand(*node.right, digits, kSum + 1));
    case MULTIPLY:
        return formula::product(operand(*node.left, digits, kProduct, true),
                                operand(*node.right, digits, kProduct));
    case DIVIDE:
        //A fraction encloses its own two halves.
        return formula::fraction(convert(*node.left, digits),
                                 convert(*node.right, digits));
    case POWER:
        //The exponent is drawn small and raised, which fences it; the base
        //is fenced unless it is an atom.
        return formula::power(operand(*node.left, digits, kAtom),
                              convert(*node.right, digits));

    case EXP:
        return formula::power(formula::number("e"), convert(*node.left, digits));
    case SQRT:
        return formula::root(convert(*node.left, digits));
    case ABS:
        return formula::bars(convert(*node.left, digits));

    case PARENTHESIS:
        //Never reaches a tree: the parser uses it as a mark on its operator
        //stack. Answered anyway, so that a hand-built tree cannot fall
        //through to the empty formula.
        return node.left ? convert(*node.left, digits) : Formula();

    default:
        break;
    }

    const std::string name = functionName(node.type);
    if (name.empty()) {
        return Formula();
    }

    return formula::function(name, convert(*node.left, digits));
}

//The exponent of a number written in scientific notation, as LaTeX writes
//it: "1.5e-06" is 1.5 times ten to the minus sixth, not the letter e.
bool splitExponent(const std::string & text, std::string & mantissa, std::string & exponent)
{
    const std::size_t mark = text.find_first_of("eE");
    if (mark == std::string::npos || mark + 1 >= text.size()) {
        return false;
    }

    mantissa = text.substr(0, mark);
    exponent = text.substr(mark + 1);

    //A leading plus and the zeros that pad the exponent are printer's
    //habits, not part of the number.
    if (!exponent.empty() && (exponent.front() == '+' || exponent.front() == '-')) {
        const char sign = exponent.front();
        exponent.erase(exponent.begin());
        while (exponent.size() > 1 && exponent.front() == '0') {
            exponent.erase(exponent.begin());
        }
        if (sign == '-') {
            exponent.insert(exponent.begin(), '-');
        }
    }

    return !mantissa.empty() && !exponent.empty();
}

std::string latexNumber(const std::string & text)
{
    if (text == kPi) {
        return "\\pi";
    }

    std::string mantissa;
    std::string exponent;
    if (splitExponent(text, mantissa, exponent)) {
        if (mantissa == "1") {
            return "10^{" + exponent + "}";
        }
        return mantissa + " \\cdot 10^{" + exponent + "}";
    }

    return text;
}

std::string latexSymbol(const std::string & name)
{
    std::string stem;
    std::string subscript;
    formula::splitName(name, stem, subscript);

    //A single letter is already the italic of a formula; a name is not, and
    //LaTeX would set it as a product of its letters.
    std::string latex = stem.size() > 1 ? "\\mathit{" + stem + "}" : stem;

    if (!subscript.empty()) {
        latex += "_{" + subscript + "}";
    }

    return latex;
}

//"\pi" and "s" written one after the other are the command \pis, which
//does not exist. A space between them is the whole fix, and it is needed
//only when a command with a letter name meets a letter.
bool endsWithCommand(const std::string & latex)
{
    if (latex.empty() || std::isalpha(static_cast<unsigned char>(latex.back())) == 0) {
        return false;
    }

    const std::size_t mark = latex.find_last_of('\\');
    if (mark == std::string::npos) {
        return false;
    }

    for (std::size_t i = mark + 1; i < latex.size(); ++i) {
        if (std::isalpha(static_cast<unsigned char>(latex[i])) == 0) {
            return false;
        }
    }

    return true;
}

std::string latexOperator(const std::string & symbol)
{
    if (symbol == "*") {
        return " \\cdot ";
    }
    if (symbol == "\u2264") {
        return " \\leq ";
    }
    if (symbol == "\u2265") {
        return " \\geq ";
    }
    return " " + symbol + " ";
}

std::string latexFunction(const std::string & name)
{
    std::string stem;
    std::string subscript;
    formula::splitName(name, stem, subscript);

    static const char * const known[] = {"sin", "cos", "tan", "sinh", "cosh", "tanh",
                                         "arcsin", "arccos", "arctan", "ln", "log", "exp"};

    const bool isKnown = std::find_if(std::begin(known), std::end(known),
                                      [&stem](const char * candidate) {
                                          return stem == candidate;
                                      }) != std::end(known);

    std::string latex = isKnown ? "\\" + stem : "\\operatorname{" + stem + "}";

    if (!subscript.empty()) {
        latex += "_{" + subscript + "}";
    }

    return latex;
}

} // namespace

namespace formula {

Formula number(const std::string & text)
{
    return node(Formula::Kind::Number, text, {});
}

Formula number(double value, int digits)
{
    return number(text::number(value, digits));
}

Formula symbol(const std::string & name)
{
    return node(Formula::Kind::Symbol, name, {});
}

Formula op(const std::string & symbol)
{
    return node(Formula::Kind::Operator, symbol, {});
}

Formula row(std::vector<Formula> parts)
{
    return node(Formula::Kind::Row, std::string(), std::move(parts));
}

Formula fraction(Formula numerator, Formula denominator)
{
    return node(Formula::Kind::Fraction, std::string(),
                {std::move(numerator), std::move(denominator)});
}

Formula power(Formula base, Formula exponent)
{
    return node(Formula::Kind::Power, std::string(), {std::move(base), std::move(exponent)});
}

Formula fenced(Formula inside)
{
    return node(Formula::Kind::Fenced, "(", {std::move(inside)});
}

Formula bars(Formula inside)
{
    return node(Formula::Kind::Fenced, "|", {std::move(inside)});
}

Formula bracketed(Formula inside)
{
    return node(Formula::Kind::Fenced, "[", {std::move(inside)});
}

Formula interval(double minimum, double maximum, int digits)
{
    //The comma belongs to the number before it, which is why it is not an
    //operator: an operator is written with air on both sides.
    return bracketed(row({number(minimum, digits), number(", "), number(maximum, digits)}));
}

Formula function(const std::string & name, Formula argument)
{
    return node(Formula::Kind::Function, name, {std::move(argument)});
}

Formula root(Formula inside)
{
    return node(Formula::Kind::Root, std::string(), {std::move(inside)});
}

Formula sum(Formula a, Formula b)
{
    //A term that carries its own minus is added by subtracting it, which is
    //how it would be written by hand.
    if (startsWithMinus(b)) {
        return difference(std::move(a), withoutMinus(std::move(b)));
    }

    return row({std::move(a), op("+"), std::move(b)});
}

Formula difference(Formula a, Formula b)
{
    return row({std::move(a), op("-"), std::move(b)});
}

Formula product(Formula a, Formula b)
{
    //The parser has no unary minus: it reads -s as (-1)*s, and that is what
    //would be drawn if the sign were not read back here.
    if (a.kind == Formula::Kind::Number && a.text == "-1") {
        return row({number("-"), std::move(b)});
    }

    //Two numbers side by side would read as one, and so would a number
    //after a symbol: those are the products that need their sign. The rest
    //are written the way they are read, k(s+1) and not k*(s+1).
    const bool needsSign = b.kind == Formula::Kind::Number
            || (a.kind == Formula::Kind::Number && b.kind == Formula::Kind::Fraction);

    return row({std::move(a),
                needsSign ? op("*") : node(Formula::Kind::Juxtaposed, std::string(), {}),
                std::move(b)});
}

Formula productOf(std::vector<Formula> factors)
{
    if (factors.empty()) {
        return number("1");
    }

    Formula result = std::move(factors.front());
    for (std::size_t i = 1; i < factors.size(); ++i) {
        result = product(std::move(result), std::move(factors[i]));
    }

    return result;
}

Formula sumOf(std::vector<Formula> terms)
{
    if (terms.empty()) {
        return number("0");
    }

    Formula result = std::move(terms.front());
    for (std::size_t i = 1; i < terms.size(); ++i) {
        result = sum(std::move(result), std::move(terms[i]));
    }

    return result;
}

bool isEmpty(const Formula & formula)
{
    return formula.kind == Formula::Kind::Row && formula.parts.empty();
}

void splitName(const std::string & name, std::string & stem, std::string & subscript)
{
    std::size_t cut = name.size();
    while (cut > 1 && std::isdigit(static_cast<unsigned char>(name[cut - 1]))) {
        --cut;
    }

    stem = name.substr(0, cut);
    subscript = name.substr(cut);
}

} // namespace formula

Formula formulaOf(const exp_node & expression, int digits)
{
    return convert(expression, digits);
}

Formula formulaOf(const ExpressionTree & expression, int digits)
{
    const exp_node * root = expression.tree();

    return root ? convert(*root, digits) : Formula();
}

Formula formulaOfText(const std::string & expression, int digits)
{
    try {
        return formulaOf(ExpressionTree(expression), digits);
    } catch (const std::invalid_argument &) {
        return Formula();
    }
}

std::string latexOf(const Formula & formula)
{
    switch (formula.kind) {
    case Formula::Kind::Number:
        return latexNumber(formula.text);
    case Formula::Kind::Symbol:
        return latexSymbol(formula.text);
    case Formula::Kind::Operator:
        return latexOperator(formula.text);
    case Formula::Kind::Juxtaposed:
        return std::string();

    case Formula::Kind::Row: {
        std::string latex;
        for (const Formula & part : formula.parts) {
            const std::string piece = latexOf(part);
            if (!piece.empty() && std::isalpha(static_cast<unsigned char>(piece.front())) != 0
                    && endsWithCommand(latex)) {
                latex += ' ';
            }
            latex += piece;
        }
        return latex;
    }

    case Formula::Kind::Fraction:
        return "\\frac{" + latexOf(formula.parts[0]) + "}{" + latexOf(formula.parts[1]) + "}";

    case Formula::Kind::Power:
        return latexOf(formula.parts[0]) + "^{" + latexOf(formula.parts[1]) + "}";

    case Formula::Kind::Fenced:
        if (formula.text == "|") {
            return "\\left|" + latexOf(formula.parts[0]) + "\\right|";
        }
        if (formula.text == "[") {
            return "\\left[" + latexOf(formula.parts[0]) + "\\right]";
        }
        return "\\left(" + latexOf(formula.parts[0]) + "\\right)";

    case Formula::Kind::Function:
        return latexFunction(formula.text) + "\\left(" + latexOf(formula.parts[0]) + "\\right)";

    case Formula::Kind::Root:
        return "\\sqrt{" + latexOf(formula.parts[0]) + "}";
    }

    return std::string();
}

} // namespace qftbx
