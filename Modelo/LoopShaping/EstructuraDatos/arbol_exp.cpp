/*
Roberto C. Cruz Rodríguez
    rcruz@instec.cu
*/
// Este interprete de funciones se basa en la creación de un árbol binario de expresiones
// donde cada nodo representa una operación a realizar y cada hoja un valor númerico

#include "arbol_exp.h"

#define PI1 3.1415926535897936
#define E1 2.71828182846

using namespace std;
using namespace cxsc;


namespace alg {

//////////////////////////////////////////
// IMPLEMENTACION DE LA CLASE arbol_exp //
//////////////////////////////////////////
exp_tree::exp_tree()
{
    root = nullptr;
}

exp_tree::exp_tree(const char *tex)
{
    root = nullptr;
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // aquí se eliminan los espacios en blanco de la entrada
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // para facilitar la lectura de la expresión matemática

    build_tree(in_exp);
}

exp_tree::exp_tree(const std::string &tex, qreal resultado, com comparacion)
{
    root = nullptr;
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // aquí se eliminan los espacios en blanco de la entrada
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // para facilitar la lectura de la expresión matemática

    this->numero_comparar = resultado;
    this->comparacion = comparacion;

    build_tree(in_exp);
}

exp_tree::exp_tree(const exp_tree &other )                                            // constructor de copia
{
    this->root = make_cpy( other.root );
}

exp_tree::~exp_tree()                                                                 // destructor
{
    delete_tree(root);
}

/********************************************************
* void exp_tree::setFunc(const std::string &tex)        *
*********************************************************
* Con esta función se elimina el
* árbol actual y se construye otro
* a partir de una nueva expresión
*/

void exp_tree::setFunc(const std::string &tex)
{
    delete_tree(root);
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // aquí se eliminan los espacios en blanco de la entrada
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // para facilitar la lectura de la expresión matemática

    numero_comparar = 0;
    comparacion = MAYORIGUAL;

    build_tree(in_exp);
}

void exp_tree::setFunc(const std::string &tex, qreal resultado, com comparacion)
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
* void exp_tree::setFunc(const char *tex)               *
*********************************************************
*         La misma que la anterior
*/
void exp_tree::setFunc(const char *tex)
{
    delete_tree(root);
    std::string in_exp = tex;

    std::string::size_type pos = 0;                                                   // aquí se eliminan los espacios en blanco de la entrada
    while (std::string::npos != ( pos = in_exp.find(" ",pos) ) ) in_exp.erase(pos,1); // para facilitar la lectura de la expresión matemática

    build_tree(in_exp);
}

/********************************************************
* exp_tree &exp_tree::operator=(const exp_tree &other)  *
*********************************************************
* Esta función nos permite evaluar
* la expresión matemática
*/
qreal exp_tree::eval(QMap<std::string, qreal> *variables )
{
    this->variables = variables;

    return eval_tree(root);
}

void exp_tree::imprimir (){
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

void exp_tree::alg_exp_node_print(exp_node * node){



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

string exp_tree::tipo(type_node tipo)  {

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

interval exp_tree::eval(QMap<string, interval > *variables){
    this->variables_in = variables;

    return eval_tree_in(root);
}

/*interval exp_tree::eval(QMap<string, interval > *variables, qreal w){

    this->w = w;
    this->variables_in = variables;

    return eval_tree_complex_interval(root);
}*/

/********************************************************
* exp_tree &exp_tree::operator=(const exp_tree &other)  *
*********************************************************
* la homonimia del operador '=', nos
* permite especificar como debe ser la
* asignacion de un objeto del tipo exp_tree
* a otro ......
*/
exp_tree &exp_tree::operator=(const exp_tree &other)
{
    delete_tree(root);
    root = make_cpy(other.root);
}

/*****************************************************************************
* double exp_tree::operator()(double xx , double yy , double zz , double tt) *
******************************************************************************
* la homonimia del operador '()' nos permite
* evaluar la expresion usando parentesis
* de esta forma result = function(78,0,0,1)
*/
qreal exp_tree::operator()(QMap <std::string, qreal> * variables)
{
    return eval (variables);
}

interval exp_tree::operator ()(QMap <std::string, interval > * variables){
    return eval (variables);
}

/********************************************************
*    double exp_tree::eval_tree(exp_node *nod)          *
*********************************************************
* esta es la función más importante de la clase ...
* es una función recursiva que recorre el arbol binario
* de expresiones y realiza la operacion matemática corres-
* pondiente en cada nodo para finalmete, devolver el resultado
* de evaluar la expresión contenida en el arbol .......
*/
qreal exp_tree::eval_tree(exp_node *nod)
{

    switch (nod->type)
    {
    case CONS :
        return nod->c_const;

    case VAR  :
        return variables->value(nod->var);

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


bool exp_tree::recorrer(QMap<string, interval > *variables){

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

    eval_tree_out(root, nuevo_intervalo);


    return true;
}

interval exp_tree::interseccion(interval a, interval b){

    assert((Sup(a) > Inf(b)) && (Sup(b) > Inf(a)));

    return interval ((Inf(a) > Inf(b) ? Inf(a) : Inf(b)),
                            (Sup(a) < Sup(b) ? Sup(a) : Sup(b)));
}

interval exp_tree::unio(interval a, interval b){

    assert((Sup(a) > Inf(b)) || (Sup(b) > Inf(a)));

    return interval ((Inf(a) > Inf(b) ? Inf(b) : Inf(a)),
                            (Sup(a) < Sup(b) ? Sup(b) : Sup(a)));
}

void exp_tree::eval_tree_out(exp_node *nod, interval intervalo){

    switch (nod->type)
    {

    case CONS :
        return;

    case VAR  :
    {
        variables_in->insert(nod->var, intervalo);
        return;
    }

    case SUMA :
    {
        interval left = nod->left->intervalo;
        interval rigth = nod->rigth->intervalo;

        interval b = intervalo - rigth;

        interval a = interseccion(left, b);
        eval_tree_out(nod->left, a);

        eval_tree_out(nod->rigth, interseccion(rigth, intervalo - a));

        return;
    }

    case RESTA :
    {

        interval a = interseccion(nod->left->intervalo, intervalo + nod->rigth->intervalo);
        eval_tree_out(nod->left, a);

        eval_tree_out(nod->rigth, interseccion(nod->rigth->intervalo, -intervalo + a));

        return;
    }

    case MULT :
    {
        interval a = interseccion(nod->left->intervalo, intervalo / nod->rigth->intervalo);
        eval_tree_out(nod->left, a);

        eval_tree_out(nod->rigth, interseccion(nod->rigth->intervalo, intervalo / a));

        return;
    }

    case DIV :
    {

        interval a = interseccion(nod->left->intervalo, intervalo * nod->rigth->intervalo);
        eval_tree_out(nod->left, a);

        eval_tree_out(nod->rigth, interseccion(nod->rigth->intervalo, a / intervalo ));

        return;
    }

    case POT :
    {

        interval inter = pow(intervalo, 1 / interval(Inf(nod->rigth->intervalo)));
        eval_tree_out(nod->left, interseccion(nod->left->intervalo, interval(-Sup(inter), Sup(inter))));

        return;
    }

    case SQRT :
    {
        interval inter = pow(intervalo, interval(2));
        eval_tree_out(nod->left, interseccion(nod->left->intervalo, interval(-Sup(inter), Sup(inter))));

        return;
    }
    case SIN :
    {

        eval_tree_out(nod->left, interseccion(nod->left->intervalo, asin(intervalo)));

        return;
    }

    case COS :
    {

        eval_tree_out(nod->left, interseccion(nod->left->intervalo, -acos(intervalo)));

        return;
    }

    case TAN :
    {

        eval_tree_out(nod->left, interseccion(nod->left->intervalo, atan(intervalo)));

        return;
    }

    case ATAN :
    {

        eval_tree_out(nod->left, interseccion(nod->left->intervalo, tan(intervalo)));

        return;
    }

    case ASIN :
    {

        eval_tree_out(nod->left, interseccion(nod->left->intervalo, sin(intervalo)));

        return;
    }

    case ACOS :
    {

        eval_tree_out(nod->left, interseccion(nod->left->intervalo, cos(intervalo)));

        return;
    }

    case ABS :
    {

        fabs ( eval_tree(nod->left) );

        return;
    }

    case LN :
    {

        eval_tree_out(nod->left, interseccion(nod->left->intervalo, exp(intervalo)));

        return;
    }

        /* si se ha añadido otra función, entoces solo agregue otro 'case ' y defina la operación a realizar */

    default:
        return;
    }
}

interval exp_tree::eval_tree_in(exp_node *nod)
{

    switch (nod->type)
    {
    case CONS :
        return nod->intervalo = interval (nod->c_const);

    case VAR  :
        return nod->intervalo = variables_in->value(nod->var);

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
        return nod->intervalo = pow (eval_tree_in(nod->left),  interval(Inf(eval_tree_in(nod->rigth))));

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

/*interval exp_tree::eval_tree_complex_interval(exp_node *nod)
{

    complex<qreal> complejo_vacio(1,0);


    switch (nod->type)
    {
    case CONST :
        return interval (nod->c_const);

    case VAR  :

        if (nod->var == "s")
            return complex<qreal> (0, w) * interval(1);

        return  variables_in->value(nod->var) * complejo_vacio;


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
* void exp_tree::delete_tree(exp_node *nod)             *
*********************************************************
* esta función se encarga de eliminar un subarbol
* a partir de su nodo raiz, recorriendo el arbol
* en post-orden. Es usada para eliminar el arbol de
* expresiones cuando se le pasa como argumento 'root'
*        ... es una función recursiva .........
*/
void exp_tree::delete_tree(exp_node *nod)
{
    if (!nod)
        return;

    delete_tree(nod->left);
    delete_tree(nod->rigth);
    delete nod;
}

/********************************************************
*      exp_node *exp_tree::make_cpy(exp_node *nod)      *
*********************************************************
* esta es otra función recursiva, que recorre el árbol
* en pre-orden y por cada nodo crea una nuevo con la misma
* información, para finalmente devolver un puntero a un nuevo
* árbol de expresiones que es una copia del recorrido
* ..... es usada por el constrcutor de copia y por el operador '='
*/
exp_node *exp_tree::make_cpy(exp_node *nod)
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
* void exp_tree::build_tree(std::string &in_exp)        *
**********************************************************/
// Con esta función se construye el árbol binario de expresiones
// a partir de la expresión infija contenida en 'in_exp'

void exp_tree::build_tree(std::string &in_exp)
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
            while ( isdigit(in_exp[pos]) || in_exp[pos] == '.' ) ++pos; // se lee la constante completa

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

            while ( (es_letra(in_exp[i]) && (i < len))) ++i;

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

bool exp_tree::es_letra(char tex){
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
