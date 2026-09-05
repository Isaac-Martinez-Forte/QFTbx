// The expression tree as the toolbox's one expression engine: the grammar a
// user writes plants and numbers in, the complex evaluation the templates run
// on, the in-memory construction the constraint propagation builds on, and
// the names it owns.
#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/core/math/expression_tree.h"
#include "src/core/math/constants.h"
#include "src/core/system/free_form.h"
#include "src/core/system/parameter.h"
#include "src/core/common/exception.h"

using namespace qftbx;

namespace {

using Complex = std::complex<double>;

double real(const char * text)
{
    ExpressionTree tree{std::string(text)};
    tree.bind({});
    return tree.evaluate(std::vector<double>());
}

} // namespace

TEST(ExpressionGrammar, BlanksUnaryMinusAndDecimalPointsAreRead)
{
    EXPECT_DOUBLE_EQ(real(" 2 * 3 + 1 "), 7.0);
    EXPECT_DOUBLE_EQ(real("2*-3"), -6.0);
    EXPECT_DOUBLE_EQ(real("2^-1"), 0.5);
    EXPECT_DOUBLE_EQ(real("-2^2"), -4.0);
    EXPECT_DOUBLE_EQ(real(".5+.25"), 0.75);
    EXPECT_DOUBLE_EQ(real("1e3/4"), 250.0);
    EXPECT_DOUBLE_EQ(real("(1+2)*(3+4)"), 21.0);
}

TEST(ExpressionGrammar, ThePowerBindsToTheRight)
{
    //2^3^2 is 2^(3^2) = 512, as in every algebra system, not (2^3)^2 = 64.
    EXPECT_DOUBLE_EQ(real("2^3^2"), 512.0);
    EXPECT_DOUBLE_EQ(real("2*3^2"), 18.0);
}

TEST(ExpressionGrammar, ConstantsInEitherCaseAndTheFunctions)
{
    EXPECT_DOUBLE_EQ(real("pi"), qftbx::math::kPi);
    EXPECT_DOUBLE_EQ(real("PI"), qftbx::math::kPi);
    EXPECT_DOUBLE_EQ(real("e"), qftbx::math::kE);
    EXPECT_DOUBLE_EQ(real("E"), qftbx::math::kE);
    EXPECT_NEAR(real("tanh(1)"), std::tanh(1.0), 1e-15);
    EXPECT_NEAR(real("log(e)"), 1.0, 1e-15);
    EXPECT_NEAR(real("log10(1000)"), 3.0, 1e-15);
    EXPECT_NEAR(real("lg(100)"), 2.0, 1e-15);
    EXPECT_NEAR(real("log2(8)"), 3.0, 1e-15);
    EXPECT_NEAR(real("ln(1)"), 0.0, 1e-15);
    EXPECT_NEAR(real("exp(0)+sqrt(16)+abs(-2)"), 7.0, 1e-15);
}

TEST(ExpressionGrammar, ANameThatStartsLikeAConstantIsAVariable)
{
    //"ev" is a parameter of the ACC'90 plant; "e" alone is the constant.
    ExpressionTree tree(std::string("ev*2+e"));
    tree.bind({"ev"});
    EXPECT_NEAR(tree.evaluate(std::vector<double>{3.0}), 6.0 + qftbx::math::kE, 1e-15);

    ExpressionTree pi1(std::string("P1+pi"));
    pi1.bind({"P1"});
    EXPECT_NEAR(pi1.evaluate(std::vector<double>{1.0}), 1.0 + qftbx::math::kPi, 1e-15);
}

TEST(ExpressionGrammar, MalformedTextIsRefusedWithAMessage)
{
    EXPECT_THROW(ExpressionTree(std::string("2*(3+")), std::invalid_argument);
    EXPECT_THROW(ExpressionTree(std::string("2*")), std::invalid_argument);
    EXPECT_THROW(ExpressionTree(std::string("2 3")), std::invalid_argument);
    EXPECT_THROW(ExpressionTree(std::string("a b")), std::invalid_argument);
    EXPECT_NO_THROW(ExpressionTree(std::string("sin (2) + 3")));
    EXPECT_THROW(ExpressionTree(std::string("2 $ 3")), std::invalid_argument);
    EXPECT_THROW(ExpressionTree(std::string("")), std::invalid_argument);
}

