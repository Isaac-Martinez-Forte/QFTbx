// The messages of the core: the text kept apart from its arguments, the
// English rendering, and what the exceptions carry.
#include <gtest/gtest.h>

#include <string>

#include "src/core/common/exception.h"
#include "src/core/common/message.h"

using namespace qftbx;

TEST(Message, RendersItsArgumentsInOrder)
{
    const Message m = QFTBX_TR("Core", "%2 comes after %1, and %1 again").arg("one").arg(2);
    EXPECT_EQ(m.context(), "Core");
    EXPECT_EQ(m.rendered(), "2 comes after one, and one again");
    EXPECT_EQ(m.arguments().size(), 2u);
}

TEST(Message, NumbersRenderAsTheCoreAlwaysWroteThem)
{
    EXPECT_EQ(QFTBX_TR("Core", "%1 points").arg(std::size_t(12)).rendered(), "12 points");
    EXPECT_EQ(QFTBX_TR("Core", "%1 rad/s").arg(0.5).rendered(), "0.5 rad/s");
    EXPECT_EQ(QFTBX_TR("Core", "%1").arg(1e-9).rendered(), "1e-09");
}

TEST(Message, ATenthPlaceholderIsNotTheFirst)
{
    //As QString::arg(): the second argument takes the lowest placeholder
    //left, which is %10, and "%1" inside "%10" is not a placeholder.
    const Message m = QFTBX_TR("Core", "%1 %10").arg("one").arg("ten");
    EXPECT_EQ(m.rendered(), "one ten");
}

TEST(Message, AnExceptionCarriesItsMessageAndSaysItInEnglish)
{
    const InvalidInput failure(QFTBX_TR("Core", "The grid asks for %1 cells, and the limit is %2.").arg(5000000).arg(1000000));
    EXPECT_STREQ(failure.what(), "The grid asks for 5000000 cells, and the limit is 1000000.");
    EXPECT_EQ(failure.message().text(), "The grid asks for %1 cells, and the limit is %2.");

    const FileError plain("a text nobody translates");
    EXPECT_STREQ(plain.what(), "a text nobody translates");
    EXPECT_TRUE(plain.message().context().empty());
}

TEST(Message, AParseErrorFramesTheMessageWithItsFileAndLine)
{
    const ParseError failure(QFTBX_TR("Core", "missing <%1> element").arg("plant"), 12, "project.qft");
    EXPECT_STREQ(failure.what(), "project.qft: missing <plant> element (line 12)");
    EXPECT_EQ(failure.line(), 12);
    EXPECT_EQ(failure.file(), "project.qft");
    EXPECT_EQ(failure.innerMessage().rendered(), "missing <plant> element");

    const ParseError noFile("odd", 3);
    EXPECT_STREQ(noFile.what(), "odd (line 3)");
}
