/*
Roberto C. Cruz Rodríguez
    rcruz@instec.cu
*/
// Este interprete de funciones se basa en la creación de un árbol binario de expresiones
// donde cada nodo representa una operación a realizar y cada hoja un valor númerico

#include "src/core/loopshaping/expression_tree.h"

#include <stdexcept>

#define PI1 3.1415926535897936
#define E1 2.71828182846

using namespace std;
using namespace cxsc;


namespace alg {

//////////////////////////////////////////
// ExpressionTree implementation //
//////////////////////////////////////////
ExpressionTree::ExpressionTree()
{
    root = nullptr;
}

ExpressionTree::ExpressionTree(const char *tex)
{
    root = nullptr;
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // aquí se eliminan los espacios en blanco de la entrada
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // para facilitar la lectura de la expresión matemática

    build_tree(in_exp);
}

ExpressionTree::ExpressionTree(const std::string &tex, qreal resultado, com comparacion)
{
    root = nullptr;
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // aquí se eliminan los espacios en blanco de la entrada
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // para facilitar la lectura de la expresión matemática

    this->numero_comparar = resultado;
    this->comparacion = comparacion;

    build_tree(in_exp);
}

ExpressionTree::ExpressionTree(const ExpressionTree &other )                                            // constructor de copia
{
    this->root = make_cpy( other.root );
}

ExpressionTree::~ExpressionTree()                                                                 // destructor
{
    delete_tree(root);
}

/********************************************************
* void ExpressionTree::setFunc(const std::string &tex)        *
*********************************************************
* Con esta función se elimina el
* árbol actual y se construye otro
* a partir de una nueva expresión
*/

void ExpressionTree::setFunc(const std::string &tex)
{
    delete_tree(root);
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // aquí se eliminan los espacios en blanco de la entrada
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // para facilitar la lectura de la expresión matemática

    numero_comparar = 0;
    comparacion = MAYORIGUAL;

    build_tree(in_exp);
}

void ExpressionTree::setFunc(const std::string &tex, qreal resultado, com comparacion)
{
    delete_tree(root);
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // aquí se eliminan los espacios en blanco de la entrada
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // para facilitar la lectura de la expresión matemática

    this->numero_comparar = resultado;
    this->comparacion = comparacion;

    build_tree(in_exp);
}

/********************************************************
* void ExpressionTree::setFunc(const char *tex)               *
*********************************************************
*         La misma que la anterior
*/
void ExpressionTree::setFunc(const char *tex)
{
    delete_tree(root);
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // aquí se eliminan los espacios en blanco de la entrada
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // para facilitar la lectura de la expresión matemática

    build_tree(in_exp);
}

/********************************************************
* ExpressionTree &ExpressionTree::operator=(const ExpressionTree &other)  *
*********************************************************
* Esta función nos permite evaluar
* la expresión matemática
*/
qreal ExpressionTree::eval(std::map<std::string, qreal> *variables )
{
    this->variables = variables;

    return eval_tree(root);
}

void ExpressionTree::imprimir (){
    alg_exp_node_print(root);
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
        alg_exp_node_print(node->left);
        cout << tipo(node->type);
        cout << "2" << endl;
    }else if (node->type > 9){
        cout << tipo(node->type);
        alg_exp_node_print(node->left);
    } else {
        alg_exp_node_print(node->left);
        cout << tipo(node->type);
        alg_exp_node_print(node->rigth);
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

    return eval_tree_in(root);
}

/*interval ExpressionTree::eval(std::map<string, interval> *variables, qreal w){

    this->w = w;
    this->variables_in = variables;

    return eval_tree_complex_interval(root);
}*/

/********************************************************
* ExpressionTree &ExpressionTree::operator=(const ExpressionTree &other)  *
*********************************************************
* la homonimia del operador '=', nos
* permite especificar como debe ser la
* asignacion de un objeto del tipo ExpressionTree
* a otro ......
*/
ExpressionTree &ExpressionTree::operator=(const ExpressionTree &other)
{
    //Falling off the end of a value-returning function is undefined
    //behaviour (the historical version did, flagged by every build).
    if (this != &other) {
        delete_tree(root);
        root = make_cpy(other.root);
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
qreal ExpressionTree::operator()(std::map<std::string, qreal> * variables)
{
    return eval (variables);
}

interval ExpressionTree::operator ()(std::map<std::string, interval> * variables){
    return eval (variables);
}

/********************************************************
*    double ExpressionTree::eval_tree(exp_node *nod)          *
*********************************************************
* esta es la función más importante de la clase ...
* es una función recursiva que recorre el arbol binario
* de expresiones y realiza la operacion matemática corres-
* pondiente en cada nodo para finalmete, devolver el resultado
* de evaluar la expresión contenida en el arbol .......
*/
qreal ExpressionTree::eval_tree(exp_node *nod)
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
        return eval_tree(nod->left) + eval_tree(nod->rigth);

    case RESTA :
        return eval_tree(nod->left) - eval_tree(nod->rigth);

    case MULT :
        return eval_tree(nod->left) * eval_tree(nod->rigth);

    case DIV :
        return eval_tree(nod->left) / eval_tree(nod->rigth);

    case POT :
        return pow (eval_tree(nod->left),  eval_tree(nod->rigth) );

    case SIN :
        return sin ( eval_tree(nod->left) );

    case COS :
        return cos ( eval_tree(nod->left) );

    case SQRT :
        return sqrt( eval_tree(nod->left) );

    case TAN :
        return tan ( eval_tree(nod->left) );

    case ATAN :
        return atan ( eval_tree(nod->left) );

    case SINH :
        return sinh ( eval_tree(nod->left) );

    case COSH :
        return cosh ( eval_tree(nod->left) );

    case TANH :
        return tanh ( eval_tree(nod->left) );

    case ASIN :
        return asin ( eval_tree(nod->left) );

    case ACOS :
        return acos ( eval_tree(nod->left) );

    case EXP :
        return exp ( eval_tree(nod->left) );

    case ABS :
        return fabs ( eval_tree(nod->left) );

    case LN :
        return log ( eval_tree(nod->left) );

    case LG :
        return log10 ( eval_tree(nod->left) );

        /* si se ha añadido otra función, entoces solo agregue otro 'case ' y defina la operación a realizar */

    default:
        return 0;
    }
}


bool ExpressionTree::propagate(std::map<string, interval> *variables){

    this->variables_in = variables;

    interval resultado = eval_tree_in(root);

    interval nuevo_intervalo;

    if (Inf(resultado) > numero_comparar){
        return true;
    }

    if (Sup(resultado) >= numero_comparar){
        nuevo_intervalo = interval (numero_comparar, Sup(resultado));
    }else{
        return false;
    }

    //A domain emptied during the backward projection proves the box
    //inconsistent with the constraint.
    return eval_tree_out(root, nuevo_intervalo);
}

bool ExpressionTree::interseccionSegura(const interval & a, const interval & b, interval & out){

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
        if (!interseccionSegura(nod->left->intervalo, intervalo - nod->rigth->intervalo, a)) {
            return false;
        }
        if (!eval_tree_out(nod->left, a)) {
            return false;
        }

        interval b;
        if (!interseccionSegura(nod->rigth->intervalo, intervalo - a, b)) {
            return false;
        }
        return eval_tree_out(nod->rigth, b);
    }

