// Characterisation tests for Parameter (src/core/system/parameter.h): they pin
// the CURRENT behaviour of the class before its modernisation. Behaviours
// marked with "// BUG:" are known defects kept as-is on purpose; fixing them
// must flip the expectation in a dedicated commit.

#include <gtest/gtest.h>

#include <QPointF>
#include <QString>

#include "src/core/system/parameter.h"

namespace {

TEST(Parameter, UncertainVariableBasics)
{
    Parameter var(QStringLiteral("a"), QPointF(1.0, 5.0), 2.0);

    EXPECT_TRUE(var.isUncertain());
    EXPECT_EQ(var.name(), QStringLiteral("a"));
    EXPECT_DOUBLE_EQ(var.rawNominal(), 2.0);
    EXPECT_EQ(var.rawRange(), QPointF(1.0, 5.0));
    // Without an explicit reparametrisation, exp falls back to the name.
    EXPECT_EQ(var.expression(), QStringLiteral("a"));
    EXPECT_DOUBLE_EQ(var.nominal(), 2.0);
    EXPECT_EQ(var.range(), QPointF(1.0, 5.0));
}

TEST(Parameter, InvertedRangeIsNormalised)
{
    Parameter var(QStringLiteral("a"), QPointF(5.0, 1.0), 2.0);

    EXPECT_EQ(var.rawRange(), QPointF(1.0, 5.0));
}

TEST(Parameter, ConstantValue)
{
    Parameter var(3.5);

    EXPECT_FALSE(var.isUncertain());
    EXPECT_DOUBLE_EQ(var.nominal(), 3.5);
    // Constants take their textual value as name and exp.
    EXPECT_EQ(var.name(), QStringLiteral("3.5"));
    EXPECT_EQ(var.expression(), QStringLiteral("3.5"));
}

TEST(Parameter, NamedConstant)
{
    Parameter var(QStringLiteral("kv"), 1.0);

    EXPECT_FALSE(var.isUncertain());
    EXPECT_EQ(var.name(), QStringLiteral("kv"));
    EXPECT_DOUBLE_EQ(var.nominal(), 1.0);
    EXPECT_EQ(var.expression(), QStringLiteral("kv"));
}

TEST(Parameter, ReparametrisationThroughExp)
{
    // exp is evaluated with muParserX substituting the raw value of the
    // variable: nominal and range are transformed, raw accessors are not.
    Parameter var(QStringLiteral("a"), QPointF(1.0, 5.0), 3.0, QStringLiteral("a*2"));

    EXPECT_TRUE(var.isUncertain());
    EXPECT_DOUBLE_EQ(var.rawNominal(), 3.0);
    EXPECT_DOUBLE_EQ(var.nominal(), 6.0);
    EXPECT_EQ(var.rawRange(), QPointF(1.0, 5.0));
    EXPECT_EQ(var.range(), QPointF(2.0, 10.0));
}

TEST(Parameter, IdentityExpBehavesAsNoReparametrisation)
{
    // The XML writer always stores exp == name; values must pass through.
    Parameter var(QStringLiteral("a"), QPointF(0.5, 2.0), 2.0, QStringLiteral("a"));

    EXPECT_DOUBLE_EQ(var.nominal(), 2.0);
    EXPECT_EQ(var.range(), QPointF(0.5, 2.0));
}

TEST(Parameter, EmptyExpFallsBackToName)
{
    // Fixed: the 4-argument constructor with an empty exp now falls back to
    // the name, like the 3-argument one (it used to leave exp empty, and
    // FreeForm::expression() emitted "*(...)": a parse error).
    Parameter var(QStringLiteral("a"), QPointF(1.0, 5.0), 2.0, QString());

    EXPECT_EQ(var.expression(), QStringLiteral("a"));
    EXPECT_DOUBLE_EQ(var.nominal(), 2.0);
}

TEST(Parameter, CopyConstructorCopiesEverything)
{
    // Fixed: the copy constructor used to copy only the range, leaving
    // name, nominal and the `variable` flag uninitialised.
    Parameter original(QStringLiteral("a"), QPointF(1.0, 5.0), 2.0, QStringLiteral("a*2"));
    Parameter copia(original);

    EXPECT_TRUE(copia.isUncertain());
    EXPECT_EQ(copia.name(), QStringLiteral("a"));
    EXPECT_DOUBLE_EQ(copia.rawNominal(), 2.0);
    EXPECT_EQ(copia.rawRange(), QPointF(1.0, 5.0));
    EXPECT_EQ(copia.expression(), QStringLiteral("a*2"));
    EXPECT_DOUBLE_EQ(copia.nominal(), 4.0);
}

TEST(Parameter, CloneUncertainVariablePreservesContent)
{
    Parameter var(QStringLiteral("a"), QPointF(1.0, 5.0), 2.0, QStringLiteral("a"));

    Parameter* copy = var.clone();
    ASSERT_NE(copy, nullptr);
    EXPECT_NE(copy, &var);
    EXPECT_TRUE(copy->isUncertain());
    EXPECT_EQ(copy->name(), QStringLiteral("a"));
    EXPECT_DOUBLE_EQ(copy->rawNominal(), 2.0);
    EXPECT_EQ(copy->rawRange(), QPointF(1.0, 5.0));
    EXPECT_EQ(copy->expression(), QStringLiteral("a"));
    delete copy;
}

TEST(Parameter, CloneConstantKeepsItsName)
{
    // Fixed: clone() used to route constants through Parameter(qreal), which
    // rewrote the name as the textual value ("kv" became "1").
    Parameter var(QStringLiteral("kv"), 1.0);

    Parameter* copy = var.clone();
    ASSERT_NE(copy, nullptr);
    EXPECT_FALSE(copy->isUncertain());
    EXPECT_DOUBLE_EQ(copy->nominal(), 1.0);
    EXPECT_EQ(copy->name(), QStringLiteral("kv"));
    delete copy;
}

TEST(Parameter, ClonarVectorMakesDeepCopies)
{
    QVector<Parameter*> origen;
    origen.append(new Parameter(QStringLiteral("a"), QPointF(1.0, 5.0), 2.0));
    origen.append(new Parameter(3.5));

    QVector<Parameter*>* copia = Parameter::cloneVector(&origen);
    ASSERT_NE(copia, nullptr);
    ASSERT_EQ(copia->size(), 2);
    EXPECT_NE(copia->at(0), origen.at(0));
    EXPECT_NE(copia->at(1), origen.at(1));
    EXPECT_EQ(copia->at(0)->name(), QStringLiteral("a"));
    EXPECT_DOUBLE_EQ(copia->at(1)->nominal(), 3.5);

    qDeleteAll(*copia);
    delete copia;
    qDeleteAll(origen);
}

} // namespace
