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
* Contiene una lista de las operaciones y funciones soportadas por el interprete.
*
* CONST, PI, E, VARX, VARY, VARZ, VART : siempre serán nodos hojas poque representan un valor (operando)
*
* SUMA, RESTA, MULT, DIV, POT : son los operadores binarios soportados por el intérprete, siempre serán nodos binarios
*
* SIN, COS, LN, ABS, .... : son los operadores unarios soportados, siempre serán nodos unarios
*/
enum type_node { CONS, PI, E, VAR,
                 PAR,
                 SUMA, RESTA, MULT, DIV, POT,
                 SIN, COS, TAN, SINH, COSH, ATAN, TANH, ASIN,
                 ACOS, EXP, ABS, LN, LG, SQRT };

enum com { MAYOR, MENOR, MAYORIGUAL, MENORIGUAL, IGUAL};

/**
*  Node del árbol binario de expresiones.
*/
struct exp_node
{
    /**
* c_const : este campo almacena un valor númerico, que puede el valor de una variable (x, y, z, t)
* o bien un valor constante contenido en la expresión matemática. Este campo solo es usado cuando el nodo
* no tiene más ramificaciones (nodo hoja)
*/
    double c_const;

    /**
* type : indica el tipo de nodo, en el árbol de expresiones cada nodo indica una operación a realizar
* o bien almacena un valor númerico (que estará en c_const), en cuyo caso la operación a realizar es
* retornar dicho valor.
*/
    type_node type;

    /**
* var : indica el identificador de la variable.
*/
    std::string var;

    /**
* intervalo : indica el valor del intervalo en ese nodo;
*/
    interval intervalo;

    /**
* left : es un puntero a otro nodo, es la rama izquierda del nodo, cuando left contiene el valor NULL
* entones el nodo no tiene rama izquierda.
*/
    exp_node *left;

    /**
* rigth : es un puntero a otro nodo, es la rama derecha del nodo, cuando rigth contiene el valor NULL
* entones el nodo no tiene rama derecha.
*/
    exp_node *rigth;
};

/**
*  La clase  ExpressionTree define un arbol binario de expresiones.
*  Esta clase implementa un interprete de funciones de cuatro variables
*  x, y, z, y t.
*/
class ExpressionTree
{
public :
    /**
* Constructor simple.
*/
    ExpressionTree();

    /**
* Constructor simple.
* @param tex : es una referencia a un objeto std::string
* donde esta contenida la expresión matemática, dicha expresión
* debe contener solamente carácteres alfanúmericos y espacios en blanco
*/
    ExpressionTree(const std::string &tex, qreal num, com comparacion);

    /**
* Constructor simple.
* @param tex : es un puntero a una cadena de carácteres
* donde esta contenida la expresión matemática, dicha cadena
* debe contener solamente carácteres alfanúmericos y espacios en blanco
*/
    ExpressionTree(const char *tex);

    /**
* Constructor de copia.
* @param other : es una referencia a otra objeto del tipo ExpressionTree,
*
*/
    ExpressionTree(const ExpressionTree & other);
    ~ExpressionTree();

    /**
* Establece una nueva expresión matemática.
* @param tex : es una referencia a un objeto std::string
* donde esta contenida la expresión matemática, dicha expresión
* debe contener solamente carácteres alfanúmericos y espacios en blanco
*/
    void setFunc(const std::string &tex);

    void setFunc(const std::string &tex, qreal resultado, com comparacion);

    /**
* Establece una nueva expresión matemática.
* @param tex : es un puntero a una cadena de carácteres
* donde esta contenida la expresión matemática, dicha cadena
* debe contener solamente carácteres alfanúmericos y espacios en blanco
*/
    void setFunc(const char *tex);

    /**
* Evalua la expresión matemática.
* @return Se retorna el resultado de evaluar la expresión matemática.
*/
    qreal eval(QMap <std::string, qreal> * variables = NULL);

    interval eval (QMap<std::string, interval > *variables);///////////////////////////////////////////

    //interval eval (QMap<std::string, interval > *variables, qreal w);///////////////////////////////////////////

    bool propagate (QMap<std::string, interval > *variables);

    void imprimir ();

    /**
* Operador de asignación.
*/
    ExpressionTree &operator=(const ExpressionTree & other);

    /**
* Operador de paréntesis ().
* El operador de paréntesis, es una alternativa
* a la función miembro eval, para evaluar la expresión matemática.
* @return Se retorna resultado de evaluar la expresión.
*/
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
*   Pila de Nodos
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
*   Pila de Operadores
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
