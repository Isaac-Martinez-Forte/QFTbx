#ifndef AVL_H
#define AVL_H

// Plantilla de Arbol AVL en C++
// (C) Mayo 2002, Salvador Pozo
// C con Clase: http://c.conclase.net

#include <QtCore>

#include "nodo.h"
#include "tripleta.h"

template<class DATO>
class AVL {
private:
    enum {IZQUIERDO, DERECHO};
    // Punteros de la lista, para cabeza y nodo actual:
    Nodo<DATO> *raiz;
    Nodo<DATO> *actual;
    qint32 contador;
    qint32 altura;

public:
    // Constructor y destructor básicos:
    AVL();
    ~AVL();

    // Insertar en árbol ordenado:
    void Insertar(const DATO dat);
    // Borrar un elemento del árbol:
    void Borrar(const DATO dat);
    // Función de búsqueda:
    bool Buscar(const DATO dat);
    // Comprobar si el árbol está vacío:
    bool Vacio(Nodo<DATO> *r) { return r==NULL; }
    // Comprobar si es un nodo hoja:
    bool EsHoja(Nodo<DATO> *r) { return !r->derecho && !r->izquierdo; }
    // Contar número de nodos:
    qint32 NumeroNodos();
    qint32 AlturaArbol();
    // Calcular altura de un dato:
    qint32 Altura(const DATO dat);
    // Devolver referencia al dato del nodo actual:
    DATO &ValorActual() { return actual->dato; }
    // Moverse al nodo raiz:
    void Raiz() { actual = raiz; }
    // Aplicar una función a cada elemento del árbol:
    void InOrden(Nodo<DATO> *nodo=NULL, bool r=true);
    void PreOrden(Nodo<DATO> *nodo=NULL, bool r=true);
    void PostOrden(Nodo<DATO> *nodo=NULL, bool r=true);

    DATO get_menor_tripleta();

private:
    // Funciones de equilibrado:
    void Equilibrar(Nodo<DATO> *nodo, qint32, bool);
    void RSI(Nodo<DATO>* nodo);
    void RSD(Nodo<DATO>* nodo);
    void RDI(Nodo<DATO>* nodo);
    void RDD(Nodo<DATO>* nodo);
    // Funciones auxiliares
    void Podar(Nodo<DATO>* &);
    void auxContador(Nodo<DATO>*);
    void auxAltura(Nodo<DATO>*, qint32);
};

template<class DATO>
AVL<DATO>::AVL(){
    raiz = NULL;
    actual = NULL;
}

template<class DATO>
AVL<DATO>::~AVL(){
    Podar(raiz);
}

// Poda: borrar todos los nodos a partir de uno, incluido
template<class DATO>
void AVL<DATO>::Podar(Nodo<DATO>* &nodo)
{
    // Algoritmo recursivo, recorrido en postorden
    if(nodo) {
        Podar(nodo->izquierdo); // Podar izquierdo
        Podar(nodo->derecho);   // Podar derecho
        delete nodo;            // Eliminar nodo
        nodo = NULL;
    }
}

// Insertar un dato en el árbol AVL
template<class DATO>
void AVL<DATO>::Insertar(const DATO dat)
{
    Nodo<DATO> *padre = NULL;

    actual = raiz;
    // Buscar el dato en el árbol, manteniendo un puntero al nodo padre
    while(!Vacio(actual) && dat != actual->dato) {
        padre = actual;
        if(dat > actual->dato){
            actual = actual->derecho;
        }
        else if(dat < actual->dato){
            actual = actual->izquierdo;
        }
    }

    // Si se ha encontrado el elemento, regresar sin insertar
    if(!Vacio(actual)){
        return;
    }
    // Si padre es NULL, entonces el árbol estaba vacío, el nuevo nodo será
    // el nodo raiz
    if(Vacio(padre)){
        raiz = new Nodo<DATO>(dat);
    }
    // Si el dato es menor que el que contiene el nodo padre, lo insertamos
    // en la rama izquierda
    else if(dat < padre->dato) {
        padre->izquierdo = new Nodo<DATO>(dat, padre);
        Equilibrar(padre, IZQUIERDO, true);
    }
    // Si el dato es mayor que el que contiene el nodo padre, lo insertamos
    // en la rama derecha
    else if(dat > padre->dato) {
        padre->derecho = new Nodo<DATO>(dat, padre);
        Equilibrar(padre, DERECHO, true);
    }
}

