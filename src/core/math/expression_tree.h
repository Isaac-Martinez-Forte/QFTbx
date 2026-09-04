/*
Roberto C. Cruz Rodríguez
    rcruz@instec.cu
*/

/**
*    @author Roberto C. Cruz Rodríguez <rcruz@instec.cu>
*/

#ifndef QFTBX_MATH_EXPRESSION_TREE_H
#define QFTBX_MATH_EXPRESSION_TREE_H

#include <complex>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "interval.hpp"

namespace qftbx {

/**
* Node kinds of the expression interpreter.
*
* CONSTANT, PI, E, VAR are always leaves (they stand for a value);
* ADD..POWER are the binary operators; SIN..LOG2 the unary functions.
*/
enum type_node { CONSTANT, PI, E, VAR,
                 PARENTHESIS,
                 ADD, SUBTRACT, MULTIPLY, DIVIDE, POWER,
                 SIN, COS, TAN, SINH, COSH, ATAN, TANH, ASIN,
                 ACOS, EXP, ABS, LN, LG, SQRT, LOG2 };

/// Comparison attached to a constraint expression (expr >= value, ...).
/// Intervals are closed, so the strict and the inclusive forms narrow the
/// same way.
enum com { GREATER, LESS, GREATER_EQUAL, LESS_EQUAL, EQUAL};

/**
* One node of the binary expression tree: a leaf holds a numeric value
* (c_const) or a variable identifier (var); an inner node holds the
* operation applied to its branches. 'enclosure' caches the cxsc::interval of
* the subtree during the forward phase of the HC4 filter, so the backward
* phase can project through it.
*/
struct exp_node
{
    //Initialised here: the construction sites each assign type, left and
    //right by hand (verified), and the tree walkers dereference the children
    //unconditionally - one forgotten assignment is an indeterminate pointer,
    //not a null one.
    double c_const = 0.0;

    type_node type = {};

    std::string var;

    //Position of the variable in the value vectors of the bound evaluation
    //(see ExpressionTree::bind); -1 while unbound.
    int index = -1;

    cxsc::interval enclosure;

    //Where the variable of a VAR node lives in the map of the evaluation in
    //progress: found once by the forward pass and written through by the
    //backward pass of the same call, instead of looked up by name again.
    //Valid only inside that call.
    cxsc::interval * slot = nullptr;

    //A node OWNS its branches, so a tree frees itself. There used to be a
    //recursive delete_tree() post-order walk, and every early return of the
    //parser had to have called it.
    std::unique_ptr<exp_node> left;
    std::unique_ptr<exp_node> right;
};

/**
* @brief An expression built in memory, node by node, without a text to
* parse.
*
* Constants, variables and the operators and functions of the grammar
* compose into a tree the same way they would in a text, and an
* ExpressionTree is made from the result (copying it, so an Expression can
* be a building block of many trees). Algorithm MR builds its thousands of
* constraints this way instead of formatting every number into a string
* and parsing it back; the text parser stays for what a user types.
*/
class Expression
{
public:
    Expression();
    Expression(double constant);
    Expression(const Expression & other);
    Expression(Expression && other) noexcept;
    Expression & operator=(const Expression & other);
    Expression & operator=(Expression && other) noexcept;
    ~Expression();

    static Expression variable(const std::string & name);
    static Expression pi();
    static Expression e();

    /// A copy of the tree, owned by the caller.
    std::unique_ptr<exp_node> release() const;

    friend Expression operator+(const Expression & a, const Expression & b);
    friend Expression operator-(const Expression & a, const Expression & b);
    friend Expression operator*(const Expression & a, const Expression & b);
    friend Expression operator/(const Expression & a, const Expression & b);
    friend Expression operator-(const Expression & a);
    friend Expression pow(const Expression & base, const Expression & exponent);

    friend Expression sqrt(const Expression & a);
    friend Expression sin(const Expression & a);
    friend Expression cos(const Expression & a);
    friend Expression tan(const Expression & a);
    friend Expression atan(const Expression & a);
    friend Expression exp(const Expression & a);
    friend Expression abs(const Expression & a);
    friend Expression ln(const Expression & a);

private:
    explicit Expression(std::unique_ptr<exp_node> node);
    static Expression binary(type_node type, const Expression & a, const Expression & b);
    static Expression unary(type_node type, const Expression & a);

    std::unique_ptr<exp_node> m_node;
};

/**
* Binary expression tree over named variables: parses a textual expression
* once (or takes one built in memory) and evaluates it over reals, over
* complex numbers or over intervals, and, as the HC4 hull-consistency
* filter of algorithm MR, narrows the variable domains against the
* constraint "expression <comparison> value" (propagate).
*
* The lexer accepts identifiers [A-Za-z][A-Za-z0-9_]*, numeric constants
* with a decimal point and scientific notation, the operators + - * / ^
* (the power binds to the right: 2^3^2 is 2^9), a unary minus, blanks
* anywhere, and the functions sin, cos, tan, asin, acos, atan, sinh, cosh,
* tanh, exp, sqrt, abs, ln, log (natural), log10, lg (decimal) and log2;
* the identifiers pi, PI, e and E are the constants. A malformed expression
* (unbalanced parentheses, a missing operand, a character the grammar has
* no rule for) and an unknown variable at evaluation time throw
* std::invalid_argument.
*
* Two ways to evaluate. The map-based eval()/propagate() look the
* variables up by name and cache per node, which is what the constraint
* propagation needs. The bound evaluate() takes the values in the order the
* names were given to bind(), reads nothing but the tree and writes
* nothing, so several threads may evaluate one tree at once: the template
* sweep does exactly that with the expression of a free-form plant.
*/
class ExpressionTree
{
public :
    ExpressionTree();

