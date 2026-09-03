// Smoke tests for ProjectReader: load the sample .qft project files shipped
// as test data and check that every section present in the file is recognised
// and recovered.
//
// These tests pin the current behaviour of the loader so that the ongoing
// backend refactor can be validated against it.

#include <gtest/gtest.h>

#include <string>

#include <vector>


#include "src/persistence/project_reader.h"
#include "src/core/system/lti_system.h"
#include "src/core/exception.h"
#include "src/core/frequencies/omega.h"

namespace {

// Indices of the flag vector returned by ProjectReader::load().
enum SectionFlag {
    kPlant = 0,
    kSpecifications,
    kOmega,
    kTemplates,
    kBoundaries,
    kController,
    kLoopShaping,
    kContour,
    kSectionFlagCount
};

std::string fixturePath(const char *name)
{
    return std::string(QFTBX_TEST_DATA_DIR "/") + name;
}

std::vector<bool> loadFlags(ProjectReader &parser, const char *fixture)
{
    //By value: there is no null to rule out any more, and nothing to free.
    std::vector<bool> flags = parser.load(fixturePath(fixture));
    EXPECT_EQ(static_cast<int>(flags.size()), kSectionFlagCount)
        << "unexpected flag count";
    return flags;
}

TEST(ProjectReaderSmoke, CerveraLoadsPlantAndFrequenciesOnly)
{
    ProjectReader parser;
    const std::vector<bool> flags = loadFlags(parser, "cervera.qft");
    ASSERT_EQ(flags.size(), kSectionFlagCount);

    EXPECT_TRUE(flags[kPlant]);
    EXPECT_TRUE(flags[kOmega]);
    EXPECT_FALSE(flags[kSpecifications]);
    EXPECT_FALSE(flags[kTemplates]);
    EXPECT_FALSE(flags[kBoundaries]);
    EXPECT_FALSE(flags[kController]);
    EXPECT_FALSE(flags[kLoopShaping]);
    EXPECT_FALSE(flags[kContour]);

    ASSERT_NE(parser.plant(), nullptr);

    Omega *omega = parser.omega();
    ASSERT_NE(omega, nullptr);
    ASSERT_NE(omega->values(), nullptr);
    const std::vector<double> &values = *omega->values();
    ASSERT_EQ(values.size(), 4);
    EXPECT_DOUBLE_EQ(values[0], 0.1);
    EXPECT_DOUBLE_EQ(values[1], 5.0);
    EXPECT_DOUBLE_EQ(values[2], 10.0);
    EXPECT_DOUBLE_EQ(values[3], 100.0);
}

TEST(ProjectReaderSmoke, Planta2LoadsUpToTemplates)
{
    ProjectReader parser;
    const std::vector<bool> flags = loadFlags(parser, "planta2.qft");
    ASSERT_EQ(flags.size(), kSectionFlagCount);

    EXPECT_TRUE(flags[kPlant]);
    EXPECT_TRUE(flags[kSpecifications]);
    EXPECT_TRUE(flags[kOmega]);
    EXPECT_TRUE(flags[kTemplates]);
    EXPECT_TRUE(flags[kContour]);
    EXPECT_FALSE(flags[kBoundaries]);
    EXPECT_FALSE(flags[kController]);
    EXPECT_FALSE(flags[kLoopShaping]);

    // One template (full cloud + contour) per design frequency.
    ASSERT_FALSE(parser.templates().empty());
    EXPECT_EQ(static_cast<int>(parser.templates().size()), 6);
    ASSERT_FALSE(parser.contour().empty());
    EXPECT_EQ(static_cast<int>(parser.contour().size()), 6);
}

TEST(ProjectReaderSmoke, MultivaluadosLoadsUpToBoundaries)
{
    ProjectReader parser;
    const std::vector<bool> flags = loadFlags(parser, "multivaluados.qft");
    ASSERT_EQ(flags.size(), kSectionFlagCount);

    EXPECT_TRUE(flags[kPlant]);
    EXPECT_TRUE(flags[kSpecifications]);
    EXPECT_TRUE(flags[kOmega]);
    EXPECT_TRUE(flags[kTemplates]);
    EXPECT_TRUE(flags[kContour]);
    EXPECT_TRUE(flags[kBoundaries]);
    EXPECT_TRUE(flags[kController]);
    EXPECT_FALSE(flags[kLoopShaping]);

    ASSERT_FALSE(parser.templates().empty());
    EXPECT_EQ(static_cast<int>(parser.templates().size()), 5);
    EXPECT_NE(parser.boundaries(), nullptr);
}

TEST(ProjectReaderSmoke, Planta1LoadsFullProject)
{
    ProjectReader parser;
    const std::vector<bool> flags = loadFlags(parser, "planta1.qft");
    ASSERT_EQ(flags.size(), kSectionFlagCount);

    for (int i = 0; i < kSectionFlagCount; ++i) {
        EXPECT_TRUE(flags[i]) << "section flag " << i << " not recovered";
    }

    ASSERT_FALSE(parser.templates().empty());
    EXPECT_EQ(static_cast<int>(parser.templates().size()), 4);
    EXPECT_NE(parser.boundaries(), nullptr);
}

TEST(ProjectReaderErrors, MissingFileThrowsFileError)
{
    ProjectReader parser;
    EXPECT_THROW(parser.load(fixturePath("does-not-exist.qft")),
                 qftbx::FileError);
}

TEST(ProjectReaderErrors, MalformedFileThrowsParseErrorWithLine)
{
    ProjectReader parser;
    try {
        parser.load(fixturePath("invalid.qft"));
        FAIL() << "expected qftbx::ParseError";
    } catch (const qftbx::ParseError &e) {
        EXPECT_GT(e.line(), 0);
        EXPECT_NE(std::string(e.what()).find("line"), std::string::npos);
    }
}

TEST(ProjectReaderErrors, CorruptNumericValuesThrowParseError)
{
    // A numeric list with garbage is rejected with a located error (the
    // historical loader silently kept the prefix before the bad token).
    ProjectReader parser;
    EXPECT_THROW(parser.load(fixturePath("corrupt_omega.qft")),
                 qftbx::ParseError);
}

} // namespace
