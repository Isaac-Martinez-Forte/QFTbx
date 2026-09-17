// Round-trip tests for the version-2 English dialect: every legacy fixture
// is loaded, written back as v2 and reloaded, and the reloaded project must
// be numerically identical (the writer keeps 17 significant digits, so the
// trip is bit-exact; the historical writer kept 6 and degraded every save).

#include "src/core/system/polynomial_form.h"
#include <gtest/gtest.h>

#include <limits>

#include <string>

#include <vector>

#include <QTemporaryDir>

#include <pugixml.hpp>

#include "tests/backend/project_compare.h"
#include "src/persistence/project_reader.h"
#include "src/persistence/project_writer.h"

using namespace qftbx;

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
    EXPECT_EQ(root.attribute("version").as_int(), 4);

    // No legacy Spanish tags anywhere in a written file.
    EXPECT_FALSE(root.child("Planta"));
    EXPECT_FALSE(root.child("especificaciones"));

    //What the user described is under <inputs> and what came out under
    //<results>, so that the file reads down the page. The controller
    //STRUCTURE is an input: it is the box the search is asked to look in.
    const pugi::xml_node inputs = root.child("inputs");
    ASSERT_TRUE(inputs) << "a written file has no <inputs>";
    if (originalSections.steps.has(qftbx::Step::Plant)) {
        EXPECT_TRUE(inputs.child("plant"));
        EXPECT_FALSE(root.child("plant")) << "the plant is still at the root";
    }
    if (originalSections.steps.has(qftbx::Step::Specifications)) {
        EXPECT_TRUE(inputs.child("specifications"));
    }
    if (originalSections.steps.has(qftbx::Step::Controller)) {
        EXPECT_TRUE(inputs.child("controller")) << "the structure to search is an input";
    }

    if (originalSections.steps.has(qftbx::Step::Templates)) {
        const pugi::xml_node results = root.child("results");
        ASSERT_TRUE(results) << "a file with templates has no <results>";
        EXPECT_TRUE(results.child("templates"));
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

TEST(ProjectWriterErrors, ANonFiniteValueIsRefusedInsteadOfWritten)
{
    //A NaN used to go to the file as "nan" and come back through strtod as a
    //NaN: a project that had gone wrong in memory reloaded as if it had not.
    qftbx::SpecificationRecords records;
    qftbx::SpecificationRecord & stability = records.at(2);
    stability.name = "Stability";
    stability.used = true;
    stability.constant = true;
    stability.omegaStart = 0.1;
    stability.omegaEnd = 10.0;
    stability.height = std::numeric_limits<double>::quiet_NaN();

    ProjectContent content;
    content.specifications = &records;

    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());

    ProjectWriter writer;
    EXPECT_THROW(writer.save(temporary.filePath("nan.qft").toStdString(), content),
                 qftbx::InvalidInput);
}

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

// What the search was run with, and what the verifier said about what came
// out of it. Two designs for the same problem differ by hundreds of units of
// gain depending on how the phase grid was read, so a file that keeps the
// controller and forgets the reading keeps a number nobody can reproduce.
// A specification that skips a frequency of its band can only say so
// through the exception list, so the file has to carry it.
TEST(RoundTripSettings, TheFrequenciesASpecificationSkipsSurviveSaveAndLoad)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const std::string path = temporary.filePath("skipped.qft").toStdString();

    ProjectReader original;
    original.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_NE(original.specifications(), nullptr);

    //The first slot the file uses, taken out of one frequency. The records
    //are read-only through the reader, so they are copied, edited and
    //written from the copy - which is what the window does too.
    qftbx::SpecificationRecords edited;
    std::size_t slot = qftbx::kSpecificationCount;
    for (std::size_t i = 0; i < qftbx::kSpecificationCount; ++i) {
        edited.at(i) = original.specifications()->at(i).clone();
        if (slot == qftbx::kSpecificationCount && edited.at(i).used) {
            slot = i;
        }
    }
    ASSERT_LT(slot, qftbx::kSpecificationCount);

    const double hole = (edited.at(slot).omegaStart + edited.at(slot).omegaEnd) / 2.0;
    edited.at(slot).skipped.push_back(hole);

    ProjectContent content;
    content.specifications = &edited;

    ProjectWriter writer;
    writer.save(path, content);

    ProjectReader reloaded;
    reloaded.load(path);
    ASSERT_NE(reloaded.specifications(), nullptr);

    const qftbx::SpecificationRecord & back = reloaded.specifications()->at(slot);
    ASSERT_EQ(back.skipped.size(), 1u);
    EXPECT_DOUBLE_EQ(back.skipped.front(), hole);

    //And the specification the engines see skips it.
    const qftbx::Specification specification =
            qftbx::toSpecification(back, qftbx::SpecificationType::TrackingLower);
    EXPECT_FALSE(specification.appliesAt(hole));
    EXPECT_TRUE(specification.appliesAt(back.omegaStart));
}

