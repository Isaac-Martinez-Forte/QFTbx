/**
 * @file
 * @brief Tests of the live-node list and the constraint-propagating tree.
 *
 * The ordered list drives every branch and bound: an insert lands between
 * its neighbours in either direction of ordering, which is what makes the
 * first solution the global optimum; the list frees what is still queued,
 * hands ownership over on removal, refuses a request on an empty list with a
 * message, keeps a high-water mark, and turns the live-node ceiling into a
 * reported error that leaves the list usable, since an unbounded list ends
 * in the out-of-memory killer rather than in an exception. The expression
 * tree is the HC4-style contractor of algorithm MR: evaluation over doubles
 * and intervals, contraction of a domain to the part consistent with a
 * constraint, constants enclosed rather than approximated, copies that work.
 */

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>

#include "src/core/common/exception.h"
#include "src/core/loopshaping/common/ordered_list.h"
#include "src/core/math/expression_tree.h"

#include "src/core/math/interval.h"

using namespace qftbx;

namespace {

std::unique_ptr<ListNode> node(double index)
{
    return std::make_unique<ListNode>(index);
}

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
    OrderedList liveList;
    liveList.insert(node(1));
    liveList.insert(node(3));
    liveList.insert(node(5));

    liveList.insert(node(4));

    liveList.takeFirst();
    EXPECT_EQ(liveList.first()->getIndex(), 3);
    liveList.takeFirst();
    EXPECT_EQ(liveList.first()->getIndex(), 4);
    liveList.takeFirst();
    EXPECT_EQ(liveList.first()->getIndex(), 5);
}

TEST(OrderedList, DescendingListAcceptsALargerFront)
{
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
    OrderedList liveList(false, 3);

    liveList.insert(node(1));
    liveList.insert(node(2));
    liveList.insert(node(3));

    EXPECT_EQ(liveList.size(), 3u);
    EXPECT_THROW(liveList.insert(node(4)), qftbx::ComputationError);

    EXPECT_EQ(liveList.size(), 3u);
    EXPECT_EQ(liveList.first()->getIndex(), 1);

    liveList.takeFirst();
    EXPECT_NO_THROW(liveList.insert(node(4)));
}

TEST(OrderedList, ThePeakIsTheHighWaterMarkNotTheCurrentSize)
{
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

    EXPECT_EQ(CountingNode::alive, 0);
    EXPECT_TRUE(liveList.isEmpty());
}

TEST(OrderedList, AnEmptyListRefusesToHandOutANode)
{
    OrderedList liveList;

    EXPECT_THROW(liveList.first(), qftbx::ComputationError);
    EXPECT_THROW(liveList.last(), qftbx::ComputationError);
    EXPECT_THROW(liveList.takeFirst(), qftbx::ComputationError);
}

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

    std::map<std::string, Interval> variables;
    variables["x"] = Interval(1.0, 4.0);

    const Interval result = tree.eval(&variables);
    EXPECT_DOUBLE_EQ((result).lower(), 5.0);
    EXPECT_DOUBLE_EQ((result).upper(), 11.0);
}

TEST(ExpressionTree, ContractionNarrowsAnInconsistentDomain)
{
    qftbx::ExpressionTree tree("1");
    tree.setFunc(std::string("x-2"), 0.0, qftbx::GREATER_EQUAL);

    std::map<std::string, Interval> variables;
    variables["x"] = Interval(0.0, 10.0);

    const bool consistent = tree.propagate(&variables);

    EXPECT_TRUE(consistent);
    EXPECT_DOUBLE_EQ(variables.at("x").lower(), 2.0);
    EXPECT_DOUBLE_EQ(variables.at("x").upper(), 10.0);
}

TEST(ExpressionTree, ContractionDetectsAnEmptyDomain)
{
    qftbx::ExpressionTree tree("1");
    tree.setFunc(std::string("x-20"), 0.0, qftbx::GREATER_EQUAL);

    std::map<std::string, Interval> variables;
    variables["x"] = Interval(0.0, 10.0);

    EXPECT_FALSE(tree.propagate(&variables));
}

TEST(ExpressionTree, TheConstantsAreEnclosedNotApproximated)
{
    qftbx::ExpressionTree tree("1");
    tree.setFunc(std::string("PI+E"));

    std::map<std::string, Interval> variables;
    const Interval result = tree.eval(&variables);

    const double truth = 3.14159265358979323846 + 2.71828182845904523536;
    EXPECT_LE((result).lower(), truth);
    EXPECT_GE((result).upper(), truth);
    EXPECT_LT((result).upper() - (result).lower(), 1e-12);

    EXPECT_NEAR(tree.eval(static_cast<std::map<std::string, double> *>(nullptr)), truth, 1e-15);
}

TEST(ExpressionTree, TheLogarithmsEvaluateOverIntervals)
{
    qftbx::ExpressionTree tree("1");
    tree.setFunc(std::string("ln(x)+lg(x)"));

    std::map<std::string, Interval> variables;
    variables["x"] = Interval(1.0, 10.0);

    const Interval result = tree.eval(&variables);
    EXPECT_NEAR((result).lower(), 0.0, 1e-12);
    EXPECT_NEAR((result).upper(), std::log(10.0) + 1.0, 1e-12);
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

    std::map<std::string, Interval> variables;
    variables["x"] = Interval(0.0, 10.0);

    ASSERT_TRUE(tree.propagate(&variables));
    EXPECT_DOUBLE_EQ(variables.at("x").lower(), 0.0);
    EXPECT_DOUBLE_EQ(variables.at("x").upper(), 2.0);
}

TEST(ExpressionTree, AMalformedExpressionIsRefused)
{
    qftbx::ExpressionTree tree("1");
    EXPECT_THROW(tree.setFunc(std::string("(2*x")), std::invalid_argument);
    EXPECT_THROW(tree.setFunc(std::string("2*x)")), std::invalid_argument);
    EXPECT_THROW(tree.setFunc(std::string("2*")), std::invalid_argument);
}

}
