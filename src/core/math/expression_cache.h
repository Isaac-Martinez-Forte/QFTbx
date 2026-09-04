#ifndef QFTBX_EXPRESSION_CACHE_H
#define QFTBX_EXPRESSION_CACHE_H

#include <complex>
#include <vector>

#include <string>
#include <vector>

namespace qftbx {
namespace math {

/**
 * @brief Evaluates a user-written expression, parsing it ONCE per thread.
 *
 * muParserX is built for this: SetExpr() lexes the text and builds an RPN,
 * the first Eval() switches the engine to ParseFromRPN, and every Eval()
 * afterwards walks that RPN without parsing again (ReInit(), which SetExpr
 * calls, is what throws the RPN away). The toolbox was not using it that way:
 * it constructed a fresh parser and called SetExpr on every single
 * evaluation, so it paid a full lex, parse and RPN build each time - and, on
 * top of that, rebuilt the lookup tables of the six function packages.
 *
 * The cache is per THREAD on purpose. A parser is mutable state: one shared
 * between threads would be a data race, and muParserX has already given us
 * one of those (see warmUpExpressionParser).
 *
 * @param expression the text to evaluate.
 * @param names the variables it may use, which must be the same list on every
 * call for a given expression - the pair (expression, names) is the cache key.
 * @param values one value per name, in the same order. These are copied into
 * the parser's bound values, so they may change freely between calls.
 */
std::complex<double> evaluateCached(const std::string & expression,
                                    const std::vector<std::string> & names,
                                    const std::vector<std::complex<double>> & values);

/**
 * @brief Is @p name already taken by muParserX?
 *
 * A parameter cannot be named after anything the parser already knows: the
 * binding fails and the plant stops evaluating. The dialogs only ever checked
 * FUNCTION names, which misses the two kinds that actually bite:
 *
 *  - the constants "e", "pi" and "i". A parameter named "e" once let a
 *    muParserX error escape an OpenMP region, which ends the process; one
 *    named "i" would be worse, because the expressions use i as the imaginary
 *    unit and the numbers would come out wrong rather than refused.
 *  - the unit postfix operators "u" (1e-6), "m" (1e-3) and "k" (1e3). "k" is
 *    the most natural name in the world for a gain.
 *
 * Checks every category muParserX exposes, so a future package cannot open a
 * new hole silently.
 */
bool isReservedName(const std::string & name);

/// The number of parsers this thread has cached. For tests: it is what proves
/// that repeated evaluation of one expression parses only once.
int cachedExpressionCount();

/**
 * @brief Whether muParserX can bind a variable under this name.
 *
 * It refuses any name that collides with something it already defines, and
 * that includes six single letters nobody would suspect: n, u, m, k, M and G
 * are its SI unit postfix operators (kilo, mega, milli...). "k" for a gain is
 * the one that hurts, being the canonical name for one in control.
 *
 * It is asked rather than answered from a list, because a list would drift
 * from whatever muParserX is vendored - and its own enumeration API does not
 * expose the postfix operators, so the only authoritative test is to try.
 * The answer is memoised per thread, so the parser is built once per name.
 */
bool isUsableVariableName(const std::string & name);

} // namespace math
} // namespace qftbx

#endif // QFTBX_EXPRESSION_CACHE_H
