#include "polosceros.h"

using namespace std;

PolosCeros::PolosCeros(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador, Var * k, Var* ret) :
FuncionTransferencia(nombre, numerador, denominador, k, ret) {
}

PolosCeros::PolosCeros(PolosCeros *datos) : FuncionTransferencia(datos) {
}

PolosCeros::~PolosCeros() {
}
