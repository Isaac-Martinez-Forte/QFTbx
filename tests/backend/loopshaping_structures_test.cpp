// Characterisation tests for the loop-shaping support structures (phase
// 8b.0 safety net): the live-node ordered list that drives every branch &
// bound, and the expression tree that implements the HC4-style contractor
// of the MR (Rambabu/ICSP) algorithm. They pin CURRENT behaviour; known
// defects are marked // BUG: and will be fixed consciously in 8b.1.

#include <gtest/gtest.h>

//A failed comparison of C-XSC values must report, not crash: see the header.
#include "tests/backend/cxsc_printing.h"

#include <map>
#include <memory>
#include <string>

#include "src/core/exception.h"
#include "src/core/loopshaping/ordered_list.h"
#include "src/core/loopshaping/expression_tree.h"

#include <interval.hpp>

namespace {

using cxsc::interval;

std::unique_ptr<ListNode> node(qreal index)
{
    return std::make_unique<ListNode>(index);
}

//Counts its own lifetime, to check who frees the nodes.
class CountingNode : public ListNode
{
public:
    explicit CountingNode(qreal index) : ListNode(index) { alive++; }
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
    tree.setFunc(std::string("x-2"), 0.0, alg::GREATER_EQUAL);

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
    tree.setFunc(std::string("x-20"), 0.0, alg::GREATER_EQUAL);

    std::map<std::string, interval> variables;
    variables["x"] = interval(0.0, 10.0);

    EXPECT_FALSE(tree.propagate(&variables));
}

} // namespace