TEST(ExpressionBinding, TheValuesFollowTheOrderOfTheNames)
{
    ExpressionTree tree(std::string("a*10+b"));

    tree.bind({"b", "a"});
    EXPECT_DOUBLE_EQ(tree.evaluate(std::vector<double>{1.0, 2.0}), 21.0);

    tree.bind({"a", "b", "unused"});
    EXPECT_DOUBLE_EQ(tree.evaluate(std::vector<double>{2.0, 1.0, 99.0}), 21.0);

    EXPECT_THROW(tree.bind({"a"}), std::invalid_argument) << "b has no value";

    std::vector<std::string> names = tree.variableNames();
    ASSERT_EQ(names.size(), 2u);
    EXPECT_EQ(names[0], "a");
    EXPECT_EQ(names[1], "b");
}

TEST(ExpressionBinding, AnUnboundTreeRefusesToEvaluate)
{
    ExpressionTree tree(std::string("x+1"));
    EXPECT_THROW(tree.evaluate(std::vector<double>{1.0}), std::invalid_argument);
}

TEST(ComplexEvaluation, ThePlantOfTheAcc90BenchmarkAtJOmega)
{
    //ev / (s^2 (s^2 + 0.02 s + 2 ev)) at s = j w, by hand and by the tree.
    ExpressionTree tree(std::string("(ev)/(s^2*(s^2 + 0.02*s + 2*ev))"));
    tree.bind({"s", "ev"});

    const double w = 0.7;
    const double ev = 1.3;
    const Complex s(0.0, w);
    const Complex expected = ev / (s * s * (s * s + 0.02 * s + 2.0 * ev));
    const Complex actual = tree.evaluate(std::vector<Complex>{s, Complex(ev, 0.0)});

    EXPECT_NEAR(actual.real(), expected.real(), 1e-15 * std::abs(expected));
    EXPECT_NEAR(actual.imag(), expected.imag(), 1e-15 * std::abs(expected));
}

TEST(ComplexEvaluation, AWholePowerOfJOmegaIsExact)
{
    //(j w)^2 is exactly -w^2 with no imaginary residue: the expression
    //library this replaced went through the logarithm and left 1e-16 j,
    //which kept an undamped resonance from ever hitting its exact zero.
    ExpressionTree tree(std::string("s^2"));
    tree.bind({"s"});
    const Complex value = tree.evaluate(std::vector<Complex>{Complex(0.0, 3.0)});
    EXPECT_EQ(value.real(), -9.0);
    EXPECT_EQ(value.imag(), 0.0);
}

TEST(ComplexEvaluation, TheFunctionsTakeComplexArguments)
{
    ExpressionTree tree(std::string("exp(s)"));
    tree.bind({"s"});
    const Complex value = tree.evaluate(std::vector<Complex>{Complex(0.0, qftbx::math::kPi)});
    EXPECT_NEAR(value.real(), -1.0, 1e-15);
    EXPECT_NEAR(value.imag(), 0.0, 1e-15);

    ExpressionTree magnitude(std::string("abs(s)"));
    magnitude.bind({"s"});
    EXPECT_DOUBLE_EQ(magnitude.evaluate(std::vector<Complex>{Complex(3.0, 4.0)}).real(), 5.0);
}

TEST(ExpressionBuilder, ATreeBuiltInMemoryEvaluatesLikeItsText)
{
    const Expression k = Expression::variable("k");
    const Expression z = Expression::variable("z");
    const Expression built = k * sqrt(pow(z, Expression(2.0)) + Expression(4.0)) - Expression(1.0);

    ExpressionTree fromBuilder(built);
    ExpressionTree fromText(std::string("k*sqrt(z^2+4)-1"));

    std::map<std::string, double> variables{{"k", 2.0}, {"z", 1.5}};
    EXPECT_DOUBLE_EQ(fromBuilder.eval(&variables), fromText.eval(&variables));
    EXPECT_DOUBLE_EQ(fromBuilder.eval(&variables), 2.0 * 2.5 - 1.0);
}

TEST(ExpressionBuilder, AnExpressionIsABuildingBlockOfManyTrees)
{
    //Two constraints over the same g: each tree owns its own nodes, so the
    //propagation of one never touches the other.
    const Expression g = Expression::variable("g");
    ExpressionTree first(g - Expression(2.0), 0.0, qftbx::GREATER_EQUAL);
    ExpressionTree second(Expression(5.0) - g, 0.0, qftbx::GREATER_EQUAL);

    std::map<std::string, Interval> domains{{"g", Interval(0.0, 10.0)}};
    EXPECT_TRUE(first.propagate(&domains));
    EXPECT_TRUE(second.propagate(&domains));
    EXPECT_DOUBLE_EQ(domains.at("g").lower(), 2.0);
    EXPECT_DOUBLE_EQ(domains.at("g").upper(), 5.0);
}