    /// Parses "text <comparison> num" as a constraint (the HC4 use).
    ExpressionTree(const std::string &text, double num, com comparison);

    /// Parses a plain expression. A malformed one (unbalanced parentheses,
    /// a missing operand) throws std::invalid_argument, as does an unknown
    /// node kind at evaluation time.
    ExpressionTree(const char *text);
    explicit ExpressionTree(const std::string & text);

    /// Takes an expression built in memory, as a plain expression or as the
    /// constraint "expression <comparison> num".
    explicit ExpressionTree(const Expression & expression);
    ExpressionTree(const Expression & expression, double num, com comparison);

    ExpressionTree(const ExpressionTree & other);
    ~ExpressionTree();

    /// Replaces the parsed expression.
    void setFunc(const std::string &text);
    /// Replaces the parsed expression and its constraint comparison.
    void setFunc(const std::string &text, double result, com comparison);
    void setFunc(const char *text);

    /// Evaluates over reals with the given variable values.
    double eval(std::map<std::string, double> * variables = nullptr);

    /// Evaluates over intervals with the given variable domains.
    cxsc::interval eval (std::map<std::string, cxsc::interval> *variables);

    /// One HC4 pass over the constraint "expression <comparison> value":
    /// forward cxsc::interval evaluation, intersection with the constraint set (a
    /// half-line for the inequalities, a point for the equality), backward
    /// projection narrowing 'variables' in place. Returns false when a
    /// domain empties (the constraint proves the box infeasible). The
    /// comparison used to be stored and never read: every constraint was
    /// propagated as >=.
    bool propagate (std::map<std::string, cxsc::interval> *variables);

    /**
     * @brief Fixes the order of the variables for evaluate().
     *
     * Every variable of the tree must be among the names, in any order,
     * and the values handed to evaluate() follow that order. A variable the
     * names do not cover throws std::invalid_argument naming it. Names that
     * the tree does not use are allowed.
     */
    void bind(const std::vector<std::string> & names);

    /// The names of the variables of the tree, each once, in order of first
    /// appearance.
    std::vector<std::string> variableNames() const;

    /// Evaluation over reals and over complex numbers with the values of
    /// the bound variables. Const and thread-safe: it reads the tree only.
    double evaluate(const std::vector<double> & values) const;
    std::complex<double> evaluate(const std::vector<std::complex<double>> & values) const;

    /// Prints the tree to stdout (debugging aid).
    void print ();

    ExpressionTree &operator=(const ExpressionTree & other);

    /// Alternative spelling of eval().
    double operator()(std::map<std::string, double> * variables = nullptr);
    cxsc::interval operator() (std::map<std::string, cxsc::interval> *variables);

    /**
     * @brief Whether a name can be a variable of an expression.
     *
     * An identifier (a letter, then letters, digits or underscores) that is
     * not a function name, not one of the constants pi and e in either
     * case, and not the Laplace variable s, which the free-form plants bind
     * to j omega. The expression parser this replaces also owned the
     * single letters n, u, m, k, M and G as unit multipliers, which is why
     * the toolbox called its gains "kv".
     */
    static bool isUsableVariableName(const std::string & name);

    /// Whether the name is taken by the grammar: a function, a constant or
    /// the Laplace variable.
    static bool isReservedName(const std::string & name);

    /// Whether the text has the shape of an identifier.
    static bool isIdentifier(const std::string & name);

    /// Whether the name is one of the functions of the grammar.
    static bool isFunctionName(const std::string & name);

private :
    void alg_exp_node_print (qftbx::exp_node * node);
    std::string symbolOf(qftbx::type_node type);
    std::unique_ptr<exp_node> make_cpy(exp_node *node);
    double eval_tree(exp_node *node);
    cxsc::interval eval_tree_in (exp_node * node);

    /// Backward (projection) phase of the HC4 filter: narrows the domains
    /// towards consistency with 'enclosure'. Returns false when a domain
    /// empties (the constraint proves the box inconsistent). Projections
    /// that are unsafe or multi-branch (trigonometric inverses outside a
    /// monotone branch, divisors straddling zero, non-square powers) are
    /// skipped: skipping narrows nothing and stays sound.
    bool eval_tree_out(exp_node * node, cxsc::interval enclosure);

    /// Intersection with an empty-signal instead of the historical throw
    /// (the release build compiled the guarding assert out, so an empty
    /// intersection built an invalid cxsc::interval and aborted the process).
    bool safeIntersection(const cxsc::interval & a, const cxsc::interval & b, cxsc::interval & out);

    void build_tree(std::string &in_exp);
    bool isLetter(char text);

    void bindNode(exp_node * node, const std::vector<std::string> & names);
    void collectNames(const exp_node * node, std::vector<std::string> & names) const;
    double evaluateReal(const exp_node * node, const std::vector<double> & values) const;
    std::complex<double> evaluateComplex(const exp_node * node,
                                         const std::vector<std::complex<double>> & values) const;

    std::unique_ptr<exp_node> root;

    //Set at the entry of eval()/propagate() so the recursion does not have
    //to carry them; initialised because not every constructor passes
    //through one of those.
    std::map<std::string, double> * variables = nullptr;
    std::map<std::string, cxsc::interval> * variables_in = nullptr;

    //The constraint propagate() tests against. Two of the four constructors
    //do not set them, and propagate() reads both.
    double comparisonValue = 0.0;
    com comparison = GREATER_EQUAL;

    //The names bind() was given, in order; empty while unbound.
    std::vector<std::string> m_boundNames;
};

//The parser used to carry two hand-written singly-linked stacks, nodeStack
//and operatorStack, a hundred lines with the same interface std::stack has
//(top/pop/push/empty) and four deletes of their own.

} // namespace qftbx

#endif // QFTBX_MATH_EXPRESSION_TREE_H
