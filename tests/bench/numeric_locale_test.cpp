/**
 * @file
 * @brief The benchmark process runs with the C numeric locale.
 *
 * The application object adopts the system locale, and under a decimal-comma
 * one `strtod` stops accepting "0.1"; every number the project reads goes
 * through it, from a frequency in a .qft to the epsilon of a plan. The reset
 * to "C" is a one-line convention no compiler enforces, so it is asserted
 * here: the active numeric locale is "C", a dotted literal is read whole, and
 * a case identifier built from an epsilon never grows a comma, since it
 * becomes a file name.
 */

#include <gtest/gtest.h>

#include <clocale>
#include <string>

#include "src/bench/plan.h"

using namespace qftbx;
using namespace qftbx::bench;

TEST(NumericLocale, TheProcessRunsWithTheCLocaleForNumbers)
{
    const char * active = std::setlocale(LC_NUMERIC, nullptr);

    ASSERT_NE(active, nullptr);
    EXPECT_EQ(std::string(active), "C")
            << "main() must reset LC_NUMERIC to \"C\" after QCoreApplication, "
               "or every number the project reads breaks under a comma locale";
}

TEST(NumericLocale, ADottedDecimalIsReadAsWritten)
{
    const char * text = "0.5";
    char * end = nullptr;
    const double value = std::strtod(text, &end);

    EXPECT_EQ(*end, '\0') << "the whole literal must be consumed";
    EXPECT_DOUBLE_EQ(value, 0.5);
}

TEST(NumericLocale, TheExamplePlanRoundTripsItsEpsilon)
{
    Plan plan = examplePlan();
    plan.epsilons = {0.5, 0.125};

    const std::vector<Case> cases = expandCases(plan);
    ASSERT_FALSE(cases.empty());

    for (const Case & one : cases) {
        const std::string id = caseId(one);
        EXPECT_EQ(id.find(','), std::string::npos) << "identifier grew a comma: " << id;
    }
}
