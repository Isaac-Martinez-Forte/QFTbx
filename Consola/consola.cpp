#include "consola.h"

consola::consola()
{

}

consola::~consola()
{

}


void consola::mostrar(){
   QProcess::execute("./literm/bin/literm -e ./muparserx/bin/example");
}
