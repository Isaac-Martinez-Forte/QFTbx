// Tests for the loop-shaping support structures: the live-node ordered list
// that drives every branch & bound, and the expression tree that implements
// the HC4-style contractor of the MR (Rambabu/ICSP) algorithm.

#include <gtest/gtest.h>

//A failed comparison of C-XSC values must report, not crash: see the header.
#include "tests/backend/cxsc_printing.h"

#include <map>
#include <memory>
#include <string>

#include "src/core/common/exception.h"
#include "src/core/loopshaping/ordered_list.h"
#include "src/core/math/expression_tree.h"

#include <interval.hpp>

using namespace qftbx;

namespace {

using cxsc::interval;

std::unique_ptr<ListNode> node(double index)
{
    return std::make_unique<ListNode>(index);
}

//Counts its own lifetime, to check who frees the nodes.
class CountingNode : public ListNode
{
public:
    explicit CountingNode(double index) : ListNode(index) { alive++; }
    ~CountingNode() override { alive--; }

    static int alive;
};

int CountingNode::alive = 0;

TEST(OrderedList, AscendingInsertsKeepTheOrder)
{
    OrderedList liveList;

    liveList.insert(node(1));
    liveList.insert(node(3));
    liveList.insert(node(5));
    liveList.insert(node(8));
    liveList.insert(node(9));

    EXPECT_EQ(liveList.first()->getIndex(), 1);
    EXPECT_EQ(liveList.last()->getIndex(), 9);
}

TEST(OrderedList, SmallerThanFirstGoesToTheFront)
{
    OrderedList liveList;
    liveList.insert(node(5));
    liveList.insert(node(2));

    EXPECT_EQ(liveList.first()->getIndex(), 2);
}

TEST(OrderedList, MiddleInsertKeepsTheOrder)
{
    // The historical insert() placed the element at i-1 instead of i, so
    // a value that belongs between two existing nodes landed BEFORE its
    // smaller neighbour, breaking the ordering that makes the first
    // solution of every branch & bound the global optimum. Fixed in 8b.2.
    OrderedList liveList;
    liveList.insert(node(1));
    liveList.insert(node(3));
    liveList.insert(node(5));

    liveList.insert(node(4)); // belongs between 3 and 5

    //takeFirst hands the ownership over, so the node dies with the
    //temporary it is returned in.
    liveList.takeFirst();   // 1
    EXPECT_EQ(liveList.first()->getIndex(), 3);
    liveList.takeFirst();
    EXPECT_EQ(liveList.first()->getIndex(), 4);
    liveList.takeFirst();
    EXPECT_EQ(liveList.first()->getIndex(), 5);
}

TEST(OrderedList, DescendingListAcceptsALargerFront)
{
    // The historical version crashed here: on a descending list the first
    // comparison already matched at position 0 and it called insert(-1)
    // (the SIGSEGV that killed the MC algorithm on the benchmarks).
    OrderedList liveList(true);
    liveList.insert(node(5));
    liveList.insert(node(8));

    EXPECT_EQ(liveList.first()->getIndex(), 8);
    EXPECT_EQ(liveList.last()->getIndex(), 5);
}

TEST(OrderedList, FirstRetrieveAndDeleteWork)
{
    OrderedList liveList;
    EXPECT_TRUE(liveList.isEmpty());

    liveList.insert(node(7));
    EXPECT_FALSE(liveList.isEmpty());

    std::unique_ptr<ListNode> primero = liveList.takeFirst();
    EXPECT_EQ(primero->getIndex(), 7);

    EXPECT_TRUE(liveList.isEmpty());
}

TEST(OrderedList, TheListFreesWhatIsStillQueued)
{
    //A successful branch & bound returns with millions of live nodes left
    //in the list; they are the list's to free.
    CountingNode::alive = 0;

    {
        OrderedList liveList;
        liveList.insert(std::make_unique<CountingNode>(1));
        liveList.insert(std::make_unique<CountingNode>(2));

        EXPECT_EQ(CountingNode::alive, 2);
    }

    EXPECT_EQ(CountingNode::alive, 0);
}

TEST(OrderedList, TheLiveNodeCeilingIsReportedNotCrashed)
{
    //A branch and bound that cannot resolve its problem grows the live list
    //without limit. With the default heuristic overcommit of Linux that ends
    //in the OOM killer rather than in a std::bad_alloc anyone can report, so
    //the ceiling is the only mechanism that turns it into a diagnosis.
    OrderedList liveList(false, 3);

    liveList.insert(node(1));
    liveList.insert(node(2));
    liveList.insert(node(3));

    EXPECT_EQ(liveList.size(), 3u);
    EXPECT_THROW(liveList.insert(node(4)), qftbx::ComputationError);

    //The refusal leaves the list usable: what was queued is still there, in
    //order, and taking one out makes room again.
    EXPECT_EQ(liveList.size(), 3u);
    EXPECT_EQ(liveList.first()->getIndex(), 1);

    liveList.takeFirst();
    EXPECT_NO_THROW(liveList.insert(node(4)));
}

TEST(OrderedList, ThePeakIsTheHighWaterMarkNotTheCurrentSize)
{
    //The peak is what a run cost in memory, so it must not fall back when
    //the search drains the list.
    OrderedList liveList;

    liveList.insert(node(1));
    liveList.insert(node(2));
    liveList.insert(node(3));
    EXPECT_EQ(liveList.peakSize(), 3u);

    liveList.takeFirst();
    liveList.takeFirst();

    EXPECT_EQ(liveList.size(), 1u);
    EXPECT_EQ(liveList.peakSize(), 3u);
}

TEST(OrderedList, TakeFirstHandsTheNodeOver)
{
    CountingNode::alive = 0;

    OrderedList liveList;
    liveList.insert(std::make_unique<CountingNode>(1));

    {
        std::unique_ptr<ListNode> taken = liveList.takeFirst();
        EXPECT_EQ(CountingNode::alive, 1);
    }

    //Out of the taker's scope the node is gone, and the list did not
    //keep a second owner of it.
    EXPECT_EQ(CountingNode::alive, 0);
    EXPECT_TRUE(liveList.isEmpty());
}

TEST(OrderedList, AnEmptyListRefusesToHandOutANode)
{
    // first(), last() and takeFirst() used to dereference end() on an empty
    // list; a search that asks for a node it does not have gets a message.
    OrderedList liveList;

    EXPECT_THROW(liveList.first(), qftbx::ComputationError);
    EXPECT_THROW(liveList.last(), qftbx::ComputationError);
    EXPECT_THROW(liveList.takeFirst(), qftbx::ComputationError);
}

//---------------------------------------------------------------- ExpressionTree

TEST(ExpressionTree, ScalarEvaluationWithVariables)
{
    qftbx::ExpressionTree tree("1");
    tree.setFunc(std::string("2*x+3"));

    std::map<std::string, double> variables;
    variables["x"] = 5.0;

    EXPECT_DOUBLE_EQ(tree.eval(&variables), 13.0);
}

TEST(ExpressionTree, ScalarEvaluationWithFunctionsAndConstants)
{
    qftbx::ExpressionTree tree("1");
    tree.setFunc(std::string("cos(0)+sqrt(9)"));

    EXPECT_DOUBLE_EQ(tree.eval(static_cast<std::map<std::string, double> *>(nullptr)), 4.0);
}

TEST(ExpressionTree, IntervalEvaluationEnclosesTheRange)
{
    qftbx::ExpressionTree tree("1");
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
    qftbx::ExpressionTree tree("1");
    tree.setFunc(std::string("x-2"), 0.0, qftbx::GREATER_EQUAL);

    std::map<std::string, interval> variables;
    variables["x"] = interval(0.0, 10.0);

    const bool consistent = tree.propagate(&variables);

    EXPECT_TRUE(consistent);
    EXPECT_DOUBLE_EQ(cxsc::_double(Inf(variables.at("x"))), 2.0);
    EXPECT_DOUBLE_EQ(cxsc::_double(Sup(variables.at("x"))), 10.0);
}

TEST(ExpressionTree, ContractionDetectsAnEmptyDomain)
{
    qftbx::ExpressionTree tree("1");
    tree.setFunc(std::string("x-20"), 0.0, qftbx::GREATER_EQUAL);

    std::map<std::string, interval> variables;
    variables["x"] = interval(0.0, 10.0);

    EXPECT_FALSE(tree.propagate(&variables));
}

TEST(ExpressionTree, TheConstantsAreEnclosedNotApproximated)
{
    qftbx::ExpressionTree tree("1");
    tree.setFunc(std::string("PI+E"));

    std::map<std::string, interval> variables;
    const interval result = tree.eval(&variables);

    // The true pi + e lies strictly inside: the interval version used to
    // return degenerate intervals of approximate constants.
    const double truth = 3.14159265358979323846 + 2.71828182845904523536;
    EXPECT_LE(cxsc::_double(Inf(result)), truth);
    EXPECT_GE(cxsc::_double(Sup(result)), truth);
    EXPECT_LT(cxsc::_double(Sup(result)) - cxsc::_double(Inf(result)), 1e-12);

    EXPECT_NEAR(tree.eval(static_cast<std::map<std::string, double> *>(nullptr)), truth, 1e-15);
}

TEST(ExpressionTree, TheLogarithmsEvaluateOverIntervals)
{
    qftbx::ExpressionTree tree("1");
    tree.setFunc(std::string("ln(x)+lg(x)"));

    std::map<std::string, interval> variables;
    variables["x"] = interval(1.0, 10.0);

    const interval result = tree.eval(&variables);
    EXPECT_NEAR(cxsc::_double(Inf(result)), 0.0, 1e-12);
    EXPECT_NEAR(cxsc::_double(Sup(result)), std::log(10.0) + 1.0, 1e-12);
}

TEST(ExpressionTree, ACopyKeepsItsVariables)
{
    qftbx::ExpressionTree original("1");
    original.setFunc(std::string("2*x+3"));

    qftbx::ExpressionTree copy(original);
    qftbx::ExpressionTree assigned("1");
    assigned = original;

    std::map<std::string, double> variables;
    variables["x"] = 5.0;

    EXPECT_DOUBLE_EQ(copy.eval(&variables), 13.0);
    EXPECT_DOUBLE_EQ(assigned.eval(&variables), 13.0);
}

TEST(ExpressionTree, AnUpperCaseNameIsAVariableNotAConstant)
{
    qftbx::ExpressionTree tree("1");
    tree.setFunc(std::string("P1+E2"));

    std::map<std::string, double> variables;
    variables["P1"] = 1.0;
    variables["E2"] = 2.0;

    EXPECT_DOUBLE_EQ(tree.eval(&variables), 3.0);
}

TEST(ExpressionTree, ContractionHonoursALessThanConstraint)
{
    qftbx::ExpressionTree tree("1");
    tree.setFunc(std::string("x-2"), 0.0, qftbx::LESS_EQUAL);

    std::map<std::string, interval> variables;
    variables["x"] = interval(0.0, 10.0);

    ASSERT_TRUE(tree.propagate(&variables));
    EXPECT_DOUBLE_EQ(cxsc::_double(Inf(variables.at("x"))), 0.0);
    EXPECT_DOUBLE_EQ(cxsc::_double(Sup(variables.at("x"))), 2.0);
}

TEST(ExpressionTree, AMalformedExpressionIsRefused)
{
    qftbx::ExpressionTree tree("1");
    EXPECT_THROW(tree.setFunc(std::string("(2*x")), std::invalid_argument);
    EXPECT_THROW(tree.setFunc(std::string("2*x)")), std::invalid_argument);
    EXPECT_THROW(tree.setFunc(std::string("2*")), std::invalid_argument);
}

} // namespace
