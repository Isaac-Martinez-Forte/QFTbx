/*
Roberto C. Cruz Rodríguez
    rcruz@instec.cu
*/
// This function interpreter builds a binary expression tree in which every
// inner node is an operation and every leaf a value.

#include "src/core/math/expression_tree.h"
#include "src/core/math/constants.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <iostream>
#include <stack>
#include <stdexcept>
#include <string>

#include "imath.hpp"

using namespace std;
using namespace cxsc;


namespace qftbx {

namespace {

//The lexer reads the expression without blanks: spaces, tabs and line
//breaks alike (only the space used to go). Two operands with nothing but
//blanks between them ("2 3", "a b") are refused first: stripped, they
//would read as one token, and the user would get 23 for "2 3".
std::string withoutSpaces(const std::string & text)
{
    const auto tokenChar = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.';
    };

    std::string stripped;
    stripped.reserve(text.size());

    bool blankAfterToken = false;
    for (const char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            blankAfterToken = blankAfterToken || (!stripped.empty() && tokenChar(stripped.back()));
            continue;
        }
        if (blankAfterToken && tokenChar(c)) {
            throw std::invalid_argument("ExpressionTree: two values with nothing between them "
                                        "in the expression '" + text + "'.");
        }
        blankAfterToken = false;
        stripped.push_back(c);
    }
    return stripped;
}

//The functions of the grammar, by the name a user writes.
const std::pair<const char *, type_node> kFunctions[] = {
    {"sin", SIN}, {"cos", COS}, {"tan", TAN}, {"atan", ATAN}, {"exp", EXP},
    {"sinh", SINH}, {"cosh", COSH}, {"tanh", TANH}, {"abs", ABS},
    {"ln", LN}, {"log", LN}, {"lg", LG}, {"log10", LG}, {"log2", LOG2},
    {"asin", ASIN}, {"acos", ACOS}, {"sqrt", SQRT},
};

