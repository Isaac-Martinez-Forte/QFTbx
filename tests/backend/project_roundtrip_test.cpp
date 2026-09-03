// Round-trip tests for the version-2 English dialect: every legacy fixture
// is loaded, written back as v2 and reloaded, and the reloaded project must
// be numerically identical (the writer keeps 17 significant digits, so the
// trip is bit-exact; the historical writer kept 6 and degraded every save).

#include <gtest/gtest.h>

#include <vector>

#include <QString>
#include <QTemporaryDir>
#include <QVector>

#include <pugixml.hpp>

#include "project_compare.h"
#include "src/persistence/project_reader.h"
#include "src/persistence/project_writer.h"

namespace {

using namespace qftbx_tests;

QString fixture(const char* name)
{
    return QStringLiteral(QFTBX_TEST_DATA_DIR "/") + QLatin1String(name);
}

class RoundTrip : public ::testing::TestWithParam<const char*>
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE(temporary.isValid());
        rewritten = temporary.filePath(QStringLiteral("roundtrip.qft"));

        originalFlags = original.load(fixture(GetParam()));

        ProjectContent content;
        content.plant = original.plant();
        content.specifications = original.specifications();
        content.omega = original.omega();
        content.templates = original.templates();
        content.contour = original.contour();
        content.epsilon = original.epsilon();
        content.boundaries = const_cast<BoundaryData *>(original.boundaries());
        content.controller = original.controller();
        content.loopShaping = original.loopShaping();

        ProjectWriter writer;
        writer.save(rewritten, content);

        reloadedFlags = reloaded.load(rewritten);
    }

    QTemporaryDir temporary;
    QString rewritten;
    ProjectReader original;
    ProjectReader reloaded;
    std::vector<bool> originalFlags;
    std::vector<bool> reloadedFlags;
};

TEST_P(RoundTrip, WritesTheVersionedEnglishDialect)
{
    pugi::xml_document document;
    ASSERT_TRUE(document.load_file(rewritten.toUtf8().constData()));

    const pugi::xml_node root = document.document_element();
    EXPECT_STREQ(root.name(), "QFT");
    EXPECT_EQ(root.attribute("version").as_int(), 2);

    // No legacy Spanish tags anywhere in a v2 file.
    EXPECT_FALSE(root.child("Planta"));
    EXPECT_FALSE(root.child("especificaciones"));
    if (originalFlags.at(0)) {
        EXPECT_TRUE(root.child("plant"));
    }
    if (originalFlags.at(1)) {
        EXPECT_TRUE(root.child("specifications"));
    }
}

TEST_P(RoundTrip, SectionFlagsSurvive)
{
    ASSERT_EQ(static_cast<int>(reloadedFlags.size()), 8);
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(reloadedFlags.at(i), originalFlags.at(i)) << "flag " << i;
    }
}

TEST_P(RoundTrip, EverySectionSurvivesBitExact)
{
    std::vector<double> probes{0.5, 1.0, 7.3};
    if (originalFlags.at(2)) {
        EXPECT_EQ(*original.omega()->values(), *reloaded.omega()->values());
        EXPECT_EQ(original.omega()->start(), reloaded.omega()->start());
        EXPECT_EQ(original.omega()->end(), reloaded.omega()->end());
        EXPECT_EQ(original.omega()->pointCount(), reloaded.omega()->pointCount());
        EXPECT_EQ(original.omega()->type(), reloaded.omega()->type());
        probes = *original.omega()->values();
    }

    if (originalFlags.at(0)) {
        expectSameSystem(original.plant(), reloaded.plant(), probes, "plant");
    }
    if (originalFlags.at(1)) {
        expectSameSpecifications(original.specifications(), reloaded.specifications());
    }
    if (originalFlags.at(3)) {
        EXPECT_EQ(*original.epsilon(), *reloaded.epsilon());
        expectSameComplexVectors(original.templates(), reloaded.templates(), "templates");
    }
    if (originalFlags.at(7)) {
        expectSameComplexVectors(original.contour(), reloaded.contour(), "contour");
    }
    if (originalFlags.at(4)) {
        expectSameBoundaries(original.boundaries(), reloaded.boundaries());
    }
    if (originalFlags.at(5)) {
        expectSameSystem(original.controller(), reloaded.controller(), probes, "controller");
    }
    if (originalFlags.at(6)) {
        expectSameLoopShaping(original.loopShaping(), reloaded.loopShaping());
    }
}

INSTANTIATE_TEST_SUITE_P(Fixtures, RoundTrip,
                         ::testing::Values("cervera.qft", "planta2.qft",
                                           "multivaluados.qft", "planta1.qft"),
                         [](const ::testing::TestParamInfo<const char*>& info) {
                             QString name = QLatin1String(info.param);
                             name.chop(4);
                             return name.toStdString();
                         });

TEST(ProjectWriterErrors, UnwritablePathThrowsFileError)
{
    ProjectWriter writer;
    ProjectContent empty;
    EXPECT_THROW(writer.save(QStringLiteral("/does/not/exist/out.qft"), empty),
                 qftbx::FileError);
}

} // namespace
