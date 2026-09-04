// A failed comparison of C-XSC values must REPORT, not crash. GoogleTest
// prints the operands of a failure, and printing a cxsc::real through the
// library's own formatting (r_outpx -> b_out) dereferences a null pointer and
// kills the process with SIGSEGV - natively, not only under valgrind. That
// turned every future interval assertion failure into a crash with no
// message. tests/backend/cxsc_printing.h supplies the PrintTo overloads that
// keep the suite able to speak.

#include <gtest/gtest.h>

#include <string>

#include "tests/backend/cxsc_printing.h"


namespace {

TEST(CxscPrinting, PrintingARealDoesNotCrashAndSaysTheValue)
{
    const cxsc::real tenth = cxsc::real(0.1);

    const std::string printed = ::testing::PrintToString(tenth);

    EXPECT_FALSE(printed.empty()) << "a cxsc::real printed as nothing";
    EXPECT_NE(printed.find("0.1"), std::string::npos)
        << "printed as \"" << printed << "\"";
}

TEST(CxscPrinting, PrintingAnIntervalShowsBothBounds)
{
    const cxsc::interval tenth = cxsc::interval(1.0) / cxsc::interval(10.0);

    const std::string printed = ::testing::PrintToString(tenth);

    EXPECT_NE(printed.find('['), std::string::npos)
        << "printed as \"" << printed << "\"";
    EXPECT_NE(printed.find(','), std::string::npos)
        << "printed as \"" << printed << "\"";
}

TEST(CxscPrinting, PrintingAComplexIntervalShowsTheFourBounds)
{
    const cxsc::cinterval box(cxsc::interval(1.0, 2.0), cxsc::interval(-3.0, -1.0));

    const std::string printed = ::testing::PrintToString(box);

    EXPECT_NE(printed.find('i'), std::string::npos)
        << "printed as \"" << printed << "\"";
    EXPECT_FALSE(printed.empty());
}

} // namespace
