/**
 * @file
 * @brief The postfix program of an expression gives what the tree gives, to
 * the bit.
 *
 * Every operation and function the grammar has, the small whole powers the
 * tree takes by squaring and the general power, and the free-form plants of
 * the examples, are evaluated over complex values by the tree and by its
 * program. In a build that does not fuse products and sums they must agree
 * to the last bit; in one that does, the compiler fuses the two
 * differently, and they must agree to a few units in the last place. The
 * program's depth is the stack it needs, and a variable left unbound is
 * refused when compiling.
 */

#include <gtest/gtest.h>

#include <complex>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/core/math/expression_program.h"
#include "src/core/math/expression_tree.h"

using namespace qftbx;
using Complex = std::complex<double>;

namespace {

bool sameBits(Complex a, Complex b)
{
    return std::memcmp(&a, &b, sizeof a) == 0;
}

bool agree(Complex a, Complex b)
{
#if QFTBX_MAY_FUSE_PRODUCTS
    return sameBits(a, b) || std::abs(a - b) <= 1e-15 * std::abs(a);
#else
    return sameBits(a, b);
#endif
}

void expectSameAsTree(const std::string & text, const std::vector<std::string> & names,
                      const std::vector<std::vector<Complex>> & points)
{
    ExpressionTree tree(text);
    tree.bind(names);
    const ExpressionProgram program = ExpressionProgram::compile(tree);
    for (const std::vector<Complex> & values : points) {
        const Complex fromTree = tree.evaluate(values);
        const Complex fromProgram = program.evaluate(values);
        EXPECT_TRUE(agree(fromTree, fromProgram))
                << text << " at s = " << values[0] << ": tree " << fromTree << ", program " << fromProgram;
    }
}

std::vector<std::vector<Complex>> pointsOf(std::size_t parameters)
{
    std::vector<std::vector<Complex>> points;
    for (const double w : {1e-3, 0.37, 1.0, 2.5, 41.0, 1e4}) {
        std::vector<Complex> values{Complex(0.0, w)};
        for (std::size_t p = 0; p < parameters; ++p) {
            values.push_back(Complex(0.3 + 0.7 * static_cast<double>(p) + 0.01 * w, 0.0));
        }
        points.push_back(values);
    }
    return points;
}

}

TEST(ExpressionProgram, EveryOperationGivesWhatTheTreeGives)
{
    const std::vector<std::string> texts = {
        "s+a", "s-a", "s*a", "s/a", "-s", "-(s+a)*2",
        "sin(s)", "cos(s)", "tan(s)", "atan(s)", "sinh(s)", "cosh(s)", "tanh(s)",
        "asin(s)", "acos(s)", "exp(s*a)", "abs(s+a)", "ln(s+a)", "log(s+a)", "log10(s+a)", "lg(s+a)",
        "log2(s+a)", "sqrt(s+a)", "pi*s+e", "PI/E",
    };
    for (const std::string & text : texts) {
        expectSameAsTree(text, {"s", "a"}, pointsOf(1));
    }
}

TEST(ExpressionProgram, PowersAreTakenAsTheTreeTakesThem)
{
    for (const char * text : {"s^2", "s^3", "s^-2", "s^0", "s^64", "s^65", "s^2.5", "s^a", "(s+a)^-7", "2^3^2"}) {
        expectSameAsTree(text, {"s", "a"}, pointsOf(1));
    }
}

TEST(ExpressionProgram, TheFreeFormPlantsOfTheExamplesGiveWhatTheTreeGives)
{
    const std::vector<std::pair<std::string, std::vector<std::string>>> plants = {
        {"(1+T1*s)/((1+T2*s)*(1+T3*s))", {"s", "T1", "T2", "T3"}},
        {"(a)/(s^2+a)", {"s", "a"}},
        {"(a)/(s^2-a)", {"s", "a"}},
        {"(ev)/(s^2*(s^2+0.01*s+2*ev))", {"s", "ev"}},
        {"(ev)/(s^2*(s^2+2*ev))", {"s", "ev"}},
        {"(1-td*s/2)/((s+a)*(1+td*s/2))", {"s", "td", "a"}},
        {"(s+a)/((s+a)*(s+b))", {"s", "a", "b"}},
        {"(s+a)/(s*(s-2.5))", {"s", "a"}},
        {"(a)/(s*(s+a))", {"s", "a"}},
        {"(1)/(s*(s+a))", {"s", "a"}},
    };
    for (const auto & [text, names] : plants) {
        expectSameAsTree(text, names, pointsOf(names.size() - 1));
    }
}

TEST(ExpressionProgram, ItsDepthIsTheStackItNeeds)
{
    const auto depthOf = [](const char * text, const std::vector<std::string> & names) {
        ExpressionTree tree(text);
        tree.bind(names);
        return ExpressionProgram::compile(tree).depth;
    };
    EXPECT_EQ(depthOf("a", {"a"}), 1u);
    EXPECT_EQ(depthOf("a+b", {"a", "b"}), 2u);
    EXPECT_EQ(depthOf("a+(b*(c+d))", {"a", "b", "c", "d"}), 4u);
    EXPECT_EQ(depthOf("((a+b)*c)+d", {"a", "b", "c", "d"}), 2u);
    EXPECT_EQ(depthOf("sin(a)^2", {"a"}), 1u);
}

TEST(ExpressionProgram, AnUnboundVariableIsRefused)
{
    const ExpressionTree tree("s+a");
    EXPECT_THROW(ExpressionProgram::compile(tree), std::invalid_argument);
}
