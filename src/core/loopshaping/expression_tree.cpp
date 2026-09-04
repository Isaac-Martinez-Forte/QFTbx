/*
Roberto C. Cruz Rodríguez
    rcruz@instec.cu
*/
// This function interpreter builds a binary expression tree in which every
// inner node is an operation and every leaf a value.

#include "src/core/loopshaping/expression_tree.h"
#include "src/core/math/constants.h"

#include <cctype>
#include <cmath>
#include <iostream>
#include <stack>
#include <stdexcept>
#include <string>

#include "imath.hpp"

using namespace std;
using namespace cxsc;


namespace alg {

namespace {

//The lexer reads the expression without blanks.
std::string withoutSpaces(const std::string & text)
{
    std::string stripped;
    stripped.reserve(text.size());
    for (const char c : text) {
        if (c != ' ') {
            stripped.push_back(c);
        }
    }
    return stripped;
}

//Enclosures of the two constants, not the nearest doubles: an interval
//evaluation that returned a degenerate [3.14159...] did not contain pi,
//and the e it returned had twelve correct digits.
interval piEnclosure()
{
    return Pi();
}

interval eEnclosure()
{
    return exp(interval(1.0));
}

} // namespace

//////////////////////////////////////////
// ExpressionTree implementation        //
//////////////////////////////////////////
ExpressionTree::ExpressionTree()
{
    root = nullptr;
}

ExpressionTree::ExpressionTree(const char *tex)
{
    root = nullptr;
    std::string in_exp = withoutSpaces(tex);

    build_tree(in_exp);
}

ExpressionTree::ExpressionTree(const std::string &tex, double resultado, com comparacion)
{
    root = nullptr;
    std::string in_exp = withoutSpaces(tex);

    this->comparisonValue = resultado;
    this->comparacion = comparacion;

    build_tree(in_exp);
}

ExpressionTree::ExpressionTree(const ExpressionTree &other )
    : root(make_cpy(other.root.get())),
      comparisonValue(other.comparisonValue),
      comparacion(other.comparacion)
{
}

//The root owns the tree, and every node owns its branches: the recursive
//post-order delete_tree() this used to call has nothing left to do.
ExpressionTree::~ExpressionTree() = default;

/********************************************************
* void ExpressionTree::setFunc(const std::string &tex)        *
*********************************************************
* Discards the current tree and builds a new one from the given
* expression.
*/

void ExpressionTree::setFunc(const std::string &tex)
{
    std::string in_exp = withoutSpaces(tex);

    comparisonValue = 0;
    comparacion = GREATER_EQUAL;

    build_tree(in_exp);
}

void ExpressionTree::setFunc(const std::string &tex, double resultado, com comparacion)
{
    std::string in_exp = withoutSpaces(tex);

    this->comparisonValue = resultado;
    this->comparacion = comparacion;

    build_tree(in_exp);
}

/********************************************************
* void ExpressionTree::setFunc(const char *tex)               *
*********************************************************
* As above, taking a C string.
*/
void ExpressionTree::setFunc(const char *tex)
{
    std::string in_exp = withoutSpaces(tex);

    build_tree(in_exp);
}

/********************************************************
* ExpressionTree &ExpressionTree::operator=(const ExpressionTree &other)  *
*********************************************************
* Evaluates the expression.
*/
double ExpressionTree::eval(std::map<std::string, double> *variables )
{
    this->variables = variables;

    return eval_tree(root.get());
}

void ExpressionTree::imprimir (){
    alg_exp_node_print(root.get());
}


/*
 * enum type_node { CONST, PI, E, VAR,
                 PAR,
                 SUMA, RESTA, MULT, DIV, POT,
                 SIN, COS, TAN, SINH, COSH, ATAN, TANH, ASIN,
                 ACOS, EXP, ABS, LN, LG, SQRT
               };
 *
 */

void ExpressionTree::alg_exp_node_print(exp_node * node){



    if (node->type == CONS){
        cout << node->c_const << endl;
    } else if (node->type == PI){
        cout << " pi " << endl;
    } else if (node->type == E){
        cout << " e " << endl;
    } else if (node->type == VAR){
        cout << node->var << endl;
    }else if (node->type == POT){
        alg_exp_node_print(node->left.get());
        cout << tipo(node->type);
        cout << "2" << endl;
    }else if (node->type > POT){
        cout << tipo(node->type);
        alg_exp_node_print(node->left.get());
    } else {
        alg_exp_node_print(node->left.get());
        cout << tipo(node->type);
        alg_exp_node_print(node->rigth.get());
    }

}

string ExpressionTree::tipo(type_node tipo)  {

    if (tipo == PI){
        return " PI ";
    }

    if(tipo == E){
        return " E ";
    }

    if (tipo == SUMA){
        return " + ";
    }

    if (tipo == RESTA){
        return " - ";
    }

    if (tipo == MULT){
        return " * ";
    }

    if (tipo == DIV){
        return " / ";
    }

    if (tipo == POT){
        return " ^ ";
    }

    if (tipo == SIN){
        return " sin ";
    }

    if (tipo == COS){
        return " cos ";
    }

    if (tipo == TAN){
        return " tan ";
    }

    if (tipo == SINH){
        return " sinh ";
    }

    if (tipo == COSH){
        return " cosh ";
    }

    if (tipo == ATAN){
        return " atan ";
    }

    if (tipo == TANH){
        return " tanh ";
    }

    if (tipo == ASIN){
        return " asin ";
    }

    if (tipo == ACOS){
        return " acos ";
    }

    if (tipo == EXP){
        return " e ";
    }

    if (tipo == ABS){
        return " abs ";
    }

    if (tipo == LN){
        return " ln ";
    }

    if (tipo == LG){
        return " log ";
    }

    if (tipo == SQRT){
        return " sqrt ";
    }

    return "";
}

interval ExpressionTree::eval(std::map<string, interval> *variables){
    this->variables_in = variables;

    return eval_tree_in(root.get());
}

//Assignment: a deep copy, so the two trees own separate nodes.
ExpressionTree &ExpressionTree::operator=(const ExpressionTree &other)
{
    //Falling off the end of a value-returning function is undefined
    //behaviour (the historical version did, flagged by every build).
    if (this != &other) {
        root = make_cpy(other.root.get());
        comparisonValue = other.comparisonValue;
        comparacion = other.comparacion;
    }

    return *this;
}

/*****************************************************************************
* double ExpressionTree::operator()(double xx , double yy , double zz , double tt) *
******************************************************************************
* la homonimia del operador '()' nos permite
* evaluar la expresion usando parentesis
* de esta forma result = function(78,0,0,1)
*/
double ExpressionTree::operator()(std::map<std::string, double> * variables)
{
    return eval (variables);
}

interval ExpressionTree::operator ()(std::map<std::string, interval> * variables){
    return eval (variables);
}

//The core of the class: a recursive walk over the binary expression tree,
//applying each node's operation and returning the value of the whole
//expression.
double ExpressionTree::eval_tree(exp_node *nod)
{

    switch (nod->type)
    {
    case CONS :
        return nod->c_const;

    case VAR  :
        return variables->at(nod->var);

    case E:
        return qftbx::math::kE;

    case PI:
        return qftbx::math::kPi;

    case SUMA :
        return eval_tree(nod->left.get()) + eval_tree(nod->rigth.get());

    case RESTA :
        return eval_tree(nod->left.get()) - eval_tree(nod->rigth.get());

    case MULT :
        return eval_tree(nod->left.get()) * eval_tree(nod->rigth.get());

    case DIV :
        return eval_tree(nod->left.get()) / eval_tree(nod->rigth.get());

    case POT :
        return pow (eval_tree(nod->left.get()),  eval_tree(nod->rigth.get()) );

    case SIN :
        return sin ( eval_tree(nod->left.get()) );

    case COS :
        return cos ( eval_tree(nod->left.get()) );

    case SQRT :
        return sqrt( eval_tree(nod->left.get()) );

    case TAN :
        return tan ( eval_tree(nod->left.get()) );

    case ATAN :
        return atan ( eval_tree(nod->left.get()) );

    case SINH :
        return sinh ( eval_tree(nod->left.get()) );

    case COSH :
        return cosh ( eval_tree(nod->left.get()) );

    case TANH :
        return tanh ( eval_tree(nod->left.get()) );

    case ASIN :
        return asin ( eval_tree(nod->left.get()) );

    case ACOS :
        return acos ( eval_tree(nod->left.get()) );

    case EXP :
        return exp ( eval_tree(nod->left.get()) );

    case ABS :
        return fabs ( eval_tree(nod->left.get()) );

    case LN :
        return log ( eval_tree(nod->left.get()) );

    case LG :
        return log10 ( eval_tree(nod->left.get()) );

        /* if another function was added, add its 'case' here with its operation */

    default:
        throw std::invalid_argument("ExpressionTree: a node the evaluator does not know.");
    }
}


bool ExpressionTree::propagate(std::map<string, interval> *variables){

    this->variables_in = variables;

    const interval resultado = eval_tree_in(root.get());

    //The part of the forward value that satisfies the constraint. Empty
    //means the box is infeasible; the whole value means there is nothing
    //to narrow. The comparison used to be stored and ignored: every
    //constraint was treated as ">=", which is the one MR builds.
    interval nuevo_intervalo;

    switch (comparacion) {
    case GREATER:
    case GREATER_EQUAL:
        if (Inf(resultado) > comparisonValue) {
            return true;
        }
        if (Sup(resultado) < comparisonValue) {
            return false;
        }
        nuevo_intervalo = interval(comparisonValue, Sup(resultado));
        break;
    case LESS:
    case LESS_EQUAL:
        if (Sup(resultado) < comparisonValue) {
            return true;
        }
        if (Inf(resultado) > comparisonValue) {
            return false;
        }
        nuevo_intervalo = interval(Inf(resultado), comparisonValue);
        break;
    case IGUAL:
        if (Inf(resultado) > comparisonValue || Sup(resultado) < comparisonValue) {
            return false;
        }
        nuevo_intervalo = interval(comparisonValue);
        break;
    }

    //A domain emptied during the backward projection proves the box
    //inconsistent with the constraint.
    return eval_tree_out(root.get(), nuevo_intervalo);
}

bool ExpressionTree::safeIntersection(const interval & a, const interval & b, interval & out){

    const real low = (Inf(a) > Inf(b)) ? Inf(a) : Inf(b);
    const real high = (Sup(a) < Sup(b)) ? Sup(a) : Sup(b);

    if (low > high) {
        return false;
    }

    out = interval(low, high);
    return true;
}

//Backward (projection) phase of the HC4 filter. Every child projection is
//intersected with the child's forward value; an empty intersection proves
//the whole box inconsistent (return false). Unsafe projections are
//skipped rather than risked: the historical version fed negative ranges
//to pow (ln of a negative aborts inside the noexcept library), called
//acos/asin outside [-1, 1], divided by intervals straddling zero and
//treated multi-branch trigonometric inverses as single-branch.
bool ExpressionTree::eval_tree_out(exp_node *nod, interval intervalo){

    interval candidato;

    switch (nod->type)
    {

    case CONS :
        return true;

    case VAR  :
    {
        (*variables_in)[nod->var] = intervalo;
        return true;
    }

    case SUMA :
    {
        interval a;
        if (!safeIntersection(nod->left->intervalo, intervalo - nod->rigth->intervalo, a)) {
            return false;
        }
        if (!eval_tree_out(nod->left.get(), a)) {
            return false;
        }

        interval b;
        if (!safeIntersection(nod->rigth->intervalo, intervalo - a, b)) {
            return false;
        }
        return eval_tree_out(nod->rigth.get(), b);
    }

    case RESTA :
    {
        interval a;
        if (!safeIntersection(nod->left->intervalo, intervalo + nod->rigth->intervalo, a)) {
            return false;
        }
        if (!eval_tree_out(nod->left.get(), a)) {
            return false;
        }

        interval b;
        if (!safeIntersection(nod->rigth->intervalo, a - intervalo, b)) {
            return false;
        }
        return eval_tree_out(nod->rigth.get(), b);
    }

    case MULT :
    {
        //Each factor projects as a quotient: only when the divisor does
        //not straddle zero (interval division would abort otherwise).
        interval a = nod->left->intervalo;

        if (Inf(nod->rigth->intervalo) > 0.0 || Sup(nod->rigth->intervalo) < 0.0) {
            if (!safeIntersection(a, intervalo / nod->rigth->intervalo, a)) {
                return false;
            }
            if (!eval_tree_out(nod->left.get(), a)) {
                return false;
            }
        }

        if (Inf(a) > 0.0 || Sup(a) < 0.0) {
            interval b;
            if (!safeIntersection(nod->rigth->intervalo, intervalo / a, b)) {
                return false;
            }
            return eval_tree_out(nod->rigth.get(), b);
        }

        return true;
    }

    case DIV :
    {
        interval a;
        if (!safeIntersection(nod->left->intervalo, intervalo * nod->rigth->intervalo, a)) {
            return false;
        }
        if (!eval_tree_out(nod->left.get(), a)) {
            return false;
        }

        if (Inf(intervalo) > 0.0 || Sup(intervalo) < 0.0) {
            interval b;
            if (!safeIntersection(nod->rigth->intervalo, a / intervalo, b)) {
                return false;
            }
            return eval_tree_out(nod->rigth.get(), b);
        }

        return true;
    }

    case POT :
    {
        //Only the square is projected (the constraint grammar uses no
        //other exponent): x^2 = I implies x in +-sqrt(I intersect [0,inf)).
        if (Inf(nod->rigth->intervalo) != 2.0) {
            return true;
        }

        interval positivo;
        if (!safeIntersection(intervalo, interval(0.0, MaxReal), positivo)) {
            return false;
        }

        const interval raiz = sqrt(positivo);
        if (!safeIntersection(nod->left->intervalo,
                                interval(-Sup(raiz), Sup(raiz)), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left.get(), candidato);
    }

    case SQRT :
    {
        //sqrt(x) = I: the result is never negative.
        interval positivo;
        if (!safeIntersection(intervalo, interval(0.0, MaxReal), positivo)) {
            return false;
        }

        if (!safeIntersection(nod->left->intervalo, sqr(positivo), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left.get(), candidato);
    }

    case SIN :
    {
        interval acotado;
        if (!safeIntersection(intervalo, interval(-1.0, 1.0), acotado)) {
            return false;
        }

        //Only within the principal monotone branch of the argument.
        if (Inf(nod->left->intervalo) < -qftbx::math::kPi / 2 || Sup(nod->left->intervalo) > qftbx::math::kPi / 2) {
            return true;
        }

        if (!safeIntersection(nod->left->intervalo, asin(acotado), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left.get(), candidato);
    }

    case COS :
    {
        interval acotado;
        if (!safeIntersection(intervalo, interval(-1.0, 1.0), acotado)) {
            return false;
        }

        //cos is monotone on [-pi, 0] and on [0, pi]; anything wider is
        //left unprojected.
        if (Inf(nod->left->intervalo) >= -qftbx::math::kPi && Sup(nod->left->intervalo) <= 0.0) {
            if (!safeIntersection(nod->left->intervalo, -acos(acotado), candidato)) {
                return false;
            }
            return eval_tree_out(nod->left.get(), candidato);
        }

        if (Inf(nod->left->intervalo) >= 0.0 && Sup(nod->left->intervalo) <= qftbx::math::kPi) {
            if (!safeIntersection(nod->left->intervalo, acos(acotado), candidato)) {
                return false;
            }
            return eval_tree_out(nod->left.get(), candidato);
        }

        return true;
    }

    case TAN :
    {
        if (Inf(nod->left->intervalo) <= -qftbx::math::kPi / 2 || Sup(nod->left->intervalo) >= qftbx::math::kPi / 2) {
            return true;
        }

        if (!safeIntersection(nod->left->intervalo, atan(intervalo), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left.get(), candidato);
    }

    case ATAN :
    {
        interval acotado;
        if (!safeIntersection(intervalo, interval(-qftbx::math::kPi / 2, qftbx::math::kPi / 2), acotado)) {
            return false;
        }

        if (!safeIntersection(nod->left->intervalo, tan(acotado), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left.get(), candidato);
    }

    case ASIN :
    {
        interval acotado;
        if (!safeIntersection(intervalo, interval(-qftbx::math::kPi / 2, qftbx::math::kPi / 2), acotado)) {
            return false;
        }

        if (!safeIntersection(nod->left->intervalo, sin(acotado), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left.get(), candidato);
    }

    case ACOS :
    {
        interval acotado;
        if (!safeIntersection(intervalo, interval(0.0, qftbx::math::kPi), acotado)) {
            return false;
        }

        if (!safeIntersection(nod->left->intervalo, cos(acotado), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left.get(), candidato);
    }

    case ABS :
    {
        //|x| = I: x in [-Sup(I+), Sup(I+)].
        interval positivo;
        if (!safeIntersection(intervalo, interval(0.0, MaxReal), positivo)) {
            return false;
        }

        if (!safeIntersection(nod->left->intervalo,
                                interval(-Sup(positivo), Sup(positivo)), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left.get(), candidato);
    }

    case LN :
    {
        //exp overflows past ~709: skip rather than trap.
        if (Sup(intervalo) > 700.0) {
            return true;
        }

        if (!safeIntersection(nod->left->intervalo, exp(intervalo), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left.get(), candidato);
    }

    default:
        return true;
    }
}

interval ExpressionTree::eval_tree_in(exp_node *nod)
{

    switch (nod->type)
    {
    case CONS :
        return nod->intervalo = interval (nod->c_const);

    case VAR  :
        //A missing variable used to return a default-constructed cxsc
        //interval, whose bounds are UNINITIALIZED memory.
        if (variables_in->find(nod->var) == variables_in->end()) {
            throw std::invalid_argument(
                    "ExpressionTree: unknown variable '" + nod->var + "' in the expression.");
        }
        return nod->intervalo = variables_in->at(nod->var);

    case E:
        return nod->intervalo = eEnclosure();

    case PI:
        return nod->intervalo = piEnclosure();

    case SUMA :
        return nod->intervalo = eval_tree_in(nod->left.get()) + eval_tree_in(nod->rigth.get());

    case RESTA :
        return nod->intervalo = eval_tree_in(nod->left.get()) - eval_tree_in(nod->rigth.get());

    case MULT :
        return nod->intervalo = eval_tree_in(nod->left.get()) * eval_tree_in(nod->rigth.get());

    case DIV :
        return nod->intervalo = eval_tree_in(nod->left.get()) / eval_tree_in(nod->rigth.get());

    case POT :
    {
        //An integral square must not go through pow (exp of ln: a base
        //touching zero or negative aborts inside the noexcept library).
        const interval base = eval_tree_in(nod->left.get());
        const interval exponente = eval_tree_in(nod->rigth.get());

        if (Inf(exponente) == 2.0 && Sup(exponente) == 2.0) {
            return nod->intervalo = sqr(base);
        }

        return nod->intervalo = pow(base, exponente);
    }

    case SIN :
        return nod->intervalo = sin ( eval_tree_in(nod->left.get()) );

    case COS :
        return nod->intervalo = cos ( eval_tree_in(nod->left.get()) );

    case SQRT :
        return nod->intervalo = sqrt( eval_tree_in(nod->left.get()) );

    case TAN :
        return nod->intervalo = tan ( eval_tree_in(nod->left.get()) );

    case ATAN :
        return nod->intervalo = atan ( eval_tree_in(nod->left.get()) );

    case SINH :
        return nod->intervalo = sinh ( eval_tree_in(nod->left.get()) );

    case COSH :
        return nod->intervalo = cosh ( eval_tree_in(nod->left.get()) );

    case TANH :
        return nod->intervalo = tanh ( eval_tree_in(nod->left.get()) );

    case ASIN :
        return nod->intervalo = asin ( eval_tree_in(nod->left.get()) );

    case ACOS :
        return nod->intervalo = acos ( eval_tree_in(nod->left.get()) );

    case EXP :
        return nod->intervalo = exp ( eval_tree_in(nod->left.get()) );

    case ABS :
        return nod->intervalo = abs ( eval_tree_in(nod->left.get()) );

    //Both logarithms were missing here and fell through to a default that
    //returned the node's cached interval: for a fresh tree, uninitialised
    //memory.
    case LN :
        return nod->intervalo = ln ( eval_tree_in(nod->left.get()) );

    case LG :
        return nod->intervalo = log10 ( eval_tree_in(nod->left.get()) );

    default:
        throw std::invalid_argument("ExpressionTree: a node the interval evaluator does not know.");

        /* if another function was added, add its 'case' here with its operation */

    }
}

//Recursive pre-order walk that copies every node into a new one with the
//same content, returning a tree identical to the original. Used by the
//copy constructor and by the assignment operator.
std::unique_ptr<exp_node> ExpressionTree::make_cpy(exp_node *nod)
{
    if (!nod) return nullptr;

    auto ptr = std::make_unique<exp_node>();
    ptr->type  = nod->type;
    ptr->c_const = nod->c_const;
    //The variable name was not copied: a copied tree evaluated its
    //variables under an empty name.
    ptr->var = nod->var;

    ptr->left = make_cpy(nod->left.get());
    ptr->rigth = make_cpy(nod->rigth.get());

    return ptr;
}

/********************************************************
* void ExpressionTree::build_tree(std::string &in_exp)        *
**********************************************************/
// Builds the binary expression tree from the infix expression held in
// 'in_exp'.

void ExpressionTree::build_tree(std::string &in_exp)
{
    std::stack<type_node> operatorStack;               // operator stack
    std::stack<std::unique_ptr<exp_node>> nodeStack;

    const auto malformed = [&in_exp](const char * why) {
        return std::invalid_argument(std::string("ExpressionTree: ") + why
                                     + " in the expression '" + in_exp + "'.");
    };

    const auto popNode = [&]() {
        std::unique_ptr<exp_node> top = std::move(nodeStack.top());
        nodeStack.pop();
        return top;
    };

    //Turns the operator on top of the stack into a node over its operands.
    //The nine copies of this block the parser used to carry differed only
    //in which operators they were asked to reduce; the loops below decide
    //that, and this does the reducing.
    const auto reduceTop = [&]() {
        auto reduced = std::make_unique<exp_node>();
        reduced->type = operatorStack.top();
        operatorStack.pop();

        const std::size_t needed = reduced->type < SIN ? 2 : 1;   // binary operators come before SIN
        if (nodeStack.size() < needed) {
            throw malformed("an operator is missing an operand");
        }

        if (needed == 2) {
            reduced->rigth = popNode();
            reduced->left = popNode();
        } else {
            reduced->left = popNode();
        }

        nodeStack.push(std::move(reduced));
    };

    const auto pushLeaf = [&](type_node type) {
        auto leaf = std::make_unique<exp_node>();
        leaf->type = type;
        nodeStack.push(std::move(leaf));
    };

    const std::string::size_type len = in_exp.length();

    //A constant, optionally signed, with an optional exponent: the exponent
    //belongs to the constant (the historical lexer stopped at the 'e' and
    //re-read it as an identifier). Advances pos past it.
    const auto readConstant = [&](std::string::size_type & pos) {
        const std::string::size_type from = pos;
        if (in_exp[pos] == '-') {
            ++pos;
        }
        while ( pos < len && (isdigit(static_cast<unsigned char>(in_exp[pos])) || in_exp[pos] == '.') ) ++pos;
        if ( pos + 1 < len && (in_exp[pos] == 'e' || in_exp[pos] == 'E') &&
             (isdigit(static_cast<unsigned char>(in_exp[pos+1])) ||
              (pos + 2 < len && (in_exp[pos+1] == '+' || in_exp[pos+1] == '-') && isdigit(static_cast<unsigned char>(in_exp[pos+2])))) )
        {
            pos += 2;
            while ( pos < len && isdigit(static_cast<unsigned char>(in_exp[pos])) ) ++pos;
        }

        auto leaf = std::make_unique<exp_node>();
        leaf->type = CONS;
        leaf->c_const = std::strtod(in_exp.substr(from, pos - from).c_str(), nullptr);
        nodeStack.push(std::move(leaf));
    };

    const auto isIdentifierChar = [this](char c) {
        return es_letra(c) || isdigit(static_cast<unsigned char>(c)) || c == '_';
    };

    //A unary minus is read as "-1 *": the -1 goes on the node stack and the
    //product takes the precedence of any other product.
    const auto unaryMinus = [&](std::string::size_type & pos) {
        auto leaf = std::make_unique<exp_node>();
        leaf->type = CONS;
        leaf->c_const = -1.0;
        nodeStack.push(std::move(leaf));

        while ( !operatorStack.empty() && operatorStack.top() > RESTA ) {
            reduceTop();
        }
        operatorStack.push(MULT);
        ++pos;
    };

    std::string::size_type pos = 0;

    while (pos < len)
    {
        const char c = in_exp[pos];
        const bool digitNext = pos + 1 < len && isdigit(static_cast<unsigned char>(in_exp[pos+1]));
        const bool afterOpening = pos > 0 && in_exp[pos-1] == '(';

        if (c != '('){
            //A function name runs up to its opening parenthesis.
            std::string::size_type i = pos;
            while ( i < len && in_exp[i] != '(' ) ++i;
            const std::string tmp_str = in_exp.substr(pos, i-pos);

            static const std::pair<const char *, type_node> functions[] = {
                {"sin", SIN}, {"cos", COS}, {"tan", TAN}, {"atan", ATAN}, {"exp", EXP},
                {"sinh", SINH}, {"cosh", COSH}, {"abs", ABS}, {"ln", LN}, {"lg", LG},
                {"asin", ASIN}, {"acos", ACOS}, {"sqrt", SQRT},
            };
            /*more functions can be added with another entry here,
            plus an entry in 'enum type_node' and a case in eval_tree() */

            bool isFunction = false;
            for (const auto & function : functions) {
                if (tmp_str == function.first) {
                    operatorStack.push(function.second);
                    isFunction = true;
                    break;
                }
            }
            if (isFunction) {
                pos = i;
                continue;
            }
        }

        if ( c == '(' )  // Ej. "(......" o "....(........"
        {
            operatorStack.push(PAR); ++pos;    // always pushed
        }
        else if ( c == ')' )
        {
            while ( !operatorStack.empty() && operatorStack.top() != PAR )  // pop operators until the opening '(' (PAR) shows up
            {
                reduceTop();
            }
            if (operatorStack.empty()) {
                throw malformed("a closing parenthesis has no opening one");
            }
            operatorStack.pop(); // pop the PAR itself
            ++pos;
        }
        else if ( c == '-' && (pos == 0 || afterOpening) && digitNext ) // "-34.89..." or "(-34.89...": a negative constant
        {
            readConstant(pos);
        }
        else if ( isdigit(static_cast<unsigned char>(c)) )// Ej. : "....67.009.." ( constante positiva )
        {
            readConstant(pos);
        }
        else if ( in_exp.compare(pos, 2, "PI") == 0 && !(pos + 2 < len && isIdentifierChar(in_exp[pos+2])) ) // the constant PI
        {
            //A whole token, not a leading letter: "P1" and "E2" used to be
            //read as the constants, swallowing the variable.
            pushLeaf(PI);
            pos += 2;
        }
        else if ( c == 'E' && !(pos + 1 < len && isIdentifierChar(in_exp[pos+1])) ) // the constant E
        {
            pushLeaf(E);
            ++pos;
        }
        else if ( es_letra(c) ) //Ej. : "......x......." (variable x)
        {
            //An identifier is a letter followed by letters, digits or
            //underscores: the historical lexer stopped at the first
            //non-letter, so "z1" parsed as the variable "z" and a stray
            //constant 1.
            std::string::size_type i = pos;
            while ( i < len && isIdentifierChar(in_exp[i]) ) ++i;

            auto leaf = std::make_unique<exp_node>();
            leaf->type = VAR;
            leaf->var = in_exp.substr(pos, i-pos);
            nodeStack.push(std::move(leaf));
            pos = i;
        }
        else if ( c == '-' && (pos == 0 || afterOpening) ) // a leading unary minus: "-sin(...", "(-x..."
        {
            unaryMinus(pos);
        }
        else if ( c == '-' || c == '+' ) // binary '-' and '+': everything pending inside the parentheses binds tighter
        {
            while ( !operatorStack.empty() && operatorStack.top() != PAR )
            {
                reduceTop();
            }
            operatorStack.push(c == '-' ? RESTA : SUMA);
            ++pos;
        }
        else if ( c == '/' || c == '*' ) // products and powers pending bind tighter
        {
            while ( !operatorStack.empty() && operatorStack.top() > RESTA )
            {
                reduceTop();
            }
            operatorStack.push(c == '/' ? DIV : MULT);
            ++pos;
        }
        else if ( c == '^' ) // only powers and functions pending bind tighter
        {
            while ( !operatorStack.empty() && operatorStack.top() > DIV )
            {
                reduceTop();
            }
            operatorStack.push(POT);
            ++pos;
        }
        else
        {
            throw malformed(("a character the lexer does not know: '" + std::string(1, c) + "'").c_str());
        }
    }

    while ( !operatorStack.empty() )
    {
        if (operatorStack.top() == PAR) {
            throw malformed("an opening parenthesis is never closed");
        }
        reduceTop();
    }

    if (nodeStack.size() != 1) {
        throw malformed(nodeStack.empty() ? "nothing to evaluate" : "operands without an operator");
    }

    root = popNode();
}

bool ExpressionTree::es_letra(char tex){
    //This built a regular expression, then a one-character string, to ask
    //whether a character is a letter. The ranges are what the pattern
    //"[a-zA-Z]" said, spelled out - and unlike std::isalpha they do not
    //depend on the locale, which could otherwise start accepting accented
    //letters the lexer has no rule for.
    return (tex >= 'a' && tex <= 'z') || (tex >= 'A' && tex <= 'Z');
}

};
