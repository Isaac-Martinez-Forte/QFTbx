#include <string>
#include <vector>
#include <cstdint>
#include "src/core/math/expression_cache.h"

#include <map>
#include <memory>

#include "mpParser.h"

#include "src/core/exception.h"

namespace qftbx {
namespace math {

namespace {

//One parsed parser plus the values its variables are bound to. The values
//live HERE and not in the caller: mup::Variable stores a POINTER to a
//mup::Value, so the value has to outlive the binding, and the vector is sized
//once and never grown again - a reallocation would dangle every binding made
//before it.
struct CachedParser
{
    explicit CachedParser(const std::string & expression, const std::vector<std::string> & names)
        : parser(mup::pckALL_COMPLEX), values(static_cast<std::size_t>(names.size()))
    {
        for (std::size_t i = 0; i < names.size(); ++i) {
            parser.DefineVar(names.at(i),
                             mup::Variable(&values[i]));
        }

        //Parsed once, right here. Every Eval() after the first walks the RPN.
        parser.SetExpr(expression);
    }

    mup::ParserX parser;
    std::vector<mup::Value> values;
};

//Keyed by the expression and its variable names: two expressions that differ
//only in which names they bind are different parsers.
using Key = std::pair<std::string, std::vector<std::string>>;

//Deliberately a pointer that is never deleted. A thread_local object holding
//muParserX state would be destroyed at thread exit, and the order against the
//library's own static packages is not something to depend on.
std::map<Key, std::unique_ptr<CachedParser>> & cache()
{
    static thread_local auto * parsers = new std::map<Key, std::unique_ptr<CachedParser>>();
    return *parsers;
}

} // namespace

std::complex<double> evaluateCached(const std::string & expression,
                                    const std::vector<std::string> & names,
                                    const std::vector<std::complex<double>> & values)
{
    if (static_cast<std::size_t>(names.size()) != values.size()) {
        throw InvalidInput("evaluateCached: one value per variable is required");
    }

    //A one-entry memo in front of the map, because the map lookup itself was
    //on the hot path: building the Key copies the expression and every name,
    //and a template sweep evaluates the same plant thousands of times per
    //frequency. QString made that copy a refcount bump and hid the cost;
    //std::string makes it a real allocation, which showed up as a 20% loss
    //on the template and boundary tests the moment the strings changed.
    //
    //The pointers are compared, not the strings: a hit needs the caller to
    //hand over the very same objects, which is what the callers do - a
    //Parameter or a FreeForm evaluating itself over and over.
    static thread_local const std::string * lastExpression = nullptr;
    static thread_local const std::vector<std::string> * lastNames = nullptr;
    static thread_local CachedParser * lastParser = nullptr;

    CachedParser * cached = nullptr;

    if (lastParser != nullptr && lastExpression == &expression && lastNames == &names) {
        cached = lastParser;
    } else {
        auto & parsers = cache();
        const Key key(expression, names);

        auto found = parsers.find(key);
        if (found == parsers.end()) {
            found = parsers.emplace(key, std::make_unique<CachedParser>(expression, names)).first;
        }

        cached = found->second.get();
        lastExpression = &expression;
        lastNames = &names;
        lastParser = cached;
    }

    for (std::size_t i = 0; i < values.size(); i++) {
        cached->values[i] = mup::Value(values[i]);
    }

    return cached->parser.Eval().GetComplex();
}

bool isReservedName(const std::string & name)
{
    if (name.empty()) {
        return false;
    }

    //The same package set every parser in the toolbox is built with, so the
    //answer matches what a real binding would do.
    static thread_local mup::ParserX probe(mup::pckALL_COMPLEX);

    const std::string identifier = name;

    return probe.IsVarDefined(identifier)
            || probe.IsConstDefined(identifier)
            || probe.IsFunDefined(identifier)
            || probe.IsOprtDefined(identifier)
            || probe.IsPostfixOprtDefined(identifier)
            || probe.IsInfixOprtDefined(identifier);
}

int cachedExpressionCount()
{
    return static_cast<int>(cache().size());
}

} // namespace math
} // namespace qftbx
