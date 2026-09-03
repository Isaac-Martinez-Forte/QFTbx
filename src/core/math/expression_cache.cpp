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
    explicit CachedParser(const QString & expression, const std::vector<QString> & names)
        : parser(mup::pckALL_COMPLEX), values(static_cast<std::size_t>(names.size()))
    {
        for (std::int32_t i = 0; i < static_cast<std::int32_t>(names.size()); i++) {
            parser.DefineVar(names.at(i).toStdString(),
                             mup::Variable(&values[static_cast<std::size_t>(i)]));
        }

        //Parsed once, right here. Every Eval() after the first walks the RPN.
        parser.SetExpr(expression.toStdString());
    }

    mup::ParserX parser;
    std::vector<mup::Value> values;
};

//Keyed by the expression and its variable names: two expressions that differ
//only in which names they bind are different parsers.
using Key = std::pair<QString, std::vector<QString>>;

//Deliberately a pointer that is never deleted. A thread_local object holding
//muParserX state would be destroyed at thread exit, and the order against the
//library's own static packages is not something to depend on.
std::map<Key, std::unique_ptr<CachedParser>> & cache()
{
    static thread_local auto * parsers = new std::map<Key, std::unique_ptr<CachedParser>>();
    return *parsers;
}

} // namespace

std::complex<double> evaluateCached(const QString & expression,
                                    const std::vector<QString> & names,
                                    const std::vector<std::complex<double>> & values)
{
    if (static_cast<std::size_t>(names.size()) != values.size()) {
        throw InvalidInput("evaluateCached: one value per variable is required");
    }

    auto & parsers = cache();
    const Key key(expression, names);

    auto found = parsers.find(key);
    if (found == parsers.end()) {
        found = parsers.emplace(key, std::make_unique<CachedParser>(expression, names)).first;
    }

    CachedParser & cached = *found->second;

    for (std::size_t i = 0; i < values.size(); i++) {
        cached.values[i] = mup::Value(values[i]);
    }

    return cached.parser.Eval().GetComplex();
}

bool isReservedName(const QString & name)
{
    if (name.isEmpty()) {
        return false;
    }

    //The same package set every parser in the toolbox is built with, so the
    //answer matches what a real binding would do.
    static thread_local mup::ParserX probe(mup::pckALL_COMPLEX);

    const std::string identifier = name.toStdString();

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