TEST(ExpressionBuilder, AnOperatorWithoutAnOperandIsRefused)
{
    const Expression empty;
    EXPECT_THROW(empty + Expression(1.0), std::invalid_argument);
    EXPECT_THROW((ExpressionTree{empty}), std::invalid_argument);
}

TEST(ExpressionNames, TheGrammarOwnsItsFunctionsConstantsAndTheLaplaceVariable)
{
    for (const char * reserved : {"sin", "cos", "sqrt", "log", "log10", "log2", "ln", "lg",
                                  "exp", "abs", "tanh", "pi", "PI", "e", "E", "s"}) {
        EXPECT_TRUE(ExpressionTree::isReservedName(reserved)) << reserved;
        EXPECT_FALSE(ExpressionTree::isUsableVariableName(reserved)) << reserved;
    }

    //The letters the expression library used to own as unit multipliers
    //are ordinary names now, "k" first among them.
    for (const char * usable : {"k", "kv", "kc", "n", "u", "m", "M", "G", "z1", "p_1", "ev", "alpha"}) {
        EXPECT_TRUE(ExpressionTree::isUsableVariableName(usable)) << usable;
    }

    for (const char * notAnIdentifier : {"", "1k", "a-b", "a b", "_x"}) {
        EXPECT_FALSE(ExpressionTree::isUsableVariableName(notAnIdentifier)) << notAnIdentifier;
    }
}

TEST(FreeFormPlant, EvaluatesItsExpressionAtJOmegaWithTheParametersBound)
{
    std::vector<Parameter> numerator{Parameter(std::string("ev"), Range(0.5, 2.0), 0.5)};
    std::vector<Parameter> denominator{Parameter(std::string("ev"), Range(0.5, 2.0), 0.5)};
    FreeForm plant(std::string("acc90"), numerator, denominator, Parameter(1.0), Parameter(0.0),
                   std::string("ev"), std::string("s^2*(s^2 + 0.02*s + 2*ev)"));

    const double w = 0.7;
    const Complex s(0.0, w);
    const double ev = 1.3;
    const Complex expected = ev / (s * s * (s * s + 0.02 * s + 2.0 * ev));

    const Complex actual = plant.valueAt(w, {ev}, {ev}, 1.0, 0.0);
    EXPECT_NEAR(actual.real(), expected.real(), 1e-14 * std::abs(expected));
    EXPECT_NEAR(actual.imag(), expected.imag(), 1e-14 * std::abs(expected));

    //The same name in the numerator and the denominator is one variable.
    EXPECT_THROW(plant.valueAt(w, {1.0}, {2.0}, 1.0, 0.0), qftbx::InvalidInput);
}

TEST(FreeFormPlant, RefusesAnExpressionItCannotReadOrAParameterItDoesNotDeclare)
{
    std::vector<Parameter> none;
    EXPECT_THROW(FreeForm(std::string("bad"), none, none, Parameter(1.0), Parameter(0.0),
                          std::string("1"), std::string("s*(s+")),
                 qftbx::InvalidInput);
    EXPECT_THROW(FreeForm(std::string("undeclared"), none, none, Parameter(1.0), Parameter(0.0),
                          std::string("a"), std::string("s+1")),
                 qftbx::InvalidInput);

    std::vector<Parameter> laplace{Parameter(std::string("s"), Range(1.0, 2.0), 1.5)};
    EXPECT_THROW(FreeForm(std::string("laplace"), laplace, none, Parameter(1.0), Parameter(0.0),
                          std::string("s"), std::string("s+1")),
                 qftbx::InvalidInput);
}

TEST(ParameterReparametrisation, IsParsedOnceAndAppliedToNominalAndRange)
{
    Parameter var(std::string("a"), Range(1.0, 5.0), 3.0, std::string("a*2+1"));
    EXPECT_DOUBLE_EQ(var.nominal(), 7.0);
    EXPECT_DOUBLE_EQ(var.range().min, 3.0);
    EXPECT_DOUBLE_EQ(var.range().max, 11.0);
    EXPECT_DOUBLE_EQ(var.rawNominal(), 3.0);

    const Parameter copy = var;
    EXPECT_DOUBLE_EQ(copy.nominal(), 7.0);

    //An expression the grammar cannot read is refused where the parameter
    //is made, not where it is first evaluated.
    EXPECT_THROW(Parameter(std::string("a"), Range(1.0, 5.0), 3.0, std::string("a*(")), qftbx::InvalidInput);
}
