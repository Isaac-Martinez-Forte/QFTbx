// Smoke tests for XmlParserLoad: load the sample .qft project files shipped
// as test data and check that every section present in the file is recognised
// and recovered.
//
// These tests pin the current behaviour of the loader so that the ongoing
// backend refactor can be validated against it.

#include <gtest/gtest.h>

#include <QString>
#include <QVector>

#include "XmlParser/parserload.h"
#include "Modelo/EstructuraSistema/sistema.h"
#include "Modelo/Objetos/omega.h"

namespace {

// Indices of the flag vector returned by XmlParserLoad::recuperarXmlDatos().
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

QString fixturePath(const char *name)
{
    return QString(QFTBX_TEST_DATA_DIR "/") + name;
}

QVector<bool> loadFlags(XmlParserLoad &parser, const char *fixture)
{
    QVector<bool> *flags = parser.recuperarXmlDatos(fixturePath(fixture));
    if (flags == nullptr) {
        ADD_FAILURE() << "recuperarXmlDatos returned null for " << fixture;
        return {};
    }
    QVector<bool> copy = *flags;
    EXPECT_EQ(copy.size(), kSectionFlagCount) << "unexpected flag count";
    return copy;
}

TEST(XmlParserLoadSmoke, CerveraLoadsPlantAndFrequenciesOnly)
{
    XmlParserLoad parser;
    const QVector<bool> flags = loadFlags(parser, "cervera.qft");
    ASSERT_EQ(flags.size(), kSectionFlagCount);

    EXPECT_TRUE(flags[kPlant]);
    EXPECT_TRUE(flags[kOmega]);
    EXPECT_FALSE(flags[kSpecifications]);
    EXPECT_FALSE(flags[kTemplates]);
    EXPECT_FALSE(flags[kBoundaries]);
    EXPECT_FALSE(flags[kController]);
    EXPECT_FALSE(flags[kLoopShaping]);
    EXPECT_FALSE(flags[kContour]);

    ASSERT_NE(parser.getPlanta(), nullptr);

    Omega *omega = parser.getOmega();
    ASSERT_NE(omega, nullptr);
    ASSERT_NE(omega->getValores(), nullptr);
    const QVector<qreal> &values = *omega->getValores();
    ASSERT_EQ(values.size(), 4);
    EXPECT_DOUBLE_EQ(values[0], 0.1);
    EXPECT_DOUBLE_EQ(values[1], 5.0);
    EXPECT_DOUBLE_EQ(values[2], 10.0);
    EXPECT_DOUBLE_EQ(values[3], 100.0);
}

TEST(XmlParserLoadSmoke, Planta2LoadsUpToTemplates)
{
    XmlParserLoad parser;
    const QVector<bool> flags = loadFlags(parser, "planta2.qft");
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
    ASSERT_NE(parser.getTemplates(), nullptr);
    EXPECT_EQ(parser.getTemplates()->size(), 6);
    ASSERT_NE(parser.getContorno(), nullptr);
    EXPECT_EQ(parser.getContorno()->size(), 6);
}

TEST(XmlParserLoadSmoke, MultivaluadosLoadsUpToBoundaries)
{
    XmlParserLoad parser;
    const QVector<bool> flags = loadFlags(parser, "multivaluados.qft");
    ASSERT_EQ(flags.size(), kSectionFlagCount);

    EXPECT_TRUE(flags[kPlant]);
    EXPECT_TRUE(flags[kSpecifications]);
    EXPECT_TRUE(flags[kOmega]);
    EXPECT_TRUE(flags[kTemplates]);
    EXPECT_TRUE(flags[kContour]);
    EXPECT_TRUE(flags[kBoundaries]);
    EXPECT_TRUE(flags[kController]);
    EXPECT_FALSE(flags[kLoopShaping]);

    ASSERT_NE(parser.getTemplates(), nullptr);
    EXPECT_EQ(parser.getTemplates()->size(), 5);
    EXPECT_NE(parser.getBoundaries(), nullptr);
}

TEST(XmlParserLoadSmoke, Planta1LoadsFullProject)
{
    XmlParserLoad parser;
    const QVector<bool> flags = loadFlags(parser, "planta1.qft");
    ASSERT_EQ(flags.size(), kSectionFlagCount);

    for (int i = 0; i < kSectionFlagCount; ++i) {
        EXPECT_TRUE(flags[i]) << "section flag " << i << " not recovered";
    }

    ASSERT_NE(parser.getTemplates(), nullptr);
    EXPECT_EQ(parser.getTemplates()->size(), 4);
    EXPECT_NE(parser.getBoundaries(), nullptr);
}

} // namespace
