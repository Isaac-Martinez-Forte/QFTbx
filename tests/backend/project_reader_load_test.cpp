/**
 * @file
 * @brief Smoke tests of the project reader over the sample .qft files.
 *
 * Each fixture stops at a different point of the design: a plant and its
 * frequencies, up to the templates and contour, up to the boundaries and
 * controller structure, and a finished design. The reader must report exactly
 * the steps the file carries, recover each section, and forget everything a
 * previous file carried when a second one is loaded. A missing file is a file
 * error; malformed XML and a corrupt numeric list are parse errors that name
 * their line.
 */

#include <gtest/gtest.h>

#include <string>

#include <vector>

#include "src/persistence/project_reader.h"
#include "src/core/system/lti_system.h"
#include "src/core/common/exception.h"
#include "src/core/frequencies/omega.h"

using namespace qftbx;

namespace {

std::string fixturePath(const char *name)
{
    return std::string(QFTBX_TEST_DATA_DIR "/") + name;
}

ProjectReader::Loaded loadSections(ProjectReader &parser, const char *fixture)
{
    return parser.load(fixturePath(fixture));
}

TEST(ProjectReaderSmoke, CerveraLoadsPlantAndFrequenciesOnly)
{
    ProjectReader parser;
    const ProjectReader::Loaded loaded = loadSections(parser, "cervera.qft");

    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Plant));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Frequencies));
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::Specifications));
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::Templates));
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::Boundaries));
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::Controller));
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::LoopShaping));
    EXPECT_FALSE(loaded.hasContour);

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
    const ProjectReader::Loaded loaded = loadSections(parser, "planta2.qft");

    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Plant));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Specifications));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Frequencies));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Templates));
    EXPECT_TRUE(loaded.hasContour);
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::Boundaries));
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::Controller));
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::LoopShaping));

    ASSERT_FALSE(parser.templates().empty());
    EXPECT_EQ(static_cast<int>(parser.templates().size()), 6);
    ASSERT_FALSE(parser.contour().empty());
    EXPECT_EQ(static_cast<int>(parser.contour().size()), 6);
}

TEST(ProjectReaderSmoke, MultivaluadosLoadsUpToBoundaries)
{
    ProjectReader parser;
    const ProjectReader::Loaded loaded = loadSections(parser, "multivaluados.qft");

    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Plant));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Specifications));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Frequencies));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Templates));
    EXPECT_TRUE(loaded.hasContour);
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Boundaries));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Controller));
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::LoopShaping));

    ASSERT_FALSE(parser.templates().empty());
    EXPECT_EQ(static_cast<int>(parser.templates().size()), 5);
    EXPECT_NE(parser.boundaries(), nullptr);
}

TEST(ProjectReaderSmoke, Planta1LoadsFullProject)
{
    ProjectReader parser;
    const ProjectReader::Loaded loaded = loadSections(parser, "planta1.qft");

    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Plant));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Specifications));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Frequencies));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Templates));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Boundaries));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Controller));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::LoopShaping));
    EXPECT_TRUE(loaded.hasContour);

    ASSERT_FALSE(parser.templates().empty());
    EXPECT_EQ(static_cast<int>(parser.templates().size()), 4);
    EXPECT_NE(parser.boundaries(), nullptr);
}

TEST(ProjectReaderSmoke, ASecondLoadDropsWhatTheFirstFileCarried)
{
    ProjectReader parser;
    loadSections(parser, "planta1.qft");
    ASSERT_FALSE(parser.templates().empty());

    const ProjectReader::Loaded loaded = loadSections(parser, "cervera.qft");

    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Plant));
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::Templates));
    EXPECT_EQ(parser.specifications(), nullptr);
    EXPECT_TRUE(parser.templates().empty());
    EXPECT_TRUE(parser.contour().empty());
    EXPECT_EQ(parser.epsilon(), nullptr);
    EXPECT_EQ(parser.boundaries(), nullptr);
    EXPECT_EQ(parser.controller(), nullptr);
    EXPECT_EQ(parser.loopShaping(), nullptr);
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
    ProjectReader parser;
    EXPECT_THROW(parser.load(fixturePath("corrupt_omega.qft")),
                 qftbx::ParseError);
}

}
