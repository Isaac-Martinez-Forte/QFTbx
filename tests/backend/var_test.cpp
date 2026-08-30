// Characterisation tests for Var (Modelo/EstructurasDatos/var.h): they pin
// the CURRENT behaviour of the class before its modernisation. Behaviours
// marked with "// BUG:" are known defects kept as-is on purpose; fixing them
// must flip the expectation in a dedicated commit.

#include <gtest/gtest.h>

#include <QPointF>
#include <QString>

#include "Modelo/EstructurasDatos/var.h"

namespace {

TEST(Var, UncertainVariableBasics)
{
    Var var(QStringLiteral("a"), QPointF(1.0, 5.0), 2.0);

    EXPECT_TRUE(var.isVariable());
    EXPECT_EQ(var.getNombre(), QStringLiteral("a"));
    EXPECT_DOUBLE_EQ(var.getN(), 2.0);
    EXPECT_EQ(var.getR(), QPointF(1.0, 5.0));
    // Without an explicit reparametrisation, exp falls back to the name.
    EXPECT_EQ(var.getExp(), QStringLiteral("a"));
    EXPECT_DOUBLE_EQ(var.getNominal(), 2.0);
    EXPECT_EQ(var.getRango(), QPointF(1.0, 5.0));
}

TEST(Var, InvertedRangeIsNormalised)
{
    Var var(QStringLiteral("a"), QPointF(5.0, 1.0), 2.0);

    EXPECT_EQ(var.getR(), QPointF(1.0, 5.0));
}

TEST(Var, ConstantValue)
{
    Var var(3.5);

    EXPECT_FALSE(var.isVariable());
    EXPECT_DOUBLE_EQ(var.getNominal(), 3.5);
    // Constants take their textual value as name and exp.
    EXPECT_EQ(var.getNombre(), QStringLiteral("3.5"));
    EXPECT_EQ(var.getExp(), QStringLiteral("3.5"));
}

TEST(Var, NamedConstant)
{
    Var var(QStringLiteral("kv"), 1.0);

    EXPECT_FALSE(var.isVariable());
    EXPECT_EQ(var.getNombre(), QStringLiteral("kv"));
    EXPECT_DOUBLE_EQ(var.getNominal(), 1.0);
    EXPECT_EQ(var.getExp(), QStringLiteral("kv"));
}

TEST(Var, ReparametrisationThroughExp)
{
    // exp is evaluated with muParserX substituting the raw value of the
    // variable: nominal and range are transformed, raw accessors are not.
    Var var(QStringLiteral("a"), QPointF(1.0, 5.0), 3.0, QStringLiteral("a*2"));

    EXPECT_TRUE(var.isVariable());
    EXPECT_DOUBLE_EQ(var.getN(), 3.0);
    EXPECT_DOUBLE_EQ(var.getNominal(), 6.0);
    EXPECT_EQ(var.getR(), QPointF(1.0, 5.0));
    EXPECT_EQ(var.getRango(), QPointF(2.0, 10.0));
}

TEST(Var, IdentityExpBehavesAsNoReparametrisation)
{
    // The XML writer always stores exp == name; values must pass through.
    Var var(QStringLiteral("a"), QPointF(0.5, 2.0), 2.0, QStringLiteral("a"));

    EXPECT_DOUBLE_EQ(var.getNominal(), 2.0);
    EXPECT_EQ(var.getRango(), QPointF(0.5, 2.0));
}

TEST(Var, EmptyExpLeavesExpEmpty)
{
    // BUG: the 4-argument constructor with an empty exp does not fall back
    // to the name (unlike the 3-argument one); getExp() returns "".
    // FormatoLibre::getExpr() would then emit "*(...)": a parse error.
    Var var(QStringLiteral("a"), QPointF(1.0, 5.0), 2.0, QString());

    EXPECT_EQ(var.getExp(), QString());
}

TEST(Var, CloneUncertainVariablePreservesContent)
{
    Var var(QStringLiteral("a"), QPointF(1.0, 5.0), 2.0, QStringLiteral("a"));

    Var* copy = var.clone();
    ASSERT_NE(copy, nullptr);
    EXPECT_NE(copy, &var);
    EXPECT_TRUE(copy->isVariable());
    EXPECT_EQ(copy->getNombre(), QStringLiteral("a"));
    EXPECT_DOUBLE_EQ(copy->getN(), 2.0);
    EXPECT_EQ(copy->getR(), QPointF(1.0, 5.0));
    EXPECT_EQ(copy->getExp(), QStringLiteral("a"));
    delete copy;
}

TEST(Var, CloneConstantLosesItsName)
{
    // BUG: clone() routes constants through Var(qreal), which rewrites the
    // name as the textual value: a named constant like "kv" becomes "1".
    Var var(QStringLiteral("kv"), 1.0);

    Var* copy = var.clone();
    ASSERT_NE(copy, nullptr);
    EXPECT_FALSE(copy->isVariable());
    EXPECT_DOUBLE_EQ(copy->getNominal(), 1.0);
    EXPECT_EQ(copy->getNombre(), QStringLiteral("1"));
    delete copy;
}

} // namespace
