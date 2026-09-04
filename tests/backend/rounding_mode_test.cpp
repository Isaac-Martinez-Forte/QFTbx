// The whole rigour claim of this toolbox rests on directed rounding: C-XSC
// computes an interval's lower bound with the mode set toward -inf and its
// upper bound toward +inf, in hand-written x86-64 assembly (r_ari_x86_64.s,
// which manipulates the SSE control word with ldmxcsr).
//
// The rounding mode is PROCESS-WIDE state. Two things therefore have to hold,
// and neither is checked anywhere else:
//
//   - Every interval operation must leave the mode as it found it. If one
//     leaks a directed mode, every plain double computation afterwards
//     silently rounds the wrong way - which is exactly the shape of a golden
//     test that fails once in a full run and never reproduces in isolation,
//     because run alone it never inherits the leaked mode.
//   - An interval operation must give the same enclosure whatever mode it is
//     entered with, or its result depends on whoever ran before it.

#include <gtest/gtest.h>

//A failed comparison of C-XSC values must report, not crash: see the header.
#include "tests/backend/cxsc_printing.h"

#include <cfenv>

#include "interval.hpp"
#include "complex.hpp"
#include "cinterval.hpp"


namespace {

//Restores whatever mode the test found, so a failure here cannot poison the
//rest of the binary.
class RoundingMode : public ::testing::Test
{
protected:
    void SetUp() override { m_entry = std::fegetround(); }
    void TearDown() override { std::fesetround(m_entry); }

private:
    int m_entry = FE_TONEAREST;
};

//A small interval computation of the kind the sweeps run: the square root of
//a complex modulus, which is where C-XSC's directed rounding shows up.
cxsc::interval enclose()
{
    cxsc::interval a(0.1, 0.3);
    cxsc::interval b(1.7, 2.1);

    return sqrt(a * a + b * b) / (a + b);
}

TEST_F(RoundingMode, AnIntervalOperationLeavesTheModeAsItFoundIt)
{
    ASSERT_EQ(std::fegetround(), FE_TONEAREST)
        << "the mode was already not to-nearest when this test started: "
           "something earlier in the binary leaked a directed mode";

    const cxsc::interval result = enclose();
    EXPECT_LE(Inf(result), Sup(result));

    EXPECT_EQ(std::fegetround(), FE_TONEAREST)
        << "an interval operation left the process rounding in a directed "
           "mode: every plain double computation after it rounds the wrong way";
}

TEST_F(RoundingMode, TheEnclosureDoesNotDependOnTheIncomingMode)
{
    //Non-degeneracy FIRST, because without it the rest of this test passes
    //for the wrong reason: if directed rounding were not in effect at all,
    //every mode would give the same answer and the comparisons below would
    //be satisfied trivially. 1/10 is not a binary float, so a rigorous
    //quotient has to straddle it. (This is what exposed valgrind, which
    //ignores the rounding mode and collapses the interval to a point.)
    const cxsc::interval tenth = cxsc::interval(1.0) / cxsc::interval(10.0);
    ASSERT_LT(_double(Inf(tenth)), _double(Sup(tenth)))
        << "the quotient collapsed to a point: directed rounding is not in "
           "effect, so nothing below this line proves anything";

    const cxsc::interval fromNearest = enclose();

    //Entered with the mode already pointing the wrong way, as a leak from
    //earlier code would leave it.
    for (int mode : {FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
        ASSERT_EQ(std::fesetround(mode), 0) << "mode " << mode << " unavailable";

        const cxsc::interval entered = enclose();

        EXPECT_DOUBLE_EQ(_double(Inf(entered)), _double(Inf(fromNearest)))
            << "the lower bound depends on the incoming rounding mode "
            << mode;
        EXPECT_DOUBLE_EQ(_double(Sup(entered)), _double(Sup(fromNearest)))
            << "the upper bound depends on the incoming rounding mode "
            << mode;
    }
}

TEST_F(RoundingMode, PlainArithmeticIsToNearestAfterIntervalWork)
{
    //The observable consequence of a leak, stated in terms a reader can
    //check: 1 + 2^-53 rounds to 1.0 to nearest, and to the next double up
    //when rounding toward +inf.
    enclose();

    const double justAboveOne = 1.0 + 0x1p-53;
    EXPECT_DOUBLE_EQ(justAboveOne, 1.0)
        << "plain double arithmetic is not rounding to nearest after an "
           "interval computation";
}

} // namespace