// Equilibrar árbol AVL partiendo del nodo nuevo
template<class DATO>
void AVL<DATO>::Equilibrar(Nodo<DATO> *nodo, int rama, bool nuevo)
{
    bool salir = false;

    // Recorrer camino inverso actualizando valores de FE:
    while(nodo && !salir) {
        if(nuevo){
            if(rama == IZQUIERDO){
                nodo->FE--; // Depende de si añadimos ...
            } else{
                nodo->FE++;
            }
        } else{
            if(rama == IZQUIERDO){
                nodo->FE++; // ... o borramos
            } else{
                nodo->FE--;
            }
        }
        if(nodo->FE == 0){
            salir = true; // La altura de las rama que
        // empieza en nodo no ha variado,
        // salir de Equilibrar
        }else if(nodo->FE == -2) { // Rotar a derechas y salir:
            if(nodo->izquierdo->FE == 1) RDD(nodo); // Rotación doble
            else RSD(nodo);                         // Rotación simple
            salir = true;
        }
        else if(nodo->FE == 2) {  // Rotar a izquierdas y salir:
            if(nodo->derecho->FE == -1) RDI(nodo); // Rotación doble
            else RSI(nodo);                        // Rotación simple
            salir = true;
        }
        if(nodo->padre){
            if(nodo->padre->derecho == nodo){
                rama = DERECHO;
            }else{
                rama = IZQUIERDO;
            }
        }

        nodo = nodo->padre; // Calcular FE, siguiente nodo del camino.
    }
}

// Rotación doble a derechas
template<class DATO>
void AVL<DATO>::RDD(Nodo<DATO>* nodo)
{
    Nodo<DATO> *Padre = nodo->padre;
    Nodo<DATO> *P = nodo;
    Nodo<DATO> *Q = P->izquierdo;
    Nodo<DATO> *R = Q->derecho;
    Nodo<DATO> *B = R->izquierdo;
    Nodo<DATO> *C = R->derecho;

    if(Padre){
        if(Padre->derecho == nodo){
            Padre->derecho = R;
        }else{
            Padre->izquierdo = R;
        }
    }else{
        raiz = R;
    }

    // Reconstruir árbol:
    Q->derecho = B;
    P->izquierdo = C;
    R->izquierdo = Q;
    R->derecho = P;

    // Reasignar padres:
    R->padre = Padre;
    P->padre = Q->padre = R;
    if(B){
        B->padre = Q;
    }
    if(C){
        C->padre = P;
    }

    // Ajustar valores de FE:
    switch(R->FE) {
    case -1: Q->FE = 0; P->FE = 1;
        break;
    case 0:  Q->FE = 0; P->FE = 0;
        break;
    case 1:  Q->FE = -1; P->FE = 0;
        break;
    }
    R->FE = 0;
}

// Rotación doble a izquierdas
template<class DATO>
void AVL<DATO>::RDI(Nodo<DATO>* nodo)
{
    Nodo<DATO> *Padre = nodo->padre;
    Nodo<DATO> *P = nodo;
    Nodo<DATO> *Q = P->derecho;
    Nodo<DATO> *R = Q->izquierdo;
    Nodo<DATO> *B = R->izquierdo;
    Nodo<DATO> *C = R->derecho;

    if(Padre){
        if(Padre->derecho == nodo){
            Padre->derecho = R;
        }else{
            Padre->izquierdo = R;
        }
    } else raiz = R;

    // Reconstruir árbol:
    P->derecho = B;
    Q->izquierdo = C;
    R->izquierdo = P;
    R->derecho = Q;

    // Reasignar padres:
    R->padre = Padre;
    P->padre = Q->padre = R;
    if(B){
        B->padre = P;
    }
    if(C){
        C->padre = Q;
    }

    // Ajustar valores de FE:
    switch(R->FE) {
    case -1: P->FE = 0; Q->FE = 1;
        break;
    case 0:  P->FE = 0; Q->FE = 0;
        break;
    case 1:  P->FE = -1; Q->FE = 0;
        break;
    }
    R->FE = 0;
}

// Rotación simple a derechas
template<class DATO>
void AVL<DATO>::RSD(Nodo<DATO>* nodo)
{
    Nodo<DATO> *Padre = nodo->padre;
    Nodo<DATO> *P = nodo;
    Nodo<DATO> *Q = P->izquierdo;
    Nodo<DATO> *B = Q->derecho;

    if(Padre){
        if(Padre->derecho == P){
            Padre->derecho = Q;
        }else{
            Padre->izquierdo = Q;
        }
    }else{
        raiz = Q;
    }

    // Reconstruir árbol:
    P->izquierdo = B;
    Q->derecho = P;

    // Reasignar padres:
    P->padre = Q;
    if(B){
        B->padre = P;
    }
    Q->padre = Padre;

    // Ajustar valores de FE:
    P->FE = 0;
    Q->FE = 0;
}

