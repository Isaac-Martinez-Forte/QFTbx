/**
 * @file
 * @brief Interpreter of arithmetic expressions over named variables.
 *
 * A free-form plant, an uncertain parameter written as a formula of others
 * and the constraints of algorithm MR are all text. The parser turns it into
 * a binary tree whose leaves are values and variables and whose inner nodes
 * are operations, and the tree evaluates over doubles, complex numbers or
 * intervals. Bound to a constraint, it also narrows the domains of its
 * variables by forward and backward propagation, the HC4 contractor of the
 * constraint-programming literature. Derived from the interpreter by
 * Roberto C. Cruz Rodríguez (rcruz@instec.cu).
 *
 * The grammar takes identifiers, decimal and scientific constants, + - * /
 * and ^ (which binds to the right: 2^3^2 is 2^9), a unary minus, the
 * functions sin, cos, tan, asin, acos, atan, sinh, cosh, tanh, exp, sqrt,
 * abs, ln, log (natural), log10, lg (decimal) and log2, and the constants
 * pi and e in either case; a malformed text, and an unknown variable at
 * evaluation time, throw std::invalid_argument. A usable variable name is
 * any other identifier but s, the Laplace variable a free-form plant binds
 * to j omega. Expression builds the same trees in memory, without a text,
 * and a tree copies it, so one Expression can go into many trees.
 *
 * eval() and propagate() look variables up by name and cache in the
 * nodes, so a tree serves one such call at a time; evaluate() takes the
 * values in the order given to bind(), which must name every variable,
 * and only reads the tree, so threads may share it. propagate() is one
 * HC4 pass against "expression <comparison> value" that narrows the
 * domains in place and returns false when one empties, which proves the
 * box infeasible and is no error. Strict and inclusive comparisons narrow
 * alike, the intervals being closed, and a backward projection that is
 * unsafe or multi-branch is skipped, which narrows nothing and stays
 * sound.
 */

#ifndef QFTBX_MATH_EXPRESSION_TREE_H
#define QFTBX_MATH_EXPRESSION_TREE_H

#include <complex>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "src/core/math/interval.h"

namespace qftbx {

enum type_node { CONSTANT, PI, E, VAR,
                 PARENTHESIS,
                 ADD, SUBTRACT, MULTIPLY, DIVIDE, POWER,
                 SIN, COS, TAN, SINH, COSH, ATAN, TANH, ASIN,
                 ACOS, EXP, ABS, LN, LG, SQRT, LOG2 };

enum com { GREATER, LESS, GREATER_EQUAL, LESS_EQUAL, EQUAL};

struct exp_node
{
    double c_const = 0.0;

    type_node type = {};

    std::string var;

    int index = -1;

    Interval enclosure;

    Interval * slot = nullptr;

    std::unique_ptr<exp_node> left;
    std::unique_ptr<exp_node> right;
};

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

class ExpressionTree
{
public :
    ExpressionTree();

    ExpressionTree(const std::string &text, double num, com comparison);

    ExpressionTree(const char *text);
    explicit ExpressionTree(const std::string & text);

    explicit ExpressionTree(const Expression & expression);
    ExpressionTree(const Expression & expression, double num, com comparison);

    ExpressionTree(const ExpressionTree & other);
    ~ExpressionTree();

    void setFunc(const std::string &text);
    void setFunc(const std::string &text, double result, com comparison);
    void setFunc(const char *text);

    double eval(std::map<std::string, double> * variables = nullptr);

    Interval eval (std::map<std::string, Interval> *variables);

    Interval eval(std::vector<Interval> & values);

    bool propagate (std::map<std::string, Interval> *variables);

    bool propagate(std::vector<Interval> & values);

    void bind(const std::vector<std::string> & names);

    std::vector<std::string> variableNames() const;

    double evaluate(const std::vector<double> & values) const;
    std::complex<double> evaluate(const std::vector<std::complex<double>> & values) const;

    const exp_node * tree() const;

    void print ();

    ExpressionTree &operator=(const ExpressionTree & other);

    double operator()(std::map<std::string, double> * variables = nullptr);
    Interval operator() (std::map<std::string, Interval> *variables);

    static bool isUsableVariableName(const std::string & name);

    static bool isReservedName(const std::string & name);

    static bool isIdentifier(const std::string & name);

    static bool isFunctionName(const std::string & name);

private :
    std::string symbolOf(qftbx::type_node type);
    std::unique_ptr<exp_node> make_cpy(exp_node *node);
    double eval_tree(exp_node *node);
    Interval eval_tree_in (exp_node * node);

    bool eval_tree_out(exp_node * node, Interval enclosure);

    bool safeIntersection(const Interval & a, const Interval & b, Interval & out);

    void build_tree(std::string &in_exp);
    bool isLetter(char text);

    void bindNode(exp_node * node, const std::vector<std::string> & names);
    void collectNames(const exp_node * node, std::vector<std::string> & names) const;
    double evaluateReal(const exp_node * node, const std::vector<double> & values) const;
    std::complex<double> evaluateComplex(const exp_node * node,
                                         const std::vector<std::complex<double>> & values) const;

    std::unique_ptr<exp_node> root;

    std::map<std::string, double> * variables = nullptr;
    std::map<std::string, Interval> * variables_in = nullptr;
    std::vector<Interval> * values_in = nullptr;

    bool propagateLoaded();

    double comparisonValue = 0.0;
    com comparison = GREATER_EQUAL;

    std::vector<std::string> m_boundNames;
};

}

#endif
