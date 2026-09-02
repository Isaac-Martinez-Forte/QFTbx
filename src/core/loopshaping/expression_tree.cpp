/*
Roberto C. Cruz Rodríguez
    rcruz@instec.cu
*/
// This function interpreter builds a binary expression tree in which every
// inner node is an operation and every leaf a value.

#include "src/core/loopshaping/expression_tree.h"

#include <stdexcept>

#define PI1 3.1415926535897936
#define E1 2.71828182846

using namespace std;
using namespace cxsc;


namespace alg {

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
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // whitespace is stripped from the input
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // so the expression reads uniformly

    build_tree(in_exp);
}

ExpressionTree::ExpressionTree(const std::string &tex, double resultado, com comparacion)
{
    root = nullptr;
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // whitespace is stripped from the input
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // so the expression reads uniformly

    this->comparisonValue = resultado;
    this->comparacion = comparacion;

    build_tree(in_exp);
}

ExpressionTree::ExpressionTree(const ExpressionTree &other )                                            // constructor de copia
{
    this->root = make_cpy(other.root.get() );
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
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // whitespace is stripped from the input
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // so the expression reads uniformly

    comparisonValue = 0;
    comparacion = GREATER_EQUAL;

    build_tree(in_exp);
}

void ExpressionTree::setFunc(const std::string &tex, double resultado, com comparacion)
{
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // whitespace is stripped from the input
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // so the expression reads uniformly

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
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // whitespace is stripped from the input
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // so the expression reads uniformly

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



    if (node->type <= 2){
        if (node->type == 0){
            cout << node->c_const << endl;
        } else if (node->type == 1){
            cout << " pi " << endl;
        }else {
            cout << " e " << endl;
        }
    } else if (node->type == VAR){
        cout << node->var << endl;

    }else if (node->type == 9){
        alg_exp_node_print(node->left.get());
        cout << tipo(node->type);
        cout << "2" << endl;
    }else if (node->type > 9){
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

/*interval ExpressionTree::eval(std::map<string, interval> *variables, double w){

    this->w = w;
    this->variables_in = variables;

    return eval_tree_complex_interval(root.get());
}*/

//Assignment: a deep copy, so the two trees own separate nodes.
ExpressionTree &ExpressionTree::operator=(const ExpressionTree &other)
{
    //Falling off the end of a value-returning function is undefined
    //behaviour (the historical version did, flagged by every build).
    if (this != &other) {
        root = make_cpy(other.root.get());
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
        return E1;

    case PI:
        return PI1;

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
        return 0;
    }
}


bool ExpressionTree::propagate(std::map<string, interval> *variables){

    this->variables_in = variables;

    interval resultado = eval_tree_in(root.get());

    interval nuevo_intervalo;

    if (Inf(resultado) > comparisonValue){
        return true;
    }

    if (Sup(resultado) >= comparisonValue){
        nuevo_intervalo = interval (comparisonValue, Sup(resultado));
    }else{
        return false;
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
        if (Inf(nod->left->intervalo) < -M_PI / 2 || Sup(nod->left->intervalo) > M_PI / 2) {
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
        if (Inf(nod->left->intervalo) >= -M_PI && Sup(nod->left->intervalo) <= 0.0) {
            if (!safeIntersection(nod->left->intervalo, -acos(acotado), candidato)) {
                return false;
            }
            return eval_tree_out(nod->left.get(), candidato);
        }

        if (Inf(nod->left->intervalo) >= 0.0 && Sup(nod->left->intervalo) <= M_PI) {
            if (!safeIntersection(nod->left->intervalo, acos(acotado), candidato)) {
                return false;
            }
            return eval_tree_out(nod->left.get(), candidato);
        }

        return true;
    }

    case TAN :
    {
        if (Inf(nod->left->intervalo) <= -M_PI / 2 || Sup(nod->left->intervalo) >= M_PI / 2) {
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
        if (!safeIntersection(intervalo, interval(-M_PI / 2, M_PI / 2), acotado)) {
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
        if (!safeIntersection(intervalo, interval(-M_PI / 2, M_PI / 2), acotado)) {
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
        if (!safeIntersection(intervalo, interval(0.0, M_PI), acotado)) {
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
        return interval (E1);

    case PI:
        return interval (PI1);

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
    default:
        return nod->intervalo;

        /* if another function was added, add its 'case' here with its operation */

    }
}

/*interval ExpressionTree::eval_tree_complex_interval(exp_node *nod)
{

    complex<double> emptyComplex(1,0);


    switch (nod->type)
    {
    case CONST :
        return interval (nod->c_const);

    case VAR  :

        if (nod->var == "s")
            return complex<double> (0, w) * interval(1);

        return  variables_in->at(nod->var) * emptyComplex;


    case E:
        return interval (M_E) * emptyComplex;

    case PI:
        return interval (M_PI) * emptyComplex;

    case SUMA :

        return  eval_tree_complex_interval(nod->left.get()) + eval_tree_complex_interval(nod->rigth.get());

    case RESTA :
        return  eval_tree_complex_interval(nod->left.get()) - eval_tree_complex_interval(nod->rigth.get());

    case MULT :
        return  eval_tree_complex_interval(nod->left.get()) * eval_tree_complex_interval(nod->rigth.get());

    case DIV :
        return  eval_tree_complex_interval(nod->left.get()) / eval_tree_complex_interval(nod->rigth.get());

    case POT :
        return pow (eval_tree_complex_interval(nod->left.get()),  eval_tree_complex_interval(nod->rigth.get()) );

    case SIN :
        return  sin ( eval_tree_complex_interval(nod->left.get()) );

    case COS :
        return  cos ( eval_tree_complex_interval(nod->left.get()) );

    case SQRT :
        return  sqrt( eval_tree_complex_interval(nod->left.get()) );

    case TAN :
        return  tan ( eval_tree_complex_interval(nod->left.get()) );

    case ATAN :
        return  atan ( eval_tree_complex_interval(nod->left.get()) );

    case SINH :
        return  sinh ( eval_tree_complex_interval(nod->left.get()) );

    case COSH :
        return  cosh ( eval_tree_complex_interval(nod->left.get()) );

    case TANH :
        return  tanh ( eval_tree_complex_interval(nod->left.get()) );

    case ASIN :
        return  asin ( eval_tree_complex_interval(nod->left.get()) );

    case ACOS :
        return  acos ( eval_tree_complex_interval(nod->left.get()) );

    case EXP :
        return  exp ( eval_tree_complex_interval(nod->left.get()) );

        //case ABS :
        //   return  abs ( eval_tree_complex_interval(nod->left.get()) );

    case LN :
        return  ln ( eval_tree_complex_interval(nod->left.get()) );

    case LG :
        return  log10 ( eval_tree_complex_interval(nod->left.get()) );

         // if another function was added, add its 'case' here with its operation

    }
}*/

//Recursive pre-order walk that copies every node into a new one with the
//same content, returning a tree identical to the original. Used by the
//copy constructor and by the assignment operator.
std::unique_ptr<exp_node> ExpressionTree::make_cpy(exp_node *nod)
{
    if (!nod) return nullptr;

    auto ptr = std::make_unique<exp_node>();
    ptr->type  = nod->type;
    ptr->c_const = nod->c_const;

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

    //Scratch node of the shunting-yard loop. This used to be the MEMBER
    //node, borrowed as a temporary all the way through the parse and only
    //holding the real node on the last line.
    std::unique_ptr<exp_node> node;

    std::string tmp_str;

    std::string::size_type i, pos = 0, len = in_exp.length();

    while (pos < len)
    {

        if (in_exp[pos] != '('){
            i = pos;
            while ( (in_exp[i] != '(') && (i < len)) ++i;

            tmp_str = in_exp.substr(pos, i-pos);

            if (tmp_str == "sin" ){
                operatorStack.push(SIN);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "cos" ){
                operatorStack.push(COS);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "tan" ){
                operatorStack.push(TAN);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "atan" ){
                operatorStack.push(ATAN);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "exp" ){
                operatorStack.push(EXP);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "sinh" ){
                operatorStack.push(SINH);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "cosh" ){
                operatorStack.push(COSH);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "abs" ){
                operatorStack.push(ABS);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "ln" ){
                operatorStack.push(LN);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "lg" ){
                operatorStack.push(LG);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "asin" ){
                operatorStack.push(ASIN);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "acos" ){
                operatorStack.push(ACOS);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "sqrt"){
                operatorStack.push(SQRT);
                pos += (i - pos);
                continue;
            }
            /*more functions can be added with another else if () here,
            plus an entry in 'enum type_node' and a case in eval_tree() */

        }

        if ( in_exp[pos] == '(' )  // Ej. "(......" o "....(........"
        {
            operatorStack.push(PAR); ++pos;    // always pushed
        }
        else if ( in_exp[pos] ==')' )
        {
            while ( operatorStack.top() != PAR )  // pop operators until the opening '(' (PAR) shows up
            {
                node = std::make_unique<exp_node>();
                node->type = operatorStack.top();
                if (node->type  < SIN) // a binary operator
                {
                    node->rigth = std::move(nodeStack.top()); nodeStack.pop();
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                }
                else                   // a unary one
                {
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                    node->rigth = nullptr;
                }
                operatorStack.pop();
                nodeStack.push(std::move(node));
            }
            operatorStack.pop(); // pop the PAR itself
            ++pos;
        }
        else if (!pos && in_exp[pos] == '-' && isdigit(in_exp[pos+1]) ) // e.g. "-34.89...": a negative constant, form 1
        {
            node = std::make_unique<exp_node>();
            node->left = nullptr;
            node->rigth = nullptr;
            node->type = CONS;

            i = pos++;
            while ( isdigit(in_exp[pos]) || in_exp[pos] == '.' ) ++pos;
            //Scientific notation ("1e-16", "2.5e+3"): the exponent belongs
            //to the constant (the historical lexer stopped at the 'e' and
            //re-read it as an identifier).
            if ( pos + 1 < len && (in_exp[pos] == 'e' || in_exp[pos] == 'E') &&
                 (isdigit(in_exp[pos+1]) ||
                  (pos + 2 < len && (in_exp[pos+1] == '+' || in_exp[pos+1] == '-') && isdigit(in_exp[pos+2]))) )
            {
                pos += 2;
                while ( pos < len && isdigit(in_exp[pos]) ) ++pos;
            } // the whole constant has been read

            node->c_const = QString::fromUtf8(in_exp.substr(i, pos-i).c_str()).toDouble();

            nodeStack.push(std::move(node));
        }
        else if ( pos && in_exp[pos] == '-' && in_exp[pos-1] == '(' && isdigit(in_exp[pos+1]) )//Ej. "..(-34.89...." (constante negativa)
        {
            node = std::make_unique<exp_node>();
            node->left = nullptr;
            node->rigth = nullptr;
            node->type = CONS;

            i = pos++;
            while ( isdigit(in_exp[pos]) || in_exp[pos] == '.' ) ++pos;
            //Scientific notation ("1e-16", "2.5e+3"): the exponent belongs
            //to the constant (the historical lexer stopped at the 'e' and
            //re-read it as an identifier).
            if ( pos + 1 < len && (in_exp[pos] == 'e' || in_exp[pos] == 'E') &&
                 (isdigit(in_exp[pos+1]) ||
                  (pos + 2 < len && (in_exp[pos+1] == '+' || in_exp[pos+1] == '-') && isdigit(in_exp[pos+2]))) )
            {
                pos += 2;
                while ( pos < len && isdigit(in_exp[pos]) ) ++pos;
            }

            node->c_const = QString::fromUtf8(in_exp.substr(i, pos-i).c_str()).toDouble();

            nodeStack.push(std::move(node));
        }
        else if ( isdigit(in_exp[pos]) )// Ej. : "....67.009.." ( constante positiva )
        {
            node = std::make_unique<exp_node>();
            node->left = nullptr;
            node->rigth = nullptr;
            node->type = CONS;

            i = pos;
            while ( isdigit(in_exp[pos]) || in_exp[pos] == '.' ) ++pos;
            //Scientific notation ("1e-16", "2.5e+3"): the exponent belongs
            //to the constant (the historical lexer stopped at the 'e' and
            //re-read it as an identifier).
            if ( pos + 1 < len && (in_exp[pos] == 'e' || in_exp[pos] == 'E') &&
                 (isdigit(in_exp[pos+1]) ||
                  (pos + 2 < len && (in_exp[pos+1] == '+' || in_exp[pos+1] == '-') && isdigit(in_exp[pos+2]))) )
            {
                pos += 2;
                while ( pos < len && isdigit(in_exp[pos]) ) ++pos;
            }

            node->c_const = QString::fromUtf8(in_exp.substr(i, pos-i).c_str()).toDouble();

            nodeStack.push(std::move(node));
        }
        else if ( in_exp[pos] == 'E' || in_exp[pos] == 'P' ) // the constants PI and E
        {
            node = std::make_unique<exp_node>();
            node->left = nullptr;
            node->rigth = nullptr;
            if (in_exp[pos] == 'E')
            {
                node->type = E;
                pos++;
            }
            else
            {
                node->type = PI;
                pos+=2;
            }
            nodeStack.push(std::move(node));
        }
        else if ( es_letra(in_exp[pos]) ) //Ej. : "......x......." (variable x)
        {

            std::string tmp_str;
            std::string::size_type i = pos, len = in_exp.length();

            //An identifier is a letter followed by letters, digits or
            //underscores: the historical lexer stopped at the first
            //non-letter, so "z1" parsed as the variable "z" and a stray
            //constant 1.
            while ( i < len && (es_letra(in_exp[i]) || isdigit((unsigned char)in_exp[i]) || in_exp[i] == '_') ) ++i;

            tmp_str = in_exp.substr(pos, i-pos);
            pos = i;

            node = std::make_unique<exp_node>();
            node->left = nullptr;
            node->rigth = nullptr;
            node->type = VAR;
            node->var = tmp_str;
            nodeStack.push(std::move(node));
        }
        else if ( !pos && in_exp[pos] == '-' ) // a leading unary minus, form 1:
        {                                      // an expression like "-sin(..." or "-x..."
            node = std::make_unique<exp_node>();               // is treated as "-1*sin(..." and "-1*x.."
            node->type = CONS;
            node->left = nullptr;
            node->rigth = nullptr;
            node->c_const = -1.0000000000000000000;
            nodeStack.push(std::move(node));

            while ( !operatorStack.empty() && operatorStack.top() > RESTA )
            {
                node = std::make_unique<exp_node>();
                node->type = operatorStack.top();
                if  ( node->type < SIN )
                {
                    node->rigth = std::move(nodeStack.top()); nodeStack.pop();
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                }
                else
                {
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                    node->rigth = nullptr;
                }
                nodeStack.push(std::move(node));
                operatorStack.pop();
            }
            operatorStack.push(MULT);
            ++pos;
        }
        else if ( in_exp[pos] == '-' && in_exp[pos-1] == '(' ) // a unary minus after '(', form 2:
        {                                                      // an expression like "...(-sin..." or "...(-x..."
            node = std::make_unique<exp_node>();                               // is treated as "...(-1*sin..." and "...(-1*x.."
            node->type = CONS;
            node->left = nullptr;
            node->rigth = nullptr;
            node->c_const = -1.0000000000000000000;
            nodeStack.push(std::move(node));

            while ( !operatorStack.empty() && operatorStack.top() > RESTA )
            {
                node = std::make_unique<exp_node>();
                node->type = operatorStack.top();
                if  ( node->type < SIN )
                {
                    node->rigth = std::move(nodeStack.top()); nodeStack.pop();
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                }
                else
                {
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                    node->rigth = nullptr;
                }
                nodeStack.push(std::move(node));
                operatorStack.pop();
            }
            operatorStack.push(MULT);
            ++pos;
        }
        else if ( in_exp[pos] == '-' ) // Ej: "....x-y..."     operador '-' binario
        {
            while ( !operatorStack.empty() && operatorStack.top() != PAR )
            {
                node = std::make_unique<exp_node>();
                node->type = operatorStack.top();

                if  ( node->type < SIN )
                {
                    node->rigth = std::move(nodeStack.top()); nodeStack.pop();
                    node->left  = std::move(nodeStack.top()); nodeStack.pop();
                }
                else
                {
                    node->left  = std::move(nodeStack.top()); nodeStack.pop();
                    node->rigth = nullptr;
                }
                nodeStack.push(std::move(node));
                operatorStack.pop();
            }
            operatorStack.push(RESTA);
            ++pos;
        }
        else if ( in_exp[pos] == '+' ) // Ej: "....x+y..."     operador '+' binario
        {
            while ( !operatorStack.empty() && operatorStack.top() != PAR )
            {
                node = std::make_unique<exp_node>();
                node->type = operatorStack.top();
                if  ( node->type < SIN )
                {
                    node->rigth = std::move(nodeStack.top()); nodeStack.pop();
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                }
                else
                {
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                    node->rigth = nullptr;
                }
                nodeStack.push(std::move(node));
                operatorStack.pop();
            }
            operatorStack.push(SUMA);
            ++pos;
        }
        else if ( in_exp[pos] == '/' )// Ej: "....x/y..."     operador '/' binario
        {
            while ( !operatorStack.empty() && operatorStack.top() > RESTA )
            {
                node = std::make_unique<exp_node>();
                node->type = operatorStack.top();
                if  ( node->type < SIN )
                {
                    node->rigth = std::move(nodeStack.top()); nodeStack.pop();
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                }
                else
                {
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                    node->rigth = nullptr;
                }
                nodeStack.push(std::move(node));
                operatorStack.pop();
            }
            operatorStack.push(DIV);
            ++pos;
        }
        else if ( in_exp[pos] == '*' )// Ej: "....x*y..."     operador '*' binario
        {
            while ( !operatorStack.empty() && operatorStack.top() > RESTA )
            {
                node = std::make_unique<exp_node>();
                node->type = operatorStack.top();
                if  ( node->type < SIN )
                {
                    node->rigth = std::move(nodeStack.top()); nodeStack.pop();
                    node->left = std::move(nodeStack.top());  nodeStack.pop();
                }
                else
                {
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                    node->rigth = nullptr;
                }
                nodeStack.push(std::move(node));
                operatorStack.pop();
            }
            operatorStack.push(MULT);
            ++pos;
        }
        else if ( in_exp[pos] == '^' ) // Ej: "....x^y..."     operador '^' binario
        {
            while ( !operatorStack.empty() && operatorStack.top() > DIV )
            {
                node = std::make_unique<exp_node>();
                node->type = operatorStack.top();
                if  ( node->type == POT )
                {
                    node->rigth = std::move(nodeStack.top()); nodeStack.pop();
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                }
                else
                {
                    node->left = std::move(nodeStack.top()); nodeStack.pop();
                    node->rigth = nullptr;
                }
                nodeStack.push(std::move(node));
                operatorStack.pop();
            }
            operatorStack.push(POT);
            ++pos;
        }
    }

    while ( !operatorStack.empty() )
    {
        node = std::make_unique<exp_node>();
        node->type = operatorStack.top();

        if ( node->type < SIN )
        {
            node->rigth = std::move(nodeStack.top()); nodeStack.pop();
            node->left = std::move(nodeStack.top()); nodeStack.pop();
        }
        else
        {
            node->left = std::move(nodeStack.top()); nodeStack.pop();
            node->rigth = nullptr;
        }
        operatorStack.pop();
        nodeStack.push(std::move(node));
    }

    root  = std::move(nodeStack.top()); nodeStack.pop();

}

bool ExpressionTree::es_letra(char tex){
    static QRegularExpression rx("^[a-zA-Z]*$");
    QString s (1, tex);

    return rx.match(s).hasMatch();
}

};
