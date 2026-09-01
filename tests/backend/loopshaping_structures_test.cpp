// Characterisation tests for the loop-shaping support structures (phase
// 8b.0 safety net): the live-node ordered list that drives every branch &
// bound, and the expression tree that implements the HC4-style contractor
// of the MR (Rambabu/ICSP) algorithm. They pin CURRENT behaviour; known
// defects are marked // BUG: and will be fixed consciously in 8b.1.

#include <gtest/gtest.h>

//A failed comparison of C-XSC values must report, not crash: see the header.
#include "tests/backend/cxsc_printing.h"

#include <map>
#include <string>

#include "src/core/loopshaping/ordered_list.h"
#include "src/core/loopshaping/expression_tree.h"

#include <interval.hpp>

namespace {

using cxsc::interval;

ListNode* node(qreal index)
{
    return new ListNode(index);
}

TEST(OrderedList, AscendingInsertsKeepTheOrder)
{
    OrderedList lista;

    lista.insert(node(1));
    lista.insert(node(3));
    lista.insert(node(5));
    lista.insert(node(8));
    lista.insert(node(9));

    EXPECT_EQ(lista.first()->getIndex(), 1);
    EXPECT_EQ(lista.last()->getIndex(), 9);
}

TEST(OrderedList, SmallerThanFirstGoesToTheFront)
{
    OrderedList lista;
    lista.insert(node(5));
    lista.insert(node(2));

    EXPECT_EQ(lista.first()->getIndex(), 2);
}

TEST(OrderedList, MiddleInsertKeepsTheOrder)
{
    // The historical insert() placed the element at i-1 instead of i, so
    // a value that belongs between two existing nodes landed BEFORE its
    // smaller neighbour, breaking the ordering that makes the first
    // solution of every branch & bound the global optimum. Fixed in 8b.2.
    OrderedList lista;
    lista.insert(node(1));
    lista.insert(node(3));
    lista.insert(node(5));

    lista.insert(node(4)); // belongs between 3 and 5

    //takeFirst hands the node over, so the test owns it from here.
    delete lista.takeFirst();   // 1
    EXPECT_EQ(lista.first()->getIndex(), 3);
    delete lista.takeFirst();
    EXPECT_EQ(lista.first()->getIndex(), 4);
    delete lista.takeFirst();
    EXPECT_EQ(lista.first()->getIndex(), 5);
}

TEST(OrderedList, DescendingListAcceptsALargerFront)
{
    // The historical version crashed here: on a descending list the first
    // comparison already matched at position 0 and it called insert(-1)
    // (the SIGSEGV that killed the MC algorithm on the benchmarks).
    OrderedList lista(true);
    lista.insert(node(5));
    lista.insert(node(8));

    EXPECT_EQ(lista.first()->getIndex(), 8);
    EXPECT_EQ(lista.last()->getIndex(), 5);
}

TEST(OrderedList, FirstRetrieveAndDeleteWork)
{
    OrderedList lista;
    EXPECT_TRUE(lista.isEmpty());

    lista.insert(node(7));
    EXPECT_FALSE(lista.isEmpty());

    ListNode* primero = lista.takeFirst();
    EXPECT_EQ(primero->getIndex(), 7);
    delete primero;

    EXPECT_TRUE(lista.isEmpty());
}

//---------------------------------------------------------------- ExpressionTree

TEST(ExpressionTree, ScalarEvaluationWithVariables)
{
    alg::ExpressionTree tree("1");
    tree.setFunc(std::string("2*x+3"));

    std::map<std::string, qreal> variables;
    variables["x"] = 5.0;

    EXPECT_DOUBLE_EQ(tree.eval(&variables), 13.0);
}

TEST(ExpressionTree, ScalarEvaluationWithFunctionsAndConstants)
{
    alg::ExpressionTree tree("1");
    tree.setFunc(std::string("cos(0)+sqrt(9)"));

    EXPECT_DOUBLE_EQ(tree.eval(static_cast<std::map<std::string, qreal> *>(nullptr)), 4.0);
}

TEST(ExpressionTree, IntervalEvaluationEnclosesTheRange)
{
    alg::ExpressionTree tree("1");
    tree.setFunc(std::string("2*x+3"));

    std::map<std::string, interval> variables;
    variables["x"] = interval(1.0, 4.0);

    const interval result = tree.eval(&variables);
    EXPECT_DOUBLE_EQ(cxsc::_double(Inf(result)), 5.0);
    EXPECT_DOUBLE_EQ(cxsc::_double(Sup(result)), 11.0);
}

TEST(ExpressionTree, ContractionNarrowsAnInconsistentDomain)
{
    // propagate() is the HC4-style contractor of the MR algorithm: it must
    // shrink the variable domains to the part consistent with the
    // constraint (expression >= threshold by default in the MR usage).
    alg::ExpressionTree tree("1");
    tree.setFunc(std::string("x-2"), 0.0, alg::MAYORIGUAL);

    std::map<std::string, interval> variables;
    variables["x"] = interval(0.0, 10.0);

    const bool consistent = tree.propagate(&variables);

    EXPECT_TRUE(consistent);
    EXPECT_DOUBLE_EQ(cxsc::_double(Inf(variables.at("x"))), 2.0);
    EXPECT_DOUBLE_EQ(cxsc::_double(Sup(variables.at("x"))), 10.0);
}

TEST(ExpressionTree, ContractionDetectsAnEmptyDomain)
{
    alg::ExpressionTree tree("1");
    tree.setFunc(std::string("x-20"), 0.0, alg::MAYORIGUAL);

    std::map<std::string, interval> variables;
    variables["x"] = interval(0.0, 10.0);

    EXPECT_FALSE(tree.propagate(&variables));
}

} // namespace
