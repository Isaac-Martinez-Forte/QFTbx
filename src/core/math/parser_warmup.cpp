#include "src/core/math/parser_warmup.h"

#include "mpParser.h"

namespace qftbx {
namespace math {

void warmUpExpressionParser()
{
    //Constructing one parser with the full package set is what initialises
    //the six singletons; the object itself is not needed afterwards. The
    //package flags must be the widest the program uses (pckALL_COMPLEX), or
    //a package a later parser asks for would still be initialised in
    //parallel.
    const mup::ParserX warmUp(mup::pckALL_COMPLEX);
    (void) warmUp;
}

} // namespace math
} // namespace qftbx
