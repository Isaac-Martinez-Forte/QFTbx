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

#include <QMap>
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
enum com { MAYOR, MENOR, MAYORIGUAL, MENORIGUAL, IGUAL};

/**
* One node of the binary expression tree: a leaf holds a numeric value
* (c_const) or a variable identifier (var); an inner node holds the
* operation applied to its branches. 'intervalo' caches the interval of
* the subtree during the forward phase of the HC4 filter, so the backward
* phase can project through it.
*/
struct exp_node
{
    double c_const;

    type_node type;

    std::string var;

    interval intervalo;

    exp_node *left;

    exp_node *rigth;
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
    qreal eval(QMap <std::string, qreal> * variables = NULL);

    /// Evaluates over intervals with the given variable domains.
    interval eval (QMap<std::string, interval > *variables);

    /// One HC4 pass over the constraint: forward interval evaluation,
    /// intersection with the constraint set, backward projection narrowing
    /// 'variables' in place. Returns false when a domain empties (the
    /// constraint proves the box infeasible).
    bool propagate (QMap<std::string, interval > *variables);

    /// Prints the tree to stdout (debugging aid).
    void imprimir ();

    ExpressionTree &operator=(const ExpressionTree & other);

    /// Alternative spelling of eval().
    qreal operator()(QMap <std::string, qreal> * variables = NULL);

    interval operator() (QMap<std::string, interval > *variables);

private :

    void alg_exp_node_print (alg::exp_node * node);

    std::string tipo(alg::type_node tipo);

    void delete_tree(exp_node *nod);
    exp_node *make_cpy(exp_node *nod);

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
    bool interseccionSegura(const interval & a, const interval & b, interval & out);

    void build_tree(std::string &in_exp);

    bool es_letra(char tex);

    exp_node *root;
    QMap <std::string, qreal> * variables;
    QMap <std::string, interval > * variables_in;

    qreal numero_comparar;
    com comparacion;

    qreal w;
};

/**
*   Stack of nodes (used by the parser).
*/
class pilaNode
{
public:
    pilaNode() {n=0; head =0;}
    ~pilaNode();
    exp_node* top() {return head->ptr;}
    void      pop();
    void push(exp_node *ptr);
    bool empty() {return (n) ? false : true;}
private:
    struct node
    {
        exp_node *ptr;
        node *next;
    } *head;
    unsigned n;
};

/**
*   Stack of operators (used by the parser).
*/
class pilaOp
{
public:
    pilaOp() {n=0; head = 0;}
    ~pilaOp();
    type_node top() {return head->value;}
    void      pop();
    void push(type_node value);
    bool empty() {return (n) ? false : true;}
private:
    struct node
    {
        type_node value;
        node *next;
    } *head;
    unsigned n;
};

}
#endif