//The base-2 logarithm, which C-XSC does not provide: ln(x) / ln(2), the
//divisor enclosed.
interval log2Enclosure(const interval & x)
{
    return ln(x) / ln(interval(2.0));
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

ExpressionTree::ExpressionTree(const char *text)
{
    root = nullptr;
    std::string in_exp = withoutSpaces(text);

    build_tree(in_exp);
}

ExpressionTree::ExpressionTree(const std::string &text, double result, com comparison)
{
    root = nullptr;
    std::string in_exp = withoutSpaces(text);

    this->comparisonValue = result;
    this->comparison = comparison;

    build_tree(in_exp);
}

ExpressionTree::ExpressionTree(const std::string &text)
{
    root = nullptr;
    std::string in_exp = withoutSpaces(text);
    build_tree(in_exp);
}

ExpressionTree::ExpressionTree(const Expression & expression)
    : root(expression.release())
{
    if (root == nullptr) {
        throw std::invalid_argument("ExpressionTree: an empty expression.");
    }
}

ExpressionTree::ExpressionTree(const Expression & expression, double num, com comparison)
    : root(expression.release()),
      comparisonValue(num),
      comparison(comparison)
{
    if (root == nullptr) {
        throw std::invalid_argument("ExpressionTree: an empty expression.");
    }
}

ExpressionTree::ExpressionTree(const ExpressionTree &other )
    : root(make_cpy(other.root.get())),
      comparisonValue(other.comparisonValue),
      comparison(other.comparison),
      m_boundNames(other.m_boundNames)
{
}

//The root owns the tree, and every node owns its branches: the recursive
//post-order delete_tree() this used to call has nothing left to do.
ExpressionTree::~ExpressionTree() = default;

/********************************************************
* void ExpressionTree::setFunc(const std::string &text)        *
*********************************************************
* Discards the current tree and builds a new one from the given
* expression.
*/

void ExpressionTree::setFunc(const std::string &text)
{
    std::string in_exp = withoutSpaces(text);

    comparisonValue = 0;
    comparison = GREATER_EQUAL;

    build_tree(in_exp);
}

void ExpressionTree::setFunc(const std::string &text, double result, com comparison)
{
    std::string in_exp = withoutSpaces(text);

    this->comparisonValue = result;
    this->comparison = comparison;

    build_tree(in_exp);
}

/********************************************************
* void ExpressionTree::setFunc(const char *text)               *
*********************************************************
* As above, taking a C string.
*/
void ExpressionTree::setFunc(const char *text)
{
    std::string in_exp = withoutSpaces(text);

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

void ExpressionTree::print (){
    alg_exp_node_print(root.get());
}


/*
 * enum type_node { CONST, PI, E, VAR,
                 PARENTHESIS,
                 ADD, SUBTRACT, MULTIPLY, DIVIDE, POWER,
                 SIN, COS, TAN, SINH, COSH, ATAN, TANH, ASIN,
                 ACOS, EXP, ABS, LN, LG, SQRT
               };
 *
 */

void ExpressionTree::alg_exp_node_print(exp_node * node){


    if (node->type == CONSTANT){
        cout << node->c_const << endl;
    } else if (node->type == PI){
        cout << " pi " << endl;
    } else if (node->type == E){
        cout << " e " << endl;
    } else if (node->type == VAR){
        cout << node->var << endl;
    }else if (node->type == POWER){
        alg_exp_node_print(node->left.get());
        cout << symbolOf(node->type);
        cout << "2" << endl;
    }else if (node->type > POWER){
        cout << symbolOf(node->type);
        alg_exp_node_print(node->left.get());
    } else {
        alg_exp_node_print(node->left.get());
        cout << symbolOf(node->type);
        alg_exp_node_print(node->right.get());
    }

}

string ExpressionTree::symbolOf(type_node type)  {

    if (type == PI){
        return " PI ";
    }

    if(type == E){
        return " E ";
    }

    if (type == ADD){
        return " + ";
    }

    if (type == SUBTRACT){
        return " - ";
    }

    if (type == MULTIPLY){
        return " * ";
    }

    if (type == DIVIDE){
        return " / ";
    }

    if (type == POWER){
        return " ^ ";
    }

    if (type == SIN){
        return " sin ";
    }

    if (type == COS){
        return " cos ";
    }

    if (type == TAN){
        return " tan ";
    }

    if (type == SINH){
        return " sinh ";
    }

    if (type == COSH){
        return " cosh ";
    }

    if (type == ATAN){
        return " atan ";
    }

    if (type == TANH){
        return " tanh ";
    }

    if (type == ASIN){
        return " asin ";
    }

    if (type == ACOS){
        return " acos ";
    }

    if (type == EXP){
        return " e ";
    }

    if (type == ABS){
        return " abs ";
    }

    if (type == LN){
        return " ln ";
    }

    if (type == LG){
        return " log10 ";
    }

    if (type == LOG2){
        return " log2 ";
    }

    if (type == SQRT){
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
        m_boundNames = other.m_boundNames;
        comparisonValue = other.comparisonValue;
        comparison = other.comparison;
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
double ExpressionTree::eval_tree(exp_node *node)
{

    switch (node->type)
    {
    case CONSTANT :
        return node->c_const;

    case VAR  :
        return variables->at(node->var);

    case E:
        return qftbx::math::kE;

    case PI:
        return qftbx::math::kPi;

    case ADD :
        return eval_tree(node->left.get()) + eval_tree(node->right.get());

    case SUBTRACT :
        return eval_tree(node->left.get()) - eval_tree(node->right.get());

    case MULTIPLY :
        return eval_tree(node->left.get()) * eval_tree(node->right.get());

    case DIVIDE :
        return eval_tree(node->left.get()) / eval_tree(node->right.get());

    case POWER :
        return pow (eval_tree(node->left.get()),  eval_tree(node->right.get()) );

    case SIN :
        return sin ( eval_tree(node->left.get()) );

    case COS :
        return cos ( eval_tree(node->left.get()) );

    case SQRT :
        return sqrt( eval_tree(node->left.get()) );

    case TAN :
        return tan ( eval_tree(node->left.get()) );

    case ATAN :
        return atan ( eval_tree(node->left.get()) );

    case SINH :
        return sinh ( eval_tree(node->left.get()) );

    case COSH :
        return cosh ( eval_tree(node->left.get()) );

    case TANH :
        return tanh ( eval_tree(node->left.get()) );

    case ASIN :
        return asin ( eval_tree(node->left.get()) );

    case ACOS :
        return acos ( eval_tree(node->left.get()) );

    case EXP :
        return exp ( eval_tree(node->left.get()) );

    case ABS :
        return fabs ( eval_tree(node->left.get()) );

    case LN :
        return log ( eval_tree(node->left.get()) );

    case LG :
        return log10 ( eval_tree(node->left.get()) );

    case LOG2 :
        return log2 ( eval_tree(node->left.get()) );

        /* if another function was added, add its 'case' here with its operation */

    default:
        throw std::invalid_argument("ExpressionTree: a node the evaluator does not know.");
    }
}


bool ExpressionTree::propagate(std::map<string, interval> *variables){

    this->variables_in = variables;

    const interval result = eval_tree_in(root.get());

    //The part of the forward value that satisfies the constraint. Empty
    //means the box is infeasible; the whole value means there is nothing
    //to narrow. The comparison used to be stored and ignored: every
    //constraint was treated as ">=", which is the one MR builds.
    interval narrowed;

    switch (comparison) {
    case GREATER:
    case GREATER_EQUAL:
        if (Inf(result) > comparisonValue) {
            return true;
        }
        if (Sup(result) < comparisonValue) {
            return false;
        }
        narrowed = interval(comparisonValue, Sup(result));
        break;
    case LESS:
    case LESS_EQUAL:
        if (Sup(result) < comparisonValue) {
            return true;
        }
        if (Inf(result) > comparisonValue) {
            return false;
        }
        narrowed = interval(Inf(result), comparisonValue);
        break;
    case EQUAL:
        if (Inf(result) > comparisonValue || Sup(result) < comparisonValue) {
            return false;
        }
        narrowed = interval(comparisonValue);
        break;
    }

    //A domain emptied during the backward projection proves the box
    //inconsistent with the constraint.
    return eval_tree_out(root.get(), narrowed);
}

bool ExpressionTree::safeIntersection(const interval & a, const interval & b, interval & out){

    const cxsc::real low = (Inf(a) > Inf(b)) ? Inf(a) : Inf(b);
    const cxsc::real high = (Sup(a) < Sup(b)) ? Sup(a) : Sup(b);

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
bool ExpressionTree::eval_tree_out(exp_node *node, interval enclosure){

    interval candidate;

    switch (node->type)
    {

    case CONSTANT :
        return true;

    case VAR  :
    {
        //The forward pass of this same propagate() found the variable.
        *node->slot = enclosure;
        return true;
    }

    case ADD :
    {
        interval a;
        if (!safeIntersection(node->left->enclosure, enclosure - node->right->enclosure, a)) {
            return false;
        }
        if (!eval_tree_out(node->left.get(), a)) {
            return false;
        }

        interval b;
        if (!safeIntersection(node->right->enclosure, enclosure - a, b)) {
            return false;
        }
        return eval_tree_out(node->right.get(), b);
    }

    case SUBTRACT :
    {
        interval a;
        if (!safeIntersection(node->left->enclosure, enclosure + node->right->enclosure, a)) {
            return false;
        }
        if (!eval_tree_out(node->left.get(), a)) {
            return false;
        }

        interval b;
        if (!safeIntersection(node->right->enclosure, a - enclosure, b)) {
            return false;
        }
        return eval_tree_out(node->right.get(), b);
    }

    case MULTIPLY :
    {
        //Each factor projects as a quotient: only when the divisor does
        //not straddle zero (interval division would abort otherwise).
        interval a = node->left->enclosure;

        if (Inf(node->right->enclosure) > 0.0 || Sup(node->right->enclosure) < 0.0) {
            if (!safeIntersection(a, enclosure / node->right->enclosure, a)) {
                return false;
            }
            if (!eval_tree_out(node->left.get(), a)) {
                return false;
            }
        }

        if (Inf(a) > 0.0 || Sup(a) < 0.0) {
            interval b;
            if (!safeIntersection(node->right->enclosure, enclosure / a, b)) {
                return false;
            }
            return eval_tree_out(node->right.get(), b);
        }

        return true;
    }

    case DIVIDE :
    {
        interval a;
        if (!safeIntersection(node->left->enclosure, enclosure * node->right->enclosure, a)) {
            return false;
        }
        if (!eval_tree_out(node->left.get(), a)) {
            return false;
        }

        if (Inf(enclosure) > 0.0 || Sup(enclosure) < 0.0) {
            interval b;
            if (!safeIntersection(node->right->enclosure, a / enclosure, b)) {
                return false;
            }
            return eval_tree_out(node->right.get(), b);
        }

        return true;
    }

    case POWER :
    {
        //Only the square is projected (the constraint grammar uses no
        //other exponent): x^2 = I implies x in +-sqrt(I intersect [0,inf)).
        if (Inf(node->right->enclosure) != 2.0) {
            return true;
        }

        interval nonNegative;
        if (!safeIntersection(enclosure, interval(0.0, MaxReal), nonNegative)) {
            return false;
        }

        const interval root = sqrt(nonNegative);
        if (!safeIntersection(node->left->enclosure,
                                interval(-Sup(root), Sup(root)), candidate)) {
            return false;
        }
        return eval_tree_out(node->left.get(), candidate);
    }

    case SQRT :
    {
        //sqrt(x) = I: the result is never negative.
        interval nonNegative;
        if (!safeIntersection(enclosure, interval(0.0, MaxReal), nonNegative)) {
            return false;
        }

        if (!safeIntersection(node->left->enclosure, sqr(nonNegative), candidate)) {
            return false;
        }
        return eval_tree_out(node->left.get(), candidate);
    }

    case SIN :
    {
        interval bounded;
        if (!safeIntersection(enclosure, interval(-1.0, 1.0), bounded)) {
            return false;
        }

        //Only within the principal monotone branch of the argument.
        if (Inf(node->left->enclosure) < -qftbx::math::kPi / 2 || Sup(node->left->enclosure) > qftbx::math::kPi / 2) {
            return true;
        }

        if (!safeIntersection(node->left->enclosure, asin(bounded), candidate)) {
            return false;
        }
        return eval_tree_out(node->left.get(), candidate);
    }

    case COS :
    {
        interval bounded;
        if (!safeIntersection(enclosure, interval(-1.0, 1.0), bounded)) {
            return false;
        }

        //cos is monotone on [-pi, 0] and on [0, pi]; anything wider is
        //left unprojected.
        if (Inf(node->left->enclosure) >= -qftbx::math::kPi && Sup(node->left->enclosure) <= 0.0) {
            if (!safeIntersection(node->left->enclosure, -acos(bounded), candidate)) {
                return false;
            }
            return eval_tree_out(node->left.get(), candidate);
        }

        if (Inf(node->left->enclosure) >= 0.0 && Sup(node->left->enclosure) <= qftbx::math::kPi) {
            if (!safeIntersection(node->left->enclosure, acos(bounded), candidate)) {
                return false;
            }
            return eval_tree_out(node->left.get(), candidate);
        }

        return true;
    }

    case TAN :
    {
        if (Inf(node->left->enclosure) <= -qftbx::math::kPi / 2 || Sup(node->left->enclosure) >= qftbx::math::kPi / 2) {
            return true;
        }

        if (!safeIntersection(node->left->enclosure, atan(enclosure), candidate)) {
            return false;
        }
        return eval_tree_out(node->left.get(), candidate);
    }

    case ATAN :
    {
        interval bounded;
        if (!safeIntersection(enclosure, interval(-qftbx::math::kPi / 2, qftbx::math::kPi / 2), bounded)) {
            return false;
        }

        if (!safeIntersection(node->left->enclosure, tan(bounded), candidate)) {
            return false;
        }
        return eval_tree_out(node->left.get(), candidate);
    }

    case ASIN :
    {
        interval bounded;
        if (!safeIntersection(enclosure, interval(-qftbx::math::kPi / 2, qftbx::math::kPi / 2), bounded)) {
            return false;
        }

        if (!safeIntersection(node->left->enclosure, sin(bounded), candidate)) {
            return false;
        }
        return eval_tree_out(node->left.get(), candidate);
    }

    case ACOS :
    {
        interval bounded;
        if (!safeIntersection(enclosure, interval(0.0, qftbx::math::kPi), bounded)) {
            return false;
        }

        if (!safeIntersection(node->left->enclosure, cos(bounded), candidate)) {
            return false;
        }
        return eval_tree_out(node->left.get(), candidate);
    }

    case ABS :
    {
        //|x| = I: x in [-Sup(I+), Sup(I+)].
        interval nonNegative;
        if (!safeIntersection(enclosure, interval(0.0, MaxReal), nonNegative)) {
            return false;
        }

        if (!safeIntersection(node->left->enclosure,
                                interval(-Sup(nonNegative), Sup(nonNegative)), candidate)) {
            return false;
        }
        return eval_tree_out(node->left.get(), candidate);
    }

    case LN :
    {
        //exp overflows past ~709: skip rather than trap.
        if (Sup(enclosure) > 700.0) {
            return true;
        }

        if (!safeIntersection(node->left->enclosure, exp(enclosure), candidate)) {
            return false;
        }
        return eval_tree_out(node->left.get(), candidate);
    }

    default:
        return true;
    }
}

interval ExpressionTree::eval_tree_in(exp_node *node)
{

    switch (node->type)
    {
    case CONSTANT :
        return node->enclosure = interval (node->c_const);

    case VAR  :
    {
        //A missing variable used to return a default-constructed cxsc
        //interval, whose bounds are UNINITIALIZED memory. One lookup: the
        //name is compared against the map's keys here and nowhere else in
        //this evaluation.
        const auto found = variables_in->find(node->var);

        if (found == variables_in->end()) {
            throw std::invalid_argument(
                    "ExpressionTree: unknown variable '" + node->var + "' in the expression.");
        }

        node->slot = &found->second;
        return node->enclosure = found->second;
    }

    case E:
        return node->enclosure = eEnclosure();

    case PI:
        return node->enclosure = piEnclosure();

    case ADD :
        return node->enclosure = eval_tree_in(node->left.get()) + eval_tree_in(node->right.get());

    case SUBTRACT :
        return node->enclosure = eval_tree_in(node->left.get()) - eval_tree_in(node->right.get());

    case MULTIPLY :
        return node->enclosure = eval_tree_in(node->left.get()) * eval_tree_in(node->right.get());

    case DIVIDE :
        return node->enclosure = eval_tree_in(node->left.get()) / eval_tree_in(node->right.get());

    case POWER :
    {
        //An integral square must not go through pow (exp of ln: a base
        //touching zero or negative aborts inside the noexcept library).
        const interval base = eval_tree_in(node->left.get());
        const interval exponent = eval_tree_in(node->right.get());

        if (Inf(exponent) == 2.0 && Sup(exponent) == 2.0) {
            return node->enclosure = sqr(base);
        }

        return node->enclosure = pow(base, exponent);
    }

    case SIN :
        return node->enclosure = sin ( eval_tree_in(node->left.get()) );

    case COS :
        return node->enclosure = cos ( eval_tree_in(node->left.get()) );

    case SQRT :
        return node->enclosure = sqrt( eval_tree_in(node->left.get()) );

    case TAN :
        return node->enclosure = tan ( eval_tree_in(node->left.get()) );

    case ATAN :
        return node->enclosure = atan ( eval_tree_in(node->left.get()) );

    case SINH :
        return node->enclosure = sinh ( eval_tree_in(node->left.get()) );

    case COSH :
        return node->enclosure = cosh ( eval_tree_in(node->left.get()) );

    case TANH :
        return node->enclosure = tanh ( eval_tree_in(node->left.get()) );

    case ASIN :
        return node->enclosure = asin ( eval_tree_in(node->left.get()) );

    case ACOS :
        return node->enclosure = acos ( eval_tree_in(node->left.get()) );

    case EXP :
        return node->enclosure = exp ( eval_tree_in(node->left.get()) );

    case ABS :
        return node->enclosure = abs ( eval_tree_in(node->left.get()) );

    //Both logarithms were missing here and fell through to a default that
    //returned the node's cached interval: for a fresh tree, uninitialised
    //memory.
    case LN :
        return node->enclosure = ln ( eval_tree_in(node->left.get()) );

    case LG :
        return node->enclosure = log10 ( eval_tree_in(node->left.get()) );

    case LOG2 :
        return node->enclosure = log2Enclosure( eval_tree_in(node->left.get()) );

    default:
        throw std::invalid_argument("ExpressionTree: a node the interval evaluator does not know.");

        /* if another function was added, add its 'case' here with its operation */

    }
}

//Recursive pre-order walk that copies every node into a new one with the
//same content, returning a tree identical to the original. Used by the
//copy constructor and by the assignment operator.
std::unique_ptr<exp_node> ExpressionTree::make_cpy(exp_node *node)
{
    if (!node) return nullptr;

    auto ptr = std::make_unique<exp_node>();
    ptr->type  = node->type;
    ptr->c_const = node->c_const;
    //The variable name was not copied: a copied tree evaluated its
    //variables under an empty name.
    ptr->var = node->var;
    ptr->index = node->index;

    ptr->left = make_cpy(node->left.get());
    ptr->right = make_cpy(node->right.get());

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
            reduced->right = popNode();
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
    //re-read it as an identifier). It may start at its decimal point
    //(".5"). Advances pos past it.
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
        leaf->type = CONSTANT;
        leaf->c_const = std::strtod(in_exp.substr(from, pos - from).c_str(), nullptr);
        nodeStack.push(std::move(leaf));
    };

    const auto isIdentifierChar = [this](char c) {
        return isLetter(c) || isdigit(static_cast<unsigned char>(c)) || c == '_';
    };

    //A unary minus is read as "-1 *": the -1 goes on the node stack and the
    //product takes the precedence of any other product.
    const auto unaryMinus = [&](std::string::size_type & pos) {
        auto leaf = std::make_unique<exp_node>();
        leaf->type = CONSTANT;
        leaf->c_const = -1.0;
        nodeStack.push(std::move(leaf));

        while ( !operatorStack.empty() && operatorStack.top() > SUBTRACT ) {
            reduceTop();
        }
        operatorStack.push(MULTIPLY);
        ++pos;
    };

    std::string::size_type pos = 0;

    //Whether a '-' at pos is a unary minus: at the start, after an opening
    //parenthesis or after an operator. "2*-3" and "s^-1" used to fail as a
    //binary minus missing its left operand.
    const auto isUnaryMinusAt = [&](std::string::size_type at) {
        if (at == 0) {
            return true;
        }
        const char before = in_exp[at - 1];
        return before == '(' || before == '+' || before == '-' || before == '*' ||
               before == '/' || before == '^';
    };

    //Whether the number starting at 'from' is followed by '^': then a minus
    //before it is the unary minus of the power, -2^2 = -(2^2), not the sign
    //of the constant.
    const auto numberIsRaised = [&](std::string::size_type from) {
        std::string::size_type i = from;
        while ( i < len && (isdigit(static_cast<unsigned char>(in_exp[i])) || in_exp[i] == '.') ) ++i;
        if ( i + 1 < len && (in_exp[i] == 'e' || in_exp[i] == 'E') &&
             (isdigit(static_cast<unsigned char>(in_exp[i+1])) ||
              (i + 2 < len && (in_exp[i+1] == '+' || in_exp[i+1] == '-') && isdigit(static_cast<unsigned char>(in_exp[i+2])))) ) {
            i += 2;
            while ( i < len && isdigit(static_cast<unsigned char>(in_exp[i])) ) ++i;
        }
        return i < len && in_exp[i] == '^';
    };

    while (pos < len)
    {
        const char c = in_exp[pos];
        const bool digitNext = pos + 1 < len && isdigit(static_cast<unsigned char>(in_exp[pos+1]));

        if ( isLetter(c) )
        {
            //An identifier is a letter followed by letters, digits or
            //underscores: the historical lexer stopped at the first
            //non-letter, so "z1" parsed as the variable "z" and a stray
            //constant 1. Followed by an opening parenthesis and naming a
            //function, it is that function; otherwise a whole-token constant
            //(pi, e, in either case: "P1" and "E2" used to be read as the
            //constants, swallowing the variable) or a variable.
            std::string::size_type i = pos;
            while ( i < len && isIdentifierChar(in_exp[i]) ) ++i;
            const std::string token = in_exp.substr(pos, i - pos);

            if (i < len && in_exp[i] == '(') {
                bool isFunction = false;
                for (const auto & function : kFunctions) {
                    if (token == function.first) {
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

            if (token == "pi" || token == "PI") {
                pushLeaf(PI);
            } else if (token == "e" || token == "E") {
                pushLeaf(E);
            } else {
                auto leaf = std::make_unique<exp_node>();
                leaf->type = VAR;
                leaf->var = token;
                nodeStack.push(std::move(leaf));
            }
            pos = i;
        }
        else if ( c == '(' )  // Ej. "(......" o "....(........"
        {
            operatorStack.push(PARENTHESIS); ++pos;    // always pushed
        }
        else if ( c == ')' )
        {
            while ( !operatorStack.empty() && operatorStack.top() != PARENTHESIS )  // pop operators until the opening '(' (PARENTHESIS) shows up
            {
                reduceTop();
            }
            if (operatorStack.empty()) {
                throw malformed("a closing parenthesis has no opening one");
            }
            operatorStack.pop(); // pop the PARENTHESIS itself
            ++pos;
        }
        else if ( c == '-' && isUnaryMinusAt(pos) && (digitNext || (pos + 1 < len && in_exp[pos+1] == '.'))
                  && !numberIsRaised(pos + 1) ) // "-34.89...", "(-34.89...", "*-.5": a negative constant
        {
            readConstant(pos);
        }
        else if ( isdigit(static_cast<unsigned char>(c)) || (c == '.' && digitNext) )// "67.009", ".5": a positive constant
        {
            readConstant(pos);
        }
        else if ( c == '-' && isUnaryMinusAt(pos) ) // a unary minus: "-sin(...", "(-x...", "2*-x"
        {
            unaryMinus(pos);
        }
        else if ( c == '-' || c == '+' ) // binary '-' and '+': everything pending inside the parentheses binds tighter
        {
            while ( !operatorStack.empty() && operatorStack.top() != PARENTHESIS )
            {
                reduceTop();
            }
            operatorStack.push(c == '-' ? SUBTRACT : ADD);
            ++pos;
        }
        else if ( c == '/' || c == '*' ) // products and powers pending bind tighter
        {
            while ( !operatorStack.empty() && operatorStack.top() > SUBTRACT )
            {
                reduceTop();
            }
            operatorStack.push(c == '/' ? DIVIDE : MULTIPLY);
            ++pos;
        }
        else if ( c == '^' ) // only functions pending bind tighter: the power binds to the right (2^3^2 = 2^9)
        {
            while ( !operatorStack.empty() && operatorStack.top() > POWER )
            {
                reduceTop();
            }
            operatorStack.push(POWER);
            ++pos;
        }
        else
        {
            throw malformed(("a character the lexer does not know: '" + std::string(1, c) + "'").c_str());
        }
    }

    while ( !operatorStack.empty() )
    {
        if (operatorStack.top() == PARENTHESIS) {
            throw malformed("an opening parenthesis is never closed");
        }
        reduceTop();
    }

    if (nodeStack.size() != 1) {
        throw malformed(nodeStack.empty() ? "nothing to evaluate" : "operands without an operator");
    }

    root = popNode();
}

bool ExpressionTree::isLetter(char text){
    //This built a regular expression, then a one-character string, to ask
    //whether a character is a letter. The ranges are what the pattern
    //"[a-zA-Z]" said, spelled out - and unlike std::isalpha they do not
    //depend on the locale, which could otherwise start accepting accented
    //letters the lexer has no rule for.
    return (text >= 'a' && text <= 'z') || (text >= 'A' && text <= 'Z');
}

//---------------------------------------------------------- bound evaluation

void ExpressionTree::bind(const std::vector<std::string> & names)
{
    m_boundNames = names;
    bindNode(root.get(), names);
}

void ExpressionTree::bindNode(exp_node * node, const std::vector<std::string> & names)
{
    if (node == nullptr) {
        return;
    }

    if (node->type == VAR) {
        const auto found = std::find(names.begin(), names.end(), node->var);
        if (found == names.end()) {
            throw std::invalid_argument("ExpressionTree: the variable '" + node->var
                                        + "' has no value bound to it.");
        }
        node->index = static_cast<int>(std::distance(names.begin(), found));
    }

    bindNode(node->left.get(), names);
    bindNode(node->right.get(), names);
}

std::vector<std::string> ExpressionTree::variableNames() const
{
    std::vector<std::string> names;
    collectNames(root.get(), names);
    return names;
}

void ExpressionTree::collectNames(const exp_node * node, std::vector<std::string> & names) const
{
    if (node == nullptr) {
        return;
    }

    if (node->type == VAR && std::find(names.begin(), names.end(), node->var) == names.end()) {
        names.push_back(node->var);
    }

    collectNames(node->left.get(), names);
    collectNames(node->right.get(), names);
}

namespace {

void requireBound(const exp_node * node, std::size_t valueCount)
{
    if (node->index < 0 || static_cast<std::size_t>(node->index) >= valueCount) {
        throw std::invalid_argument("ExpressionTree: the variable '" + node->var
                                    + "' is not bound to a value (bind() first).");
    }
}

//An exponent that is a small whole constant is applied as repeated
//products, which is exact where the general complex power goes through
//the logarithm: (j w)^2 comes out as exactly -w^2.
bool isSmallWholeNumber(double value)
{
    return value == std::floor(value) && std::abs(value) <= 64.0;
}

} // namespace

double ExpressionTree::evaluate(const std::vector<double> & values) const
{
    if (root == nullptr) {
        throw std::invalid_argument("ExpressionTree: nothing to evaluate.");
    }
    return evaluateReal(root.get(), values);
}

std::complex<double> ExpressionTree::evaluate(const std::vector<std::complex<double>> & values) const
{
    if (root == nullptr) {
        throw std::invalid_argument("ExpressionTree: nothing to evaluate.");
    }
    return evaluateComplex(root.get(), values);
}

double ExpressionTree::evaluateReal(const exp_node * node, const std::vector<double> & values) const
{
    switch (node->type)
    {
    case CONSTANT : return node->c_const;
    case VAR      : requireBound(node, values.size()); return values[static_cast<std::size_t>(node->index)];
    case E        : return qftbx::math::kE;
    case PI       : return qftbx::math::kPi;
    case ADD      : return evaluateReal(node->left.get(), values) + evaluateReal(node->right.get(), values);
    case SUBTRACT : return evaluateReal(node->left.get(), values) - evaluateReal(node->right.get(), values);
    case MULTIPLY : return evaluateReal(node->left.get(), values) * evaluateReal(node->right.get(), values);
    case DIVIDE   : return evaluateReal(node->left.get(), values) / evaluateReal(node->right.get(), values);
    case POWER    : return std::pow(evaluateReal(node->left.get(), values), evaluateReal(node->right.get(), values));
    case SIN      : return std::sin(evaluateReal(node->left.get(), values));
    case COS      : return std::cos(evaluateReal(node->left.get(), values));
    case TAN      : return std::tan(evaluateReal(node->left.get(), values));
    case ATAN     : return std::atan(evaluateReal(node->left.get(), values));
    case SINH     : return std::sinh(evaluateReal(node->left.get(), values));
    case COSH     : return std::cosh(evaluateReal(node->left.get(), values));
    case TANH     : return std::tanh(evaluateReal(node->left.get(), values));
    case ASIN     : return std::asin(evaluateReal(node->left.get(), values));
    case ACOS     : return std::acos(evaluateReal(node->left.get(), values));
    case EXP      : return std::exp(evaluateReal(node->left.get(), values));
    case ABS      : return std::fabs(evaluateReal(node->left.get(), values));
    case LN       : return std::log(evaluateReal(node->left.get(), values));
    case LG       : return std::log10(evaluateReal(node->left.get(), values));
    case LOG2     : return std::log2(evaluateReal(node->left.get(), values));
    case SQRT     : return std::sqrt(evaluateReal(node->left.get(), values));
    default:
        throw std::invalid_argument("ExpressionTree: a node the evaluator does not know.");
    }
}

std::complex<double> ExpressionTree::evaluateComplex(const exp_node * node,
                                                     const std::vector<std::complex<double>> & values) const
{
    using Complex = std::complex<double>;

    switch (node->type)
    {
    case CONSTANT : return Complex(node->c_const, 0.0);
    case VAR      : requireBound(node, values.size()); return values[static_cast<std::size_t>(node->index)];
    case E        : return Complex(qftbx::math::kE, 0.0);
    case PI       : return Complex(qftbx::math::kPi, 0.0);
    case ADD      : return evaluateComplex(node->left.get(), values) + evaluateComplex(node->right.get(), values);
    case SUBTRACT : return evaluateComplex(node->left.get(), values) - evaluateComplex(node->right.get(), values);
    case MULTIPLY : return evaluateComplex(node->left.get(), values) * evaluateComplex(node->right.get(), values);
    case DIVIDE   : return evaluateComplex(node->left.get(), values) / evaluateComplex(node->right.get(), values);
    case POWER :
    {
        const Complex base = evaluateComplex(node->left.get(), values);
        const exp_node * exponent = node->right.get();

        if (exponent->type == CONSTANT && isSmallWholeNumber(exponent->c_const)) {
            return std::pow(base, static_cast<int>(exponent->c_const));
        }

        return std::pow(base, evaluateComplex(exponent, values));
    }
    case SIN      : return std::sin(evaluateComplex(node->left.get(), values));
    case COS      : return std::cos(evaluateComplex(node->left.get(), values));
    case TAN      : return std::tan(evaluateComplex(node->left.get(), values));
    case ATAN     : return std::atan(evaluateComplex(node->left.get(), values));
    case SINH     : return std::sinh(evaluateComplex(node->left.get(), values));
    case COSH     : return std::cosh(evaluateComplex(node->left.get(), values));
    case TANH     : return std::tanh(evaluateComplex(node->left.get(), values));
    case ASIN     : return std::asin(evaluateComplex(node->left.get(), values));
    case ACOS     : return std::acos(evaluateComplex(node->left.get(), values));
    case EXP      : return std::exp(evaluateComplex(node->left.get(), values));
    case ABS      : return Complex(std::abs(evaluateComplex(node->left.get(), values)), 0.0);
    case LN       : return std::log(evaluateComplex(node->left.get(), values));
    case LG       : return std::log10(evaluateComplex(node->left.get(), values));
    case LOG2     : return std::log(evaluateComplex(node->left.get(), values)) / std::log(2.0);
    case SQRT     : return std::sqrt(evaluateComplex(node->left.get(), values));
    default:
        throw std::invalid_argument("ExpressionTree: a node the evaluator does not know.");
    }
}

//------------------------------------------------------------------- names

bool ExpressionTree::isIdentifier(const std::string & name)
{
    if (name.empty()) {
        return false;
    }

    const auto letter = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    };
    const auto digit = [](char c) { return c >= '0' && c <= '9'; };

    if (!letter(name.front())) {
        return false;
    }

    for (const char c : name) {
        if (!letter(c) && !digit(c) && c != '_') {
            return false;
        }
    }

    return true;
}

bool ExpressionTree::isFunctionName(const std::string & name)
{
    for (const auto & function : kFunctions) {
        if (name == function.first) {
            return true;
        }
    }

    return false;
}

bool ExpressionTree::isReservedName(const std::string & name)
{
    return isFunctionName(name) || name == "pi" || name == "PI" || name == "e" || name == "E" ||
           name == "s";
}

bool ExpressionTree::isUsableVariableName(const std::string & name)
{
    return isIdentifier(name) && !isReservedName(name);
}

//-------------------------------------------------------------- Expression

Expression::Expression() = default;

Expression::Expression(double constant)
    : m_node(std::make_unique<exp_node>())
{
    m_node->type = CONSTANT;
    m_node->c_const = constant;
}

Expression::Expression(std::unique_ptr<exp_node> node)
    : m_node(std::move(node))
{
}

Expression::Expression(const Expression & other)
    : m_node(other.release())
{
}

Expression::Expression(Expression && other) noexcept = default;

Expression & Expression::operator=(const Expression & other)
{
    if (this != &other) {
        m_node = other.release();
    }
    return *this;
}

Expression & Expression::operator=(Expression && other) noexcept = default;

Expression::~Expression() = default;

Expression Expression::variable(const std::string & name)
{
    auto node = std::make_unique<exp_node>();
    node->type = VAR;
    node->var = name;
    return Expression(std::move(node));
}

Expression Expression::pi()
{
    auto node = std::make_unique<exp_node>();
    node->type = PI;
    return Expression(std::move(node));
}

Expression Expression::e()
{
    auto node = std::make_unique<exp_node>();
    node->type = E;
    return Expression(std::move(node));
}

namespace {

std::unique_ptr<exp_node> copyNode(const exp_node * node)
{
    if (node == nullptr) {
        return nullptr;
    }

    auto copy = std::make_unique<exp_node>();
    copy->type = node->type;
    copy->c_const = node->c_const;
    copy->var = node->var;
    copy->left = copyNode(node->left.get());
    copy->right = copyNode(node->right.get());
    return copy;
}

} // namespace

std::unique_ptr<exp_node> Expression::release() const
{
    return copyNode(m_node.get());
}

Expression Expression::binary(type_node type, const Expression & a, const Expression & b)
{
    if (a.m_node == nullptr || b.m_node == nullptr) {
        throw std::invalid_argument("Expression: an operator is missing an operand.");
    }

    auto node = std::make_unique<exp_node>();
    node->type = type;
    node->left = a.release();
    node->right = b.release();
    return Expression(std::move(node));
}

Expression Expression::unary(type_node type, const Expression & a)
{
    if (a.m_node == nullptr) {
        throw std::invalid_argument("Expression: a function is missing its argument.");
    }

    auto node = std::make_unique<exp_node>();
    node->type = type;
    node->left = a.release();
    return Expression(std::move(node));
}

Expression operator+(const Expression & a, const Expression & b) { return Expression::binary(ADD, a, b); }
Expression operator-(const Expression & a, const Expression & b) { return Expression::binary(SUBTRACT, a, b); }
Expression operator*(const Expression & a, const Expression & b) { return Expression::binary(MULTIPLY, a, b); }
Expression operator/(const Expression & a, const Expression & b) { return Expression::binary(DIVIDE, a, b); }
Expression operator-(const Expression & a) { return Expression::binary(MULTIPLY, Expression(-1.0), a); }
Expression pow(const Expression & base, const Expression & exponent) { return Expression::binary(POWER, base, exponent); }

Expression sqrt(const Expression & a) { return Expression::unary(SQRT, a); }
Expression sin(const Expression & a) { return Expression::unary(SIN, a); }
Expression cos(const Expression & a) { return Expression::unary(COS, a); }
Expression tan(const Expression & a) { return Expression::unary(TAN, a); }
Expression atan(const Expression & a) { return Expression::unary(ATAN, a); }
Expression exp(const Expression & a) { return Expression::unary(EXP, a); }
Expression abs(const Expression & a) { return Expression::unary(ABS, a); }
Expression ln(const Expression & a) { return Expression::unary(LN, a); }

} // namespace qftbx