// Rotación simple a izquierdas
template<class DATO>
void AVL<DATO>::RSI(Nodo<DATO>* nodo)
{
    Nodo<DATO> *Padre = nodo->padre;
    Nodo<DATO> *P = nodo;
    Nodo<DATO> *Q = P->derecho;
    Nodo<DATO> *B = Q->izquierdo;

    if(Padre){
        if(Padre->derecho == P){
            Padre->derecho = Q;
        } else{
            Padre->izquierdo = Q;
        }
    }else{
        raiz = Q;
    }

    // Reconstruir árbol:
    P->derecho = B;
    Q->izquierdo = P;

    // Reasignar padres:
    P->padre = Q;
    if(B){
        B->padre = P;
    }
    Q->padre = Padre;

    // Ajustar valores de FE:
    P->FE = 0;
    Q->FE = 0;
}

// Eliminar un elemento de un árbol AVL
template<class DATO>
void AVL<DATO>::Borrar(const DATO dat)
{
    Nodo<DATO> *padre = NULL;
    Nodo<DATO> *nodo;
    DATO aux;

    if(EsHoja(raiz)){
        Podar(raiz);
        return;
    }

    actual = raiz;

    // Mientras sea posible que el valor esté en el árbol
    while(!Vacio(actual)) {
        if(dat == actual->dato) { // Si el valor está en el nodo actual
            if(EsHoja(actual)) { // Y si además es un nodo hoja: lo borramos
                if(padre){ // Si tiene padre (no es el nodo raiz)
                    // Anulamos el puntero que le hace referencia
                    if(padre->derecho == actual){
                        padre->derecho = NULL;
                    }else if(padre->izquierdo == actual){
                        padre->izquierdo = NULL;
                    }
                }
                //delete actual; // Borrar el nodo
                actual = NULL;
                // El nodo padre del actual puede ser ahora un nodo hoja, por lo tanto su
                // FE es cero, pero debemos seguir el camino a partir de su padre, si existe.
                if((padre->derecho == actual && padre->FE == 1) ||
                        (padre->izquierdo == actual && padre->FE == -1)) {
                    padre->FE = 0;
                    actual = padre;
                    padre = actual->padre;
                }
                if(padre){
                    if(padre->derecho == actual){
                        Equilibrar(padre, DERECHO, false);
                    }else{
                        Equilibrar(padre, IZQUIERDO, false);
                    }
                }
                return;
            }
            else { // Si el valor está en el nodo actual, pero no es hoja
                // Buscar nodo
                padre = actual;
                // Buscar nodo más izquierdo de rama derecha
                if(actual->derecho) {
                    nodo = actual->derecho;
                    while(nodo->izquierdo) {
                        padre = nodo;
                        nodo = nodo->izquierdo;
                    }
                }
                // O buscar nodo más derecho de rama izquierda
                else {
                    nodo = actual->izquierdo;
                    while(nodo->derecho) {
                        padre = nodo;
                        nodo = nodo->derecho;
                    }
                }
                // Intercambiar valores de no a borrar u nodo encontrado
                // y continuar, cerrando el bucle. El nodo encontrado no tiene
                // por qué ser un nodo hoja, cerrando el bucle nos aseguramos
                // de que sólo se eliminan nodos hoja.
                aux = actual->dato;
                actual->dato = nodo->dato;
                nodo->dato = aux;
                actual = nodo;
            }
        } else { // Todavía no hemos encontrado el valor, seguir buscándolo
            padre = actual;
            if(dat > actual->dato) actual = actual->derecho;
            else if(dat < actual->dato) actual = actual->izquierdo;
        }
    }
}

// Recorrido de árbol en inorden, aplicamos la función func, que tiene
// el prototipo:
// template<class DATO> void func(DATO&);
template<class DATO>
void AVL<DATO>::InOrden(Nodo<DATO> *nodo, bool r)
{
    if(r){
        nodo = raiz;
    }
    if(nodo->izquierdo){
        InOrden(nodo->izquierdo, false);
    }
    Tripleta * tri = (Tripleta *) nodo->dato;
    std::cout << std::to_string(tri->index) << std::endl;
    if(nodo->derecho){
        InOrden(nodo->derecho, false);
    }
}

