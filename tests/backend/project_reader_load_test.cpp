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

using namespace qftbx;

namespace {

std::string fixturePath(const char *name)
{
    return std::string(QFTBX_TEST_DATA_DIR "/") + name;
}

//ProjectReader::Loaded says what the file carried, by name. It used to be an
//eight-element std::vector<bool> read through an enum of indices declared
//here, seven of whose entries were steps and the eighth something else
//entirely.
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

    // One template (full cloud + contour) per design frequency.
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

    //Every step, and the contour with them: this fixture is a finished
    //design. Named one by one rather than looped over indices, which is the
    //point of the steps having names.
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

    //cervera.qft carries a plant and frequencies only: nothing of the
    //finished design read before it may show through.
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
    // A numeric list with garbage is rejected with a located error (the
    // historical loader silently kept the prefix before the bad token).
    ProjectReader parser;
    EXPECT_THROW(parser.load(fixturePath("corrupt_omega.qft")),
                 qftbx::ParseError);
}

} // namespace
