// Round-trip tests for the version-2 English dialect: every legacy fixture
// is loaded, written back as v2 and reloaded, and the reloaded project must
// be numerically identical (the writer keeps 17 significant digits, so the
// trip is bit-exact; the historical writer kept 6 and degraded every save).

#include "src/core/system/polynomial_form.h"
#include <gtest/gtest.h>

#include <string>

#include <vector>

#include <QTemporaryDir>

#include <pugixml.hpp>

#include "project_compare.h"
#include "src/persistence/project_reader.h"
#include "src/persistence/project_writer.h"

namespace {

using namespace qftbx_tests;

std::string fixture(const char* name)
{
    return std::string(QFTBX_TEST_DATA_DIR "/") + name;
}

class RoundTrip : public ::testing::TestWithParam<const char*>
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE(temporary.isValid());
        rewritten = temporary.filePath("roundtrip.qft").toStdString();

        originalSections = original.load(fixture(GetParam()));

        ProjectContent content;
        content.plant = original.plant();
        content.specifications = original.specifications();
        content.omega = original.omega();
        content.templates = &original.templates();
        content.contour = &original.contour();
        content.epsilon = original.epsilon();
        content.boundaries = original.boundaries();
        content.controller = original.controller();
        content.loopShaping = original.loopShaping();

        ProjectWriter writer;
        writer.save(rewritten, content);

        reloadedSections = reloaded.load(rewritten);
    }

    QTemporaryDir temporary;
    std::string rewritten;
    ProjectReader original;
    ProjectReader reloaded;
    ProjectReader::Loaded originalSections;
    ProjectReader::Loaded reloadedSections;
};

TEST_P(RoundTrip, WritesTheVersionedEnglishDialect)
{
    pugi::xml_document document;
    ASSERT_TRUE(document.load_file(rewritten.c_str()));

    const pugi::xml_node root = document.document_element();
    EXPECT_STREQ(root.name(), "QFT");
    EXPECT_EQ(root.attribute("version").as_int(), 2);

    // No legacy Spanish tags anywhere in a v2 file.
    EXPECT_FALSE(root.child("Planta"));
    EXPECT_FALSE(root.child("especificaciones"));
    if (originalSections.steps.has(qftbx::Step::Plant)) {
        EXPECT_TRUE(root.child("plant"));
    }
    if (originalSections.steps.has(qftbx::Step::Specifications)) {
        EXPECT_TRUE(root.child("specifications"));
    }
}

TEST_P(RoundTrip, SectionFlagsSurvive)
{
    //The set compares as a whole, which is what a typed set buys over eight
    //positions looped over by index.
    EXPECT_EQ(reloadedSections.steps, originalSections.steps);
    EXPECT_EQ(reloadedSections.hasContour, originalSections.hasContour);
}

TEST_P(RoundTrip, EverySectionSurvivesBitExact)
{
    std::vector<double> probes{0.5, 1.0, 7.3};
    if (originalSections.steps.has(qftbx::Step::Frequencies)) {
        EXPECT_EQ(*original.omega()->values(), *reloaded.omega()->values());
        EXPECT_EQ(original.omega()->start(), reloaded.omega()->start());
        EXPECT_EQ(original.omega()->end(), reloaded.omega()->end());
        EXPECT_EQ(original.omega()->pointCount(), reloaded.omega()->pointCount());
        EXPECT_EQ(original.omega()->type(), reloaded.omega()->type());
        probes = *original.omega()->values();
    }

    if (originalSections.steps.has(qftbx::Step::Plant)) {
        expectSameSystem(original.plant(), reloaded.plant(), probes, "plant");
    }
    if (originalSections.steps.has(qftbx::Step::Specifications)) {
        expectSameSpecifications(original.specifications(), reloaded.specifications());
    }
    if (originalSections.steps.has(qftbx::Step::Templates)) {
        EXPECT_EQ(*original.epsilon(), *reloaded.epsilon());
        expectSameComplexVectors(original.templates(), reloaded.templates(), "templates");
    }
    if (originalSections.hasContour) {
        expectSameComplexVectors(original.contour(), reloaded.contour(), "contour");
    }
    if (originalSections.steps.has(qftbx::Step::Boundaries)) {
        expectSameBoundaries(original.boundaries(), reloaded.boundaries());
    }
    if (originalSections.steps.has(qftbx::Step::Controller)) {
        expectSameSystem(original.controller(), reloaded.controller(), probes, "controller");
    }
    if (originalSections.steps.has(qftbx::Step::LoopShaping)) {
        expectSameLoopShaping(original.loopShaping(), reloaded.loopShaping());
    }
}

INSTANTIATE_TEST_SUITE_P(Fixtures, RoundTrip,
                         ::testing::Values("cervera.qft", "planta2.qft",
                                           "multivaluados.qft", "planta1.qft"),
                         [](const ::testing::TestParamInfo<const char*>& info) {
                             std::string name = info.param;
                             name.resize(name.size() - 4);   //drop the ".qft"
                             return name;
                         });

TEST(ProjectWriterErrors, UnwritablePathThrowsFileError)
{
    ProjectWriter writer;
    ProjectContent empty;
    EXPECT_THROW(writer.save(std::string("/does/not/exist/out.qft"), empty),
                 qftbx::FileError);
}

} // namespace

TEST(RoundTripReparametrised, AReparametrisedParameterSurvivesSaveAndLoad)
{
    // A parameter with a reparametrisation expression: its RAW range is
    // [1, 2] and its expression maps that to [10, 20]. The writer used to
    // store the TRANSFORMED values while the reader took what it found as the
    // raw ones, so one save-and-load applied the expression twice and the
    // parameter came back as [100, 200]. No fixture carries a
    // reparametrisation, which is how it stayed unnoticed.
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{
        Parameter(std::string("a"), qftbx::Range(1.0, 2.0), 1.5, std::string("a*10")),
        Parameter(1.0)};

    auto plant = std::make_unique<PolynomialForm>(
        std::string("P"), numerator, denominator,
        Parameter(std::string("kv"), qftbx::Range(1.0, 2.0), 1.5),
        Parameter(0.0));

    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const std::string path = temporary.filePath("reparametrised.qft").toStdString();

    ProjectContent content;
    content.plant = plant.get();
    ProjectWriter writer;
    writer.save(path, content);

    ProjectReader reader;
    reader.load(path);
    ASSERT_NE(reader.plant(), nullptr);

    Parameter & reloaded = reader.plant()->denominator().at(0);
    EXPECT_EQ(reloaded.expression(), "a*10");
    EXPECT_EQ(reloaded.rawRange().min, 1.0) << "the raw range must come back raw";
    EXPECT_EQ(reloaded.rawRange().max, 2.0);
    EXPECT_EQ(reloaded.rawNominal(), 1.5);
    EXPECT_EQ(reloaded.range().min, 10.0) << "and the expression applied once, not twice";
    EXPECT_EQ(reloaded.range().max, 20.0);
    EXPECT_EQ(reloaded.nominal(), 15.0);
}
