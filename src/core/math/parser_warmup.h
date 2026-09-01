#ifndef QFTBX_PARSER_WARMUP_H
#define QFTBX_PARSER_WARMUP_H

namespace qftbx {
namespace math {

/**
 * @brief Initialises muParserX's package singletons on the calling thread.
 *
 * muParserX builds its function packages as lazy singletons WITHOUT
 * synchronisation (mpPackageUnit.cpp and its siblings all read
 * `s_pInstance.get()==nullptr` and then `s_pInstance.reset(new ...)`), and
 * every mup::ParserX constructor touches every package of its set. The
 * default set is pckALL_COMPLEX - common, complex, string, unit and matrix -
 * which is what the whole toolbox uses; the sixth package, non-complex, is
 * the one nothing here asks for. Two threads constructing a
 * parser at the same time therefore both see null, both construct, and one
 * reset() DELETES the object the other is about to use: a use-after-free on a
 * function table, whose symptom is a wrong or throwing evaluation once, on one
 * frequency, never reproducibly.
 *
 * This is not hypothetical here: ThreadSanitizer reports it on the template
 * sweep, which constructs one parser per frequency inside an OpenMP loop, and
 * TransferFunction::evaluate constructs one per call from inside the boundary
 * loops too.
 *
 * Call this ONCE, sequentially, before entering any parallel region that will
 * evaluate expressions. After it every thread only reads pointers that are
 * already non-null.
 */
void warmUpExpressionParser();

} // namespace math
} // namespace qftbx

#endif // QFTBX_PARSER_WARMUP_H