// The description of a plant is text the user wrote and nothing derives it:
// if the file does not carry it, it is gone. A file written before it
// existed simply has none, which is what an empty description is.
TEST(RoundTripSettings, TheDescriptionOfThePlantSurvivesSaveAndLoad)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const std::string path = temporary.filePath("described.qft").toStdString();

    ProjectReader original;
    original.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_NE(original.plant(), nullptr);

    EXPECT_TRUE(original.plant()->description().empty())
        << "a file written before the description existed has none";

    original.plant()->setDescription("The one of the 2007 paper, inertia uncertain");

    ProjectContent content;
    content.plant = original.plant();

    ProjectWriter writer;
    writer.save(path, content);

    ProjectReader reloaded;
    reloaded.load(path);
    ASSERT_NE(reloaded.plant(), nullptr);
    EXPECT_EQ(reloaded.plant()->description(),
              "The one of the 2007 paper, inertia uncertain");

    //And it is not part of what tells one plant from another: a project
    //whose description changed keeps its templates.
    EXPECT_TRUE(reloaded.plant()->sameAs(*original.plant()));
    reloaded.plant()->setDescription("something else entirely");
    EXPECT_TRUE(reloaded.plant()->sameAs(*original.plant()))
        << "a description is not what makes a plant a different plant";
}

TEST(RoundTripSettings, TheRunAndTheVerdictSurviveSaveAndLoad)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const std::string path = temporary.filePath("settings.qft").toStdString();

    ProjectReader original;
    original.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_NE(original.loopShaping(), nullptr);

    original.loopShaping()->setRun({qftbx::mc2, 0.05, true});
    SpecificationCheck check;
    check.worstExcessDb = -1.25;
    original.loopShaping()->setCheck(check);

    ProjectContent content;
    content.plant = original.plant();
    content.omega = original.omega();
    content.templates = &original.templates();
    content.epsilon = original.epsilon();
    content.controller = original.controller();
    content.loopShaping = original.loopShaping();

    ProjectWriter writer;
    writer.save(path, content);

    ProjectReader reloaded;
    reloaded.load(path);
    ASSERT_NE(reloaded.loopShaping(), nullptr);

    EXPECT_EQ(reloaded.loopShaping()->run().algorithm, qftbx::mc2);
    EXPECT_DOUBLE_EQ(reloaded.loopShaping()->run().epsilon, 0.05);
    EXPECT_TRUE(reloaded.loopShaping()->run().conservativeColumns);

    ASSERT_TRUE(reloaded.loopShaping()->check().has_value());
    EXPECT_TRUE(reloaded.loopShaping()->check()->satisfied());
    EXPECT_DOUBLE_EQ(reloaded.loopShaping()->check()->worstExcessDb, -1.25);
}

// A verdict with nothing active to exceed is minus infinity, and the file
// carries no infinities: the verdict survives without the number.
TEST(RoundTripSettings, AVerdictWithNoActiveSpecificationIsStillWritten)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const std::string path = temporary.filePath("verdict.qft").toStdString();

    ProjectReader original;
    original.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_NE(original.loopShaping(), nullptr);
    original.loopShaping()->setCheck(SpecificationCheck{});

    ProjectContent content;
    content.controller = original.controller();
    content.loopShaping = original.loopShaping();

    ProjectWriter writer;
    writer.save(path, content);

    ProjectReader reloaded;
    reloaded.load(path);
    ASSERT_NE(reloaded.loopShaping(), nullptr);
    ASSERT_TRUE(reloaded.loopShaping()->check().has_value());
    EXPECT_TRUE(reloaded.loopShaping()->check()->satisfied());
    EXPECT_FALSE(std::isfinite(reloaded.loopShaping()->check()->worstExcessDb));
}
