#ifndef NODO
#define NODO

#include <QtCore>
// Clase Nodo de Arbol AVL:
template<class DATO>
class Nodo {
public:
    // Constructor:
    Nodo(const DATO dat, Nodo<DATO> *pad=NULL, Nodo<DATO> *izq=NULL, Nodo<DATO> *der=NULL) {
        dato = dat;
        padre = pad;
        izquierdo = izq;
        derecho = der;
        FE = 0;
    }

    // Miembros:
    DATO dato;
    qint32 FE;
    Nodo<DATO> *izquierdo;
    Nodo<DATO> *derecho;
    Nodo<DATO> *padre;
};
#endif // NODO

