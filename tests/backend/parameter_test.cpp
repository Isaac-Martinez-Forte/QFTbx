/**
 * @file
 * @brief Tests of the parameter type and its reparametrisation.
 *
 * A constant takes its textual value as name and expression; an uncertain
 * variable keeps a raw range and nominal, and an optional expression in its
 * own name transforms the values it reports while the raw accessors stay
 * untouched. An empty expression falls back to the name, an inverted range is
 * normalised, and a copy carries every field, so a vector of parameters copies
 * member by member and mutating the copy leaves the source alone. A failure
 * here means a plant read from a file or handed to a sweep reports the wrong
 * value or the wrong name.
 */

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
    Parameter var(std::string("a"), Range(1.0, 5.0), 3.0, std::string("a*2"));

    EXPECT_TRUE(var.isUncertain());
    EXPECT_DOUBLE_EQ(var.rawNominal(), 3.0);
    EXPECT_DOUBLE_EQ(var.nominal(), 6.0);
    EXPECT_EQ(var.rawRange(), Range(1.0, 5.0));
    EXPECT_EQ(var.range(), Range(2.0, 10.0));
}

TEST(Parameter, IdentityExpBehavesAsNoReparametrisation)
{
    Parameter var(std::string("a"), Range(0.5, 2.0), 2.0, std::string("a"));

    EXPECT_DOUBLE_EQ(var.nominal(), 2.0);
    EXPECT_EQ(var.range(), Range(0.5, 2.0));
}

TEST(Parameter, EmptyExpFallsBackToName)
{
    Parameter var(std::string("a"), Range(1.0, 5.0), 2.0, std::string());

    EXPECT_EQ(var.expression(), std::string("a"));
    EXPECT_DOUBLE_EQ(var.nominal(), 2.0);
}

TEST(Parameter, CopyConstructorCopiesEverything)
{
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
    Parameter var(std::string("kv"), 1.0);

    Parameter copy = var;
    EXPECT_FALSE(copy.isUncertain());
    EXPECT_DOUBLE_EQ(copy.nominal(), 1.0);
    EXPECT_EQ(copy.name(), std::string("kv"));
}

TEST(Parameter, VectorCopyIsIndependent)
{
    std::vector<Parameter> source;
    source.emplace_back(std::string("a"), Range(1.0, 5.0), 2.0);
    source.emplace_back(3.5);

    std::vector<Parameter> copy = source;
    ASSERT_EQ(copy.size(), 2u);
    EXPECT_NE(&copy[0], &source[0]);
    EXPECT_NE(&copy[1], &source[1]);
    EXPECT_EQ(copy[0].name(), std::string("a"));
    EXPECT_DOUBLE_EQ(copy[1].nominal(), 3.5);

    copy[0].setName(std::string("b"));
    EXPECT_EQ(source[0].name(), std::string("a"));
}

}
