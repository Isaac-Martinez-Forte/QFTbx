// Tests for Parameter (src/core/system/parameter.h).

#include <gtest/gtest.h>

#include <string>

#include <vector>

#include "src/core/math/point.h"
#include "src/core/math/range.h"

#include "src/core/system/parameter.h"

using namespace qftbx;

namespace {

TEST(Parameter, UncertainVariableBasics)
{
    Parameter var(std::string("a"), Range(1.0, 5.0), 2.0);

    EXPECT_TRUE(var.isUncertain());
    EXPECT_EQ(var.name(), std::string("a"));
    EXPECT_DOUBLE_EQ(var.rawNominal(), 2.0);
    EXPECT_EQ(var.rawRange(), Range(1.0, 5.0));
    // Without an explicit reparametrisation, exp falls back to the name.
    EXPECT_EQ(var.expression(), std::string("a"));
    EXPECT_DOUBLE_EQ(var.nominal(), 2.0);
    EXPECT_EQ(var.range(), Range(1.0, 5.0));
}

TEST(Parameter, InvertedRangeIsNormalised)
{
    Parameter var(std::string("a"), Range(5.0, 1.0), 2.0);

    EXPECT_EQ(var.rawRange(), Range(1.0, 5.0));
}

TEST(Parameter, ConstantValue)
{
    Parameter var(3.5);

    EXPECT_FALSE(var.isUncertain());
    EXPECT_DOUBLE_EQ(var.nominal(), 3.5);
    // Constants take their textual value as name and exp.
    EXPECT_EQ(var.name(), std::string("3.5"));
    EXPECT_EQ(var.expression(), std::string("3.5"));
}

TEST(Parameter, NamedConstant)
{
    Parameter var(std::string("kv"), 1.0);

    EXPECT_FALSE(var.isUncertain());
    EXPECT_EQ(var.name(), std::string("kv"));
    EXPECT_DOUBLE_EQ(var.nominal(), 1.0);
    EXPECT_EQ(var.expression(), std::string("kv"));
}

TEST(Parameter, ReparametrisationThroughExp)
{
    // exp is evaluated with muParserX substituting the raw value of the
    // variable: nominal and range are transformed, raw accessors are not.
    Parameter var(std::string("a"), Range(1.0, 5.0), 3.0, std::string("a*2"));

    EXPECT_TRUE(var.isUncertain());
    EXPECT_DOUBLE_EQ(var.rawNominal(), 3.0);
    EXPECT_DOUBLE_EQ(var.nominal(), 6.0);
    EXPECT_EQ(var.rawRange(), Range(1.0, 5.0));
    EXPECT_EQ(var.range(), Range(2.0, 10.0));
}

TEST(Parameter, IdentityExpBehavesAsNoReparametrisation)
{
    // The XML writer always stores exp == name; values must pass through.
    Parameter var(std::string("a"), Range(0.5, 2.0), 2.0, std::string("a"));

    EXPECT_DOUBLE_EQ(var.nominal(), 2.0);
    EXPECT_EQ(var.range(), Range(0.5, 2.0));
}

TEST(Parameter, EmptyExpFallsBackToName)
{
    // Fixed: the 4-argument constructor with an empty exp now falls back to
    // the name, like the 3-argument one (it used to leave exp empty, and
    // FreeForm::expression() emitted "*(...)": a parse error).
    Parameter var(std::string("a"), Range(1.0, 5.0), 2.0, std::string());

    EXPECT_EQ(var.expression(), std::string("a"));
    EXPECT_DOUBLE_EQ(var.nominal(), 2.0);
}

TEST(Parameter, CopyConstructorCopiesEverything)
{
    // Fixed: the copy constructor used to copy only the range, leaving
    // name, nominal and the `variable` flag uninitialised.
    Parameter original(std::string("a"), Range(1.0, 5.0), 2.0, std::string("a*2"));
    Parameter copia(original);

    EXPECT_TRUE(copia.isUncertain());
    EXPECT_EQ(copia.name(), std::string("a"));
    EXPECT_DOUBLE_EQ(copia.rawNominal(), 2.0);
    EXPECT_EQ(copia.rawRange(), Range(1.0, 5.0));
    EXPECT_EQ(copia.expression(), std::string("a*2"));
    EXPECT_DOUBLE_EQ(copia.nominal(), 4.0);
}

TEST(Parameter, CopyOfUncertainVariablePreservesContent)
{
    // Value semantics replaced clone(): the copy constructor carries the
    // name, the raw range, the raw nominal and the reparametrisation.
    Parameter var(std::string("a"), Range(1.0, 5.0), 2.0, std::string("a"));

    Parameter copy = var;
    EXPECT_NE(&copy, &var);
    EXPECT_TRUE(copy.isUncertain());
    EXPECT_EQ(copy.name(), std::string("a"));
    EXPECT_DOUBLE_EQ(copy.rawNominal(), 2.0);
    EXPECT_EQ(copy.rawRange(), Range(1.0, 5.0));
    EXPECT_EQ(copy.expression(), std::string("a"));
}

TEST(Parameter, CopyOfConstantKeepsItsName)
{
    // The defect the old clone() had: it routed constants through
    // Parameter(double), rewriting the name as the textual value ("kv"
    // became "1"). A copy cannot lose the name.
    Parameter var(std::string("kv"), 1.0);

    Parameter copy = var;
    EXPECT_FALSE(copy.isUncertain());
    EXPECT_DOUBLE_EQ(copy.nominal(), 1.0);
    EXPECT_EQ(copy.name(), std::string("kv"));
}

TEST(Parameter, VectorCopyIsIndependent)
{
    // What cloneVector() used to do by hand, std::vector does by itself.
    std::vector<Parameter> source;
    source.emplace_back(std::string("a"), Range(1.0, 5.0), 2.0);
    source.emplace_back(3.5);

    std::vector<Parameter> copy = source;
    ASSERT_EQ(copy.size(), 2u);
    EXPECT_NE(&copy[0], &source[0]);
    EXPECT_NE(&copy[1], &source[1]);
    EXPECT_EQ(copy[0].name(), std::string("a"));
    EXPECT_DOUBLE_EQ(copy[1].nominal(), 3.5);

    //Mutating the copy leaves the source alone. Through setName(), which is
    //the one mutator left: setNominal() went with the other setters that
    //skipped the constructors' checks.
    copy[0].setName(std::string("b"));
    EXPECT_EQ(source[0].name(), std::string("a"));
}

} // namespace