    case RESTA :
    {
        interval a;
        if (!interseccionSegura(nod->left->intervalo, intervalo + nod->rigth->intervalo, a)) {
            return false;
        }
        if (!eval_tree_out(nod->left, a)) {
            return false;
        }

        interval b;
        if (!interseccionSegura(nod->rigth->intervalo, a - intervalo, b)) {
            return false;
        }
        return eval_tree_out(nod->rigth, b);
    }

    case MULT :
    {
        //Each factor projects as a quotient: only when the divisor does
        //not straddle zero (interval division would abort otherwise).
        interval a = nod->left->intervalo;

        if (Inf(nod->rigth->intervalo) > 0.0 || Sup(nod->rigth->intervalo) < 0.0) {
            if (!interseccionSegura(a, intervalo / nod->rigth->intervalo, a)) {
                return false;
            }
            if (!eval_tree_out(nod->left, a)) {
                return false;
            }
        }

        if (Inf(a) > 0.0 || Sup(a) < 0.0) {
            interval b;
            if (!interseccionSegura(nod->rigth->intervalo, intervalo / a, b)) {
                return false;
            }
            return eval_tree_out(nod->rigth, b);
        }

        return true;
    }

    case DIV :
    {
        interval a;
        if (!interseccionSegura(nod->left->intervalo, intervalo * nod->rigth->intervalo, a)) {
            return false;
        }
        if (!eval_tree_out(nod->left, a)) {
            return false;
        }

        if (Inf(intervalo) > 0.0 || Sup(intervalo) < 0.0) {
            interval b;
            if (!interseccionSegura(nod->rigth->intervalo, a / intervalo, b)) {
                return false;
            }
            return eval_tree_out(nod->rigth, b);
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
        if (!interseccionSegura(intervalo, interval(0.0, MaxReal), positivo)) {
            return false;
        }

        const interval raiz = sqrt(positivo);
        if (!interseccionSegura(nod->left->intervalo,
                                interval(-Sup(raiz), Sup(raiz)), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left, candidato);
    }

    case SQRT :
    {
        //sqrt(x) = I: the result is never negative.
        interval positivo;
        if (!interseccionSegura(intervalo, interval(0.0, MaxReal), positivo)) {
            return false;
        }

        if (!interseccionSegura(nod->left->intervalo, sqr(positivo), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left, candidato);
    }

    case SIN :
    {
        interval acotado;
        if (!interseccionSegura(intervalo, interval(-1.0, 1.0), acotado)) {
            return false;
        }

        //Only within the principal monotone branch of the argument.
        if (Inf(nod->left->intervalo) < -M_PI / 2 || Sup(nod->left->intervalo) > M_PI / 2) {
            return true;
        }

        if (!interseccionSegura(nod->left->intervalo, asin(acotado), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left, candidato);
    }

    case COS :
    {
        interval acotado;
        if (!interseccionSegura(intervalo, interval(-1.0, 1.0), acotado)) {
            return false;
        }

        //cos is monotone on [-pi, 0] and on [0, pi]; anything wider is
        //left unprojected.
        if (Inf(nod->left->intervalo) >= -M_PI && Sup(nod->left->intervalo) <= 0.0) {
            if (!interseccionSegura(nod->left->intervalo, -acos(acotado), candidato)) {
                return false;
            }
            return eval_tree_out(nod->left, candidato);
        }

        if (Inf(nod->left->intervalo) >= 0.0 && Sup(nod->left->intervalo) <= M_PI) {
            if (!interseccionSegura(nod->left->intervalo, acos(acotado), candidato)) {
                return false;
            }
            return eval_tree_out(nod->left, candidato);
        }

        return true;
    }

    case TAN :
    {
        if (Inf(nod->left->intervalo) <= -M_PI / 2 || Sup(nod->left->intervalo) >= M_PI / 2) {
            return true;
        }

        if (!interseccionSegura(nod->left->intervalo, atan(intervalo), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left, candidato);
    }

    case ATAN :
    {
        interval acotado;
        if (!interseccionSegura(intervalo, interval(-M_PI / 2, M_PI / 2), acotado)) {
            return false;
        }

        if (!interseccionSegura(nod->left->intervalo, tan(acotado), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left, candidato);
    }

    case ASIN :
    {
        interval acotado;
        if (!interseccionSegura(intervalo, interval(-M_PI / 2, M_PI / 2), acotado)) {
            return false;
        }

        if (!interseccionSegura(nod->left->intervalo, sin(acotado), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left, candidato);
    }

    case ACOS :
    {
        interval acotado;
        if (!interseccionSegura(intervalo, interval(0.0, M_PI), acotado)) {
            return false;
        }

        if (!interseccionSegura(nod->left->intervalo, cos(acotado), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left, candidato);
    }

    case ABS :
    {
        //|x| = I: x in [-Sup(I+), Sup(I+)].
        interval positivo;
        if (!interseccionSegura(intervalo, interval(0.0, MaxReal), positivo)) {
            return false;
        }

        if (!interseccionSegura(nod->left->intervalo,
                                interval(-Sup(positivo), Sup(positivo)), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left, candidato);
    }

    case LN :
    {
        //exp overflows past ~709: skip rather than trap.
        if (Sup(intervalo) > 700.0) {
            return true;
        }

        if (!interseccionSegura(nod->left->intervalo, exp(intervalo), candidato)) {
            return false;
        }
        return eval_tree_out(nod->left, candidato);
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
        return nod->intervalo = eval_tree_in(nod->left) + eval_tree_in(nod->rigth);

    case RESTA :
        return nod->intervalo = eval_tree_in(nod->left) - eval_tree_in(nod->rigth);

    case MULT :
        return nod->intervalo = eval_tree_in(nod->left) * eval_tree_in(nod->rigth);

    case DIV :
        return nod->intervalo = eval_tree_in(nod->left) / eval_tree_in(nod->rigth);

    case POT :
    {
        //An integral square must not go through pow (exp of ln: a base
        //touching zero or negative aborts inside the noexcept library).
        const interval base = eval_tree_in(nod->left);
        const interval exponente = eval_tree_in(nod->rigth);

        if (Inf(exponente) == 2.0 && Sup(exponente) == 2.0) {
            return nod->intervalo = sqr(base);
        }

        return nod->intervalo = pow(base, exponente);
    }

    case SIN :
        return nod->intervalo = sin ( eval_tree_in(nod->left) );

    case COS :
        return nod->intervalo = cos ( eval_tree_in(nod->left) );

    case SQRT :
        return nod->intervalo = sqrt( eval_tree_in(nod->left) );

    case TAN :
        return nod->intervalo = tan ( eval_tree_in(nod->left) );

    case ATAN :
        return nod->intervalo = atan ( eval_tree_in(nod->left) );

    case SINH :
        return nod->intervalo = sinh ( eval_tree_in(nod->left) );

    case COSH :
        return nod->intervalo = cosh ( eval_tree_in(nod->left) );

    case TANH :
        return nod->intervalo = tanh ( eval_tree_in(nod->left) );

    case ASIN :
        return nod->intervalo = asin ( eval_tree_in(nod->left) );

    case ACOS :
        return nod->intervalo = acos ( eval_tree_in(nod->left) );

    case EXP :
        return nod->intervalo = exp ( eval_tree_in(nod->left) );

    case ABS :
        return nod->intervalo = abs ( eval_tree_in(nod->left) );
    default:
        return nod->intervalo;

        /* si se ha añadido otra función, entoces solo agregue otro 'case ' y defina la operación a realizar */

    }
}

/*interval ExpressionTree::eval_tree_complex_interval(exp_node *nod)
{

    complex<qreal> complejo_vacio(1,0);


    switch (nod->type)
    {
    case CONST :
        return interval (nod->c_const);

    case VAR  :

        if (nod->var == "s")
            return complex<qreal> (0, w) * interval(1);

        return  variables_in->at(nod->var) * complejo_vacio;


    case E:
        return interval (M_E) * complejo_vacio;

    case PI:
        return interval (M_PI) * complejo_vacio;

    case SUMA :

        return  eval_tree_complex_interval(nod->left) + eval_tree_complex_interval(nod->rigth);

    case RESTA :
        return  eval_tree_complex_interval(nod->left) - eval_tree_complex_interval(nod->rigth);

    case MULT :
        return  eval_tree_complex_interval(nod->left) * eval_tree_complex_interval(nod->rigth);

    case DIV :
        return  eval_tree_complex_interval(nod->left) / eval_tree_complex_interval(nod->rigth);

    case POT :
        return pow (eval_tree_complex_interval(nod->left),  eval_tree_complex_interval(nod->rigth) );

    case SIN :
        return  sin ( eval_tree_complex_interval(nod->left) );

    case COS :
        return  cos ( eval_tree_complex_interval(nod->left) );

    case SQRT :
        return  sqrt( eval_tree_complex_interval(nod->left) );

    case TAN :
        return  tan ( eval_tree_complex_interval(nod->left) );

    case ATAN :
        return  atan ( eval_tree_complex_interval(nod->left) );

    case SINH :
        return  sinh ( eval_tree_complex_interval(nod->left) );

    case COSH :
        return  cosh ( eval_tree_complex_interval(nod->left) );

    case TANH :
        return  tanh ( eval_tree_complex_interval(nod->left) );

    case ASIN :
        return  asin ( eval_tree_complex_interval(nod->left) );

    case ACOS :
        return  acos ( eval_tree_complex_interval(nod->left) );

    case EXP :
        return  exp ( eval_tree_complex_interval(nod->left) );

        //case ABS :
        //   return  abs ( eval_tree_complex_interval(nod->left) );

    case LN :
        return  ln ( eval_tree_complex_interval(nod->left) );

    case LG :
        return  log10 ( eval_tree_complex_interval(nod->left) );

         // si se ha añadido otra función, entoces solo agregue otro 'case ' y defina la operación a realizar

    }
}*/

/********************************************************
* void ExpressionTree::delete_tree(exp_node *nod)             *
*********************************************************
* esta función se encarga de eliminar un subarbol
* a partir de su nodo raiz, recorriendo el arbol
* en post-orden. Es usada para eliminar el arbol de
* expresiones cuando se le pasa como argumento 'root'
*        ... es una función recursiva .........
*/
void ExpressionTree::delete_tree(exp_node *nod)
{
    if (!nod)
        return;

    delete_tree(nod->left);
    delete_tree(nod->rigth);
    delete nod;
}

/********************************************************
*      exp_node *ExpressionTree::make_cpy(exp_node *nod)      *
*********************************************************
* esta es otra función recursiva, que recorre el árbol
* en pre-orden y por cada nodo crea una nuevo con la misma
* información, para finalmente devolver un puntero a un nuevo
* árbol de expresiones que es una copia del recorrido
* ..... es usada por el constrcutor de copia y por el operador '='
*/
exp_node *ExpressionTree::make_cpy(exp_node *nod)
{
    if (!nod) return 0;

    exp_node *ptr = new exp_node;
    ptr->type  = nod->type;
    ptr->c_const = nod->c_const;

    ptr->left = make_cpy(nod->left);
    ptr->rigth = make_cpy(nod->rigth);

    return ptr;
}

/********************************************************
* void ExpressionTree::build_tree(std::string &in_exp)        *
**********************************************************/
// Con esta función se construye el árbol binario de expresiones
// a partir de la expresión infija contenida en 'in_exp'

void ExpressionTree::build_tree(std::string &in_exp)
{
    pilaOp   pila_op  ;  // pila de operadores
    pilaNode pila_nod;

    std::string tmp_str;

    std::string::size_type i, pos = 0, len = in_exp.length();

    while (pos < len)
    {

        if (in_exp[pos] != '('){
            i = pos;
            while ( (in_exp[i] != '(') && (i < len)) ++i;

            tmp_str = in_exp.substr(pos, i-pos);

            if (tmp_str == "sin" ){
                pila_op.push(SIN);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "cos" ){
                pila_op.push(COS);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "tan" ){
                pila_op.push(TAN);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "atan" ){
                pila_op.push(ATAN);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "exp" ){
                pila_op.push(EXP);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "sinh" ){
                pila_op.push(SINH);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "cosh" ){
                pila_op.push(COSH);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "abs" ){
                pila_op.push(ABS);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "ln" ){
                pila_op.push(LN);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "lg" ){
                pila_op.push(LG);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "asin" ){
                pila_op.push(ASIN);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "acos" ){
                pila_op.push(ACOS);
                pos += (i - pos);
                continue;
            }
            else if (tmp_str == "sqrt"){
                pila_op.push(SQRT);
                pos += (i - pos);
                continue;
            }
            /*se pueden añadir más funciones, poniendo un nuevo else if ()
y modificando el 'enun type_node' y la función 'eval_tree(..)' */

        }

        if ( in_exp[pos] == '(' )  // Ej. "(......" o "....(........"
        {
            pila_op.push(PAR); ++pos;    // se apila siempre
        }
        else if ( in_exp[pos] ==')' )
        {
            while ( pila_op.top() != PAR )  // se desapilan operadores hasta que se encuentre '(' PAR
            {
                root = new exp_node;
                root->type = pila_op.top();
                if (root->type  < SIN) // si es un operador binario
                {
                    root->rigth = pila_nod.top(); pila_nod.pop();
                    root->left = pila_nod.top(); pila_nod.pop();
                }
                else                   // si es unario
                {
                    root->left = pila_nod.top(); pila_nod.pop();
                    root->rigth = nullptr;
                }
                pila_op.pop();
                pila_nod.push(root);
            }
            pila_op.pop(); // se desapila PAR
            ++pos;
        }
        else if (!pos && in_exp[pos] == '-' && isdigit(in_exp[pos+1]) ) // Ej. : "-34.89....." (constante negativa) Forma 1
        {
            root = new exp_node;
            root->left = nullptr;
            root->rigth = nullptr;
            root->type = CONS;

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
            } // se lee la constante completa

            root->c_const = QString::fromUtf8(in_exp.substr(i, pos-i).c_str()).toDouble();

            pila_nod.push(root);
        }
        else if ( pos && in_exp[pos] == '-' && in_exp[pos-1] == '(' && isdigit(in_exp[pos+1]) )//Ej. "..(-34.89...." (constante negativa)
        {
            root = new exp_node;
            root->left = nullptr;
            root->rigth = nullptr;
            root->type = CONS;

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

            root->c_const = QString::fromUtf8(in_exp.substr(i, pos-i).c_str()).toDouble();

            pila_nod.push(root);
        }
        else if ( isdigit(in_exp[pos]) )// Ej. : "....67.009.." ( constante positiva )
        {
            root = new exp_node;
            root->left = nullptr;
            root->rigth = nullptr;
            root->type = CONS;

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

            root->c_const = QString::fromUtf8(in_exp.substr(i, pos-i).c_str()).toDouble();

            pila_nod.push(root);
        }
        else if ( in_exp[pos] == 'E' || in_exp[pos] == 'P' ) // Las constantes PI y el numero E
        {
            root = new exp_node;
            root->left = nullptr;
            root->rigth = nullptr;
            if (in_exp[pos] == 'E')
            {
                root->type = E;
                pos++;
            }
            else
            {
                root->type = PI;
                pos+=2;
            }
            pila_nod.push(root);
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

            root = new exp_node;
            root->left = 0;
            root->rigth = 0;
            root->type = VAR;
            root->var = tmp_str;
            pila_nod.push(root);
        }
        else if ( !pos && in_exp[pos] == '-' ) // Ej. : "-sin(.........." o "-x........" (- unario) Forma 1
        {                                      // En este caso una expresión del tipo "-sin(.........." ó "-x........"
            root = new exp_node;               // será tratada como "-1*sin(.........." y  "-1*x........"
            root->type = CONS;
            root->left = 0;
            root->rigth = 0;
            root->c_const = -1.0000000000000000000;
            pila_nod.push(root);

            while ( !pila_op.empty() && pila_op.top() > RESTA )
            {
                root = new exp_node;
                root->type = pila_op.top();
                if  ( root->type < SIN )
                {
                    root->rigth = pila_nod.top(); pila_nod.pop();
                    root->left = pila_nod.top(); pila_nod.pop();
                }
                else
                {
                    root->left = pila_nod.top(); pila_nod.pop();
                    root->rigth = 0;
                }
                pila_nod.push(root);
                pila_op.pop();
            }
            pila_op.push(MULT);
            ++pos;
        }
        else if ( in_exp[pos] == '-' && in_exp[pos-1] == '(' ) // Ej. : "...(-sin..." o "...(-x..." (- unario) Forma 2
        {                                                      // En este caso una expresión del tipo "...(-sin..." ó "...(-x..."
            root = new exp_node;                               // será tratada como "..(-1*sin...." y  "...(-1*x...."
            root->type = CONS;
            root->left = 0;
            root->rigth = 0;
            root->c_const = -1.0000000000000000000;
            pila_nod.push(root);

            while ( !pila_op.empty() && pila_op.top() > RESTA )
            {
                root = new exp_node;
                root->type = pila_op.top();
                if  ( root->type < SIN )
                {
                    root->rigth = pila_nod.top(); pila_nod.pop();
                    root->left = pila_nod.top(); pila_nod.pop();
                }
                else
                {
                    root->left = pila_nod.top(); pila_nod.pop();
                    root->rigth = 0;
                }
                pila_nod.push(root);
                pila_op.pop();
            }
            pila_op.push(MULT);
            ++pos;
        }
        else if ( in_exp[pos] == '-' ) // Ej: "....x-y..."     operador '-' binario
        {
            while ( !pila_op.empty() && pila_op.top() != PAR )
            {
                root = new exp_node;
                root->type = pila_op.top();

                if  ( root->type < SIN )
                {
                    root->rigth = pila_nod.top(); pila_nod.pop();
                    root->left  = pila_nod.top(); pila_nod.pop();
                }
                else
                {
                    root->left  = pila_nod.top(); pila_nod.pop();
                    root->rigth = 0;
                }
                pila_nod.push(root);
                pila_op.pop();
            }
            pila_op.push(RESTA);
            ++pos;
        }
        else if ( in_exp[pos] == '+' ) // Ej: "....x+y..."     operador '+' binario
        {
            while ( !pila_op.empty() && pila_op.top() != PAR )
            {
                root = new exp_node;
                root->type = pila_op.top();
                if  ( root->type < SIN )
                {
                    root->rigth = pila_nod.top(); pila_nod.pop();
                    root->left = pila_nod.top(); pila_nod.pop();
                }
                else
                {
                    root->left = pila_nod.top(); pila_nod.pop();
                    root->rigth = 0;
                }
                pila_nod.push(root);
                pila_op.pop();
            }
            pila_op.push(SUMA);
            ++pos;
        }
        else if ( in_exp[pos] == '/' )// Ej: "....x/y..."     operador '/' binario
        {
            while ( !pila_op.empty() && pila_op.top() > RESTA )
            {
                root = new exp_node;
                root->type = pila_op.top();
                if  ( root->type < SIN )
                {
                    root->rigth = pila_nod.top(); pila_nod.pop();
                    root->left = pila_nod.top(); pila_nod.pop();
                }
                else
                {
                    root->left = pila_nod.top(); pila_nod.pop();
                    root->rigth = 0;
                }
                pila_nod.push(root);
                pila_op.pop();
            }
            pila_op.push(DIV);
            ++pos;
        }
        else if ( in_exp[pos] == '*' )// Ej: "....x*y..."     operador '*' binario
        {
            while ( !pila_op.empty() && pila_op.top() > RESTA )
            {
                root = new exp_node;
                root->type = pila_op.top();
                if  ( root->type < SIN )
                {
                    root->rigth = pila_nod.top(); pila_nod.pop();
                    root->left = pila_nod.top();  pila_nod.pop();
                }
                else
                {
                    root->left = pila_nod.top(); pila_nod.pop();
                    root->rigth = 0;
                }
                pila_nod.push(root);
                pila_op.pop();
            }
            pila_op.push(MULT);
            ++pos;
        }
        else if ( in_exp[pos] == '^' ) // Ej: "....x^y..."     operador '^' binario
        {
            while ( !pila_op.empty() && pila_op.top() > DIV )
            {
                root = new exp_node;
                root->type = pila_op.top();
                if  ( root->type == POT )
                {
                    root->rigth = pila_nod.top(); pila_nod.pop();
                    root->left = pila_nod.top(); pila_nod.pop();
                }
                else
                {
                    root->left = pila_nod.top(); pila_nod.pop();
                    root->rigth = 0;
                }
                pila_nod.push(root);
                pila_op.pop();
            }
            pila_op.push(POT);
            ++pos;
        }
    }

    while ( !pila_op.empty() )
    {
        root = new exp_node;
        root->type = pila_op.top();

        if ( root->type < SIN )
        {
            root->rigth = pila_nod.top(); pila_nod.pop();
            root->left = pila_nod.top(); pila_nod.pop();
        }
        else
        {
            root->left = pila_nod.top(); pila_nod.pop();
            root->rigth = 0;
        }
        pila_op.pop();
        pila_nod.push(root);
    }

    root  = pila_nod.top(); pila_nod.pop();

}

bool ExpressionTree::es_letra(char tex){
    static QRegularExpression rx("^[a-zA-Z]*$");
    QString s (1, tex);

    return rx.match(s).hasMatch();
}

//////////////////////////////////////////
// IMPLEMENTACION DE LA CLASE pilaNode  //
//////////////////////////////////////////
void pilaNode::pop()
{
    if (!n)
        return;
    else
    {
        node *ptr_tmp = head->next;  //se guarda la dirección del segundo nodo
        delete head;                 //se borra el nodo de la cabecera
        head = ptr_tmp;              //se restablece la cabecera al segundo nodo
        --n;
    }
}

void pilaNode::push(exp_node *ptr)
{

    if (!n)
    {

        head = new node;
        head->ptr = ptr;
        head->next = 0;
        ++n;
    }
    else
    {
        node *ptr_tmp = new node;
        ptr_tmp->next = head;
        head = ptr_tmp;
        head->ptr = ptr;
        ++n;
    }
}

pilaNode::~pilaNode()
{
    node *ptr_tmp = head;
    while (ptr_tmp)             //mientras que no se encuentre el final de la lista
    {
        ptr_tmp = head->next;   //ptr_tmp apunta al nodo siguiente
        delete head;            //se borra el nodo actual
        head = ptr_tmp;         //head y ptr_tmp vuelven a apuntar al mismo nodo (el siguinte)
    }
}

//////////////////////////////////////////
// IMPLEMENTACION DE LA CLASE pilaOp    //
//////////////////////////////////////////
void pilaOp::pop()
{
    if (!n)
        return;
    else
    {
        node *ptr_tmp = head->next;
        delete head;
        head = ptr_tmp;
        --n;
    }
}

void pilaOp::push(type_node value)
{

    if (!n)
    {
        head = new node;
        head->value = value;
        head->next = 0;
        ++n;
    }
    else
    {
        node *ptr_tmp = new node;
        ptr_tmp->next = head;
        head = ptr_tmp;
        head->value = value;
        ++n;
    }
}

pilaOp::~pilaOp()
{
    node *ptr_tmp = head;
    while (ptr_tmp)
    {
        ptr_tmp = head->next;
        delete head;
        head = ptr_tmp;
    }
}

};