// Recorrido de árbol en preorden, aplicamos la función func, que tiene
// el prototipo:
// template<class DATO> void func(DATO&);
template<class DATO>
void AVL<DATO>::PreOrden(Nodo<DATO> *nodo, bool r)
{
    if(r){
        nodo = raiz;
    }

    Tripleta * tri = (Tripleta *) nodo->dato;
    std::cout << "Preorden: " << std::to_string(tri->index) << std::endl;

    //func(nodo->dato, nodo->FE);
    if(nodo->izquierdo){
        PreOrden(nodo->izquierdo, false);
    }
    if(nodo->derecho){
        PreOrden(nodo->derecho, false);
    }
}

// Recorrido de árbol en postorden, aplicamos la función func, que tiene
// el prototipo:
// template<class DATO> void func(DATO&);
template<class DATO>
void AVL<DATO>::PostOrden(Nodo<DATO> *nodo, bool r)
{
    if(r){
        nodo = raiz;
    }
    if(nodo->izquierdo){
        PostOrden(nodo->izquierdo, false);
    }
    if(nodo->derecho){
        PostOrden(nodo->derecho, false);
    }
    Tripleta * tri = (Tripleta *) nodo->dato;
    std::cout << std::to_string(tri->index) << std::endl;
}

// Buscar un valor en el árbol
template<class DATO>
bool AVL<DATO>::Buscar(const DATO dat)
{
    actual = raiz;

    // Todavía puede aparecer, ya que quedan nodos por mirar
    while(!Vacio(actual)) {
        if(dat == actual->dato){
            return true; // dato encontrado
        }else if(dat > actual->dato){
            actual = actual->derecho; // Seguir
        }else if(dat < actual->dato){
            actual = actual->izquierdo;
        }
    }
    return false; // No está en árbol
}

// Calcular la altura del nodo que contiene el dato dat
template<class DATO>
qint32 AVL<DATO>::Altura(const DATO dat)
{
    qint32 altura = 0;
    actual = raiz;

    // Todavía puede aparecer, ya que quedan nodos por mirar
    while(!Vacio(actual)) {
        if(dat == actual->dato){
            return altura; // dato encontrado
        }else {
            altura++; // Incrementamos la altura, seguimos buscando
            if(dat > actual->dato){
                actual = actual->derecho;
            }else if(dat < actual->dato){
                actual = actual->izquierdo;
            }
        }
    }
    return -1; // No está en árbol
}

// Contar el número de nodos
template<class DATO>
qint32 AVL<DATO>::NumeroNodos()
{
    contador = 0;

    auxContador(raiz); // FUnción auxiliar
    return contador;
}

// Función auxiliar para contar nodos. Función recursiva de recorrido en
//   preorden, el proceso es aumentar el contador
template<class DATO>
void AVL<DATO>::auxContador(Nodo<DATO> *nodo)
{
    contador++;  // Otro nodo
    // Continuar recorrido
    if(nodo->izquierdo){
        auxContador(nodo->izquierdo);
    }
    if(nodo->derecho){
        auxContador(nodo->derecho);
    }
}

// Calcular la altura del árbol, que es la altura del nodo de mayor altura.
template<class DATO>
qint32 AVL<DATO>::AlturaArbol()
{
    altura = 0;

    if (!Vacio(raiz)){
        altura++;
    }

    auxAltura(raiz, altura); // Función auxiliar
    return altura;
}

// Función auxiliar para calcular altura. Función recursiva de recorrido en
// postorden, el proceso es actualizar la altura sólo en nodos hojas de mayor
// altura de la máxima actual
template<class DATO>
void AVL<DATO>::auxAltura(Nodo<DATO> *nodo, qint32 a)
{
    // Recorrido postorden
    if(nodo->izquierdo){
        auxAltura(nodo->izquierdo, a+1);
    }
    if(nodo->derecho){
        auxAltura(nodo->derecho, a+1);
    }
    // Proceso, si es un nodo hoja, y su altura es mayor que la actual del
    // árbol, actualizamos la altura actual del árbol
    if(EsHoja(nodo) && a > altura){
        altura = a;
    }
}
template<class DATO>
DATO AVL<DATO>::get_menor_tripleta(){

    if (AlturaArbol() == 0){
        return NULL;
    }
    if (AlturaArbol() == 1){
        return raiz->dato;
    }

    Nodo <DATO> * nodo_actual = raiz;
    Nodo <DATO> * nodo_menor = raiz;

    while (true){

        if (EsHoja(nodo_actual)){
            return nodo_menor->dato;
        } else {
            nodo_actual = nodo_actual->izquierdo;
            if (nodo_actual){
                if (nodo_actual->dato < nodo_menor->dato){
                    nodo_menor = nodo_actual;
                }
            } else {
                return nodo_menor->dato;
            }
        }
    }
}

#endif // AVL_H


