/*
Roberto C. Cruz Rodríguez
    rcruz@instec.cu
*/
/**
*    @author Roberto C. Cruz Rodríguez <rcruz@instec.cu>
*/
#ifndef QFTBX_LOOPSHAPING_EXPRESSION_TREE_H
#define QFTBX_LOOPSHAPING_EXPRESSION_TREE_H

#include <cstdlib>
#include <cctype>
#include <cmath>
#include <cassert>
#include <iostream>

#include <map>
#include <memory>
#include <stack>
#include <QRegularExpression>

#include "interval.hpp"

using namespace cxsc;

namespace alg {

/**
* Node kinds of the expression interpreter.
*
* CONS, PI, E, VAR are always leaves (they stand for a value);
* SUMA..POT are the binary operators; SIN..SQRT the unary functions.
*/
enum type_node { CONS, PI, E, VAR,
                 PAR,
                 SUMA, RESTA, MULT, DIV, POT,
                 SIN, COS, TAN, SINH, COSH, ATAN, TANH, ASIN,
                 ACOS, EXP, ABS, LN, LG, SQRT };

/// Comparison attached to a constraint expression (expr >= value, ...).
enum com { GREATER, LESS, GREATER_EQUAL, LESS_EQUAL, IGUAL};

/**
* One node of the binary expression tree: a leaf holds a numeric value
* (c_const) or a variable identifier (var); an inner node holds the
* operation applied to its branches. 'intervalo' caches the interval of
* the subtree during the forward phase of the HC4 filter, so the backward
* phase can project through it.
*/
struct exp_node
{
    //Initialised here: the 17 construction sites each assign type, left and
    //rigth by hand (verified), and the tree walkers dereference the children
    //unconditionally - one forgotten assignment is an indeterminate pointer,
    //not a null one.
    double c_const = 0.0;

    type_node type = {};

    std::string var;

    interval intervalo;

    //A node OWNS its branches, so a tree frees itself. There used to be a
    //recursive delete_tree() post-order walk, and every early return of the
    //parser had to have called it.
    std::unique_ptr<exp_node> left;

    std::unique_ptr<exp_node> rigth;
};

/**
* Binary expression tree over named variables: parses a textual expression
* once and evaluates it over reals or over intervals, and — as the HC4
* hull-consistency filter of algorithm MR — narrows the variable domains
* against the constraint "expression <comparison> value" (propagate).
*
* The lexer accepts identifiers [A-Za-z][A-Za-z0-9_]*, numeric constants
* with scientific notation, the operators + - * / ^ and the unary
* functions listed in type_node. An unknown variable at evaluation time
* throws std::invalid_argument (the historical code returned an
* UNINITIALISED interval).
*/
class ExpressionTree
{
public :
    ExpressionTree();

    /// Parses "tex <comparacion> num" as a constraint (the HC4 use).
    ExpressionTree(const std::string &tex, qreal num, com comparacion);

    /// Parses a plain expression.
    ExpressionTree(const char *tex);

    ExpressionTree(const ExpressionTree & other);
    ~ExpressionTree();

    /// Replaces the parsed expression.
    void setFunc(const std::string &tex);

    /// Replaces the parsed expression and its constraint comparison.
    void setFunc(const std::string &tex, qreal resultado, com comparacion);

    void setFunc(const char *tex);

    /// Evaluates over reals with the given variable values.
    qreal eval(std::map<std::string, qreal> * variables = NULL);

    /// Evaluates over intervals with the given variable domains.
    interval eval (std::map<std::string, interval> *variables);

    /// One HC4 pass over the constraint: forward interval evaluation,
    /// intersection with the constraint set, backward projection narrowing
    /// 'variables' in place. Returns false when a domain empties (the
    /// constraint proves the box infeasible).
    bool propagate (std::map<std::string, interval> *variables);

    /// Prints the tree to stdout (debugging aid).
    void imprimir ();

    ExpressionTree &operator=(const ExpressionTree & other);

    /// Alternative spelling of eval().
    qreal operator()(std::map<std::string, qreal> * variables = NULL);

    interval operator() (std::map<std::string, interval> *variables);

private :

    void alg_exp_node_print (alg::exp_node * node);

    std::string tipo(alg::type_node tipo);

    std::unique_ptr<exp_node> make_cpy(exp_node *nod);

    qreal eval_tree(exp_node *nod);

    interval eval_tree_in (exp_node * nod);

    interval eval_tree_complex_interval(exp_node *nod);

    /// Backward (projection) phase of the HC4 filter: narrows the domains
    /// towards consistency with 'intervalo'. Returns false when a domain
    /// empties (the constraint proves the box inconsistent). Projections
    /// that are unsafe or multi-branch (trigonometric inverses outside a
    /// monotone branch, divisors straddling zero, non-square powers) are
    /// skipped: skipping narrows nothing and stays sound.
    bool eval_tree_out(exp_node * nod, interval intervalo);

    /// Intersection with an empty-signal instead of the historical throw
    /// (the release build compiled the guarding assert out, so an empty
    /// intersection built an invalid interval and aborted the process).
    bool safeIntersection(const interval & a, const interval & b, interval & out);

    void build_tree(std::string &in_exp);

    bool es_letra(char tex);

    std::unique_ptr<exp_node> root;
    //Set at the entry of eval()/propagate() so the recursion does not have
    //to carry them; initialised because not every constructor passes
    //through one of those.
    std::map<std::string, qreal> * variables = nullptr;
    std::map<std::string, interval> * variables_in = nullptr;

    //The constraint propagate() tests against. Two of the four constructors
    //do not set them, and propagate() reads both.
    qreal comparisonValue = 0.0;
    com comparacion = GREATER_EQUAL;

    qreal w = 0.0;
};

//The parser used to carry two hand-written singly-linked stacks, nodeStack
//and operatorStack, a hundred lines with the same interface std::stack has
//(top/pop/push/empty) and four deletes of their own.

}
#endif
