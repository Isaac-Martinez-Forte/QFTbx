/*
Roberto C. Cruz Rodríguez
    rcruz@instec.cu
*/
/**
*    @author Roberto C. Cruz Rodríguez <rcruz@instec.cu>
*/
#ifndef QFTBX_LOOPSHAPING_EXPRESSION_TREE_H
#define QFTBX_LOOPSHAPING_EXPRESSION_TREE_H

#include <map>
#include <memory>
#include <string>

#include "interval.hpp"


namespace qftbx {

/**
* Node kinds of the expression interpreter.
*
* CONSTANT, PI, E, VAR are always leaves (they stand for a value);
* ADD..POWER are the binary operators; SIN..SQRT the unary functions.
*/
enum type_node { CONSTANT, PI, E, VAR,
                 PARENTHESIS,
                 ADD, SUBTRACT, MULTIPLY, DIVIDE, POWER,
                 SIN, COS, TAN, SINH, COSH, ATAN, TANH, ASIN,
                 ACOS, EXP, ABS, LN, LG, SQRT };

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
    //Initialised here: the 17 construction sites each assign type, left and
    //right by hand (verified), and the tree walkers dereference the children
    //unconditionally - one forgotten assignment is an indeterminate pointer,
    //not a null one.
    double c_const = 0.0;

    type_node type = {};

    std::string var;

    cxsc::interval enclosure;

    //A node OWNS its branches, so a tree frees itself. There used to be a
    //recursive delete_tree() post-order walk, and every early return of the
    //parser had to have called it.
    std::unique_ptr<exp_node> left;

    std::unique_ptr<exp_node> right;
};

/**
* Binary expression tree over named variables: parses a textual expression
* once and evaluates it over reals or over intervals, and — as the HC4
* hull-consistency filter of algorithm MR — narrows the variable domains
* against the constraint "expression <comparison> value" (propagate).
*
* The lexer accepts identifiers [A-Za-z][A-Za-z0-9_]*, numeric constants
* with scientific notation, the operators + - * / ^ and the unary
* functions listed in type_node; the identifiers E and PI are the
* constants. A malformed expression (unbalanced parentheses, a missing
* operand, a character the grammar has no rule for) and an unknown
* variable at evaluation time throw std::invalid_argument; the historical
* parser read past the end of its stacks on the former and returned an
* UNINITIALISED cxsc::interval on the latter.
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

    /// Prints the tree to stdout (debugging aid).
    void print ();

    ExpressionTree &operator=(const ExpressionTree & other);

    /// Alternative spelling of eval().
    double operator()(std::map<std::string, double> * variables = nullptr);

    cxsc::interval operator() (std::map<std::string, cxsc::interval> *variables);

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
};

//The parser used to carry two hand-written singly-linked stacks, nodeStack
//and operatorStack, a hundred lines with the same interface std::stack has
//(top/pop/push/empty) and four deletes of their own.

}
#endif
