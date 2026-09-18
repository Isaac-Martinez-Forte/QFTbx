// The record of stages: what it writes, that it is silent until somebody
// opens it, and that it never grows past the size it was given.
//
// The engines used to print their timings on standard output. Nothing in the
// interface showed them, nothing could silence them, and anything driving the
// core from another language got them in the middle of its own session. They
// go here now, and this is the contract.

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include "src/core/common/record.h"
#include "src/core/project/settings.h"

namespace {

std::string contentsOf(const std::string & path)
{
    std::ifstream file(path);
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

std::string temporaryPath(const char * name)
{
    const char * directory = std::getenv("TMPDIR");
    return std::string(directory != nullptr ? directory : "/tmp") + "/qftbx-" + name + ".log";
}

class Record : public ::testing::Test
{
protected:
    void SetUp() override { path = temporaryPath(::testing::UnitTest::GetInstance()
                                                     ->current_test_info()->name());
                            std::remove(path.c_str());
                            std::remove((path + ".1").c_str()); }
    void TearDown() override { qftbx::record::close();
                               std::remove(path.c_str());
                               std::remove((path + ".1").c_str()); }

    std::string path;
};

} // namespace

TEST_F(Record, WritesNothingUntilItIsOpened)
{
    ASSERT_FALSE(qftbx::record::isOpen());

    qftbx::record::write("templates", "sweep", "ms=1.0");

    std::ifstream file(path);
    EXPECT_FALSE(file.good()) << "a record nobody opened wrote a file";
}

TEST_F(Record, OneLinePerStageWithItsNumbers)
{
    qftbx::record::open(path, 1024 * 1024);
    ASSERT_TRUE(qftbx::record::isOpen());
    EXPECT_EQ(qftbx::record::path(), path);

    qftbx::record::write("loop shaping", "mc2",
                         qftbx::record::milliseconds(116.44) + " "
                             + qftbx::record::number("gain", 311.75));

    const std::string written = contentsOf(path);
    EXPECT_NE(written.find("loop shaping"), std::string::npos);
    EXPECT_NE(written.find("mc2"), std::string::npos) << "the line does not say which algorithm ran";
    EXPECT_NE(written.find("ms=116.4"), std::string::npos) << written;
    EXPECT_NE(written.find("gain=311.75"), std::string::npos) << written;
    EXPECT_EQ(std::count(written.begin(), written.end(), '\n'), 1) << "one stage, one line";
}

TEST_F(Record, TimesAStageByItself)
{
    qftbx::record::open(path, 1024 * 1024);

    {
        qftbx::record::Timed stage("boundaries", "sheets");
        stage.note(qftbx::record::number("frequencies", 6));
    }

    const std::string written = contentsOf(path);
    EXPECT_NE(written.find("boundaries"), std::string::npos);
    EXPECT_NE(written.find("ms="), std::string::npos) << "the line has no duration";
    EXPECT_NE(written.find("frequencies=6"), std::string::npos) << written;
}

TEST_F(Record, NeverGrowsPastTheSizeItWasGiven)
{
    //Two generations of a small limit: the file rotates and the older one is
    //dropped, so what is on disc is bounded whatever how long it runs.
    const std::size_t limit = 2048;
    qftbx::record::open(path, limit);

    for (int i = 0; i < 400; ++i) {
        qftbx::record::write("templates", "sweep",
                             qftbx::record::milliseconds(double(i)) + " "
                                 + qftbx::record::number("points", 625));
    }

    const std::string current = contentsOf(path);
    const std::string previous = contentsOf(path + ".1");

    EXPECT_LT(current.size(), limit + 256u) << "the current generation ran past its limit";
    EXPECT_LT(previous.size(), limit + 256u) << "the previous generation ran past its limit";
    EXPECT_FALSE(previous.empty()) << "nothing rotated: the limit was never reached";

    std::ifstream third(path + ".2");
    EXPECT_FALSE(third.good()) << "a third generation was kept";

    //And the newest lines are the ones that survived.
    EXPECT_NE(current.find("ms=399.0"), std::string::npos) << "the last line written is not there";
}

TEST_F(Record, TheSettingsSayWhereItGoesAndWhetherItGoesAtAll)
{
    qftbx::Settings settings;
    EXPECT_FALSE(settings.log.enabled) << "the record is on by default";

    qftbx::openRecord(settings);
    EXPECT_FALSE(qftbx::record::isOpen()) << "the record opened although the settings say not to";

    settings.log.enabled = true;
    settings.log.path = path;
    settings.log.sizeLimitKilobytes = 16;
    qftbx::openRecord(settings);

    EXPECT_TRUE(qftbx::record::isOpen());
    EXPECT_EQ(qftbx::record::path(), path);
}
