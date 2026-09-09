// The numeric locale of the process, which the benchmark module forgot to
// pin.
//
// QCoreApplication adopts the system locale when it is constructed, so under
// a decimal-comma locale (es_ES, de_DE, fr_FR...) std::strtod stops
// accepting "0.1". Every number the project reads goes through it: the
// <min-frequency> of a .qft, the epsilon of a benchmark plan. The
// application's main() and the backend and GUI test mains all reset
// LC_NUMERIC to "C" right after the application object for exactly this
// reason; the benchmark tool and this test binary were written later and did
// not (2026-09-09). The tool then refused a plan with epsilon="0.5" -- "is
// not a number" -- and, once past that, refused the project itself.
//
// This is a one-line convention with no compiler to enforce it, so it is
// pinned here instead: on a machine whose environment says comma the whole
// suite fails without the line, and on any machine the assertion below does.

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
    //What the plan reader does with the attribute values, and what broke.
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

    //A comma would survive into the identifier, which becomes a file name:
    //caseId() only replaces dots.
    for (const Case & one : cases) {
        const std::string id = caseId(one);
        EXPECT_EQ(id.find(','), std::string::npos) << "identifier grew a comma: " << id;
    }
}
