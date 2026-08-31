// Parity tests for the new pugixml-based ProjectReader against the
// historical QXmlStream loader, on every shipped fixture: same sections,
// same plants (numerically identical evaluations), same specifications,
// templates, boundaries and loop-shaping data.

#include <gtest/gtest.h>

#include <complex>

#include <QString>
#include <QVector>

#include "Modelo/Herramientas/exception.h"
#include "Modelo/Objetos/omega.h"
#include "XmlParser/parserload.h"
#include "src/persistence/project_reader.h"

namespace {

using Complex = std::complex<qreal>;

QString fixture(const char* name)
{
    return QStringLiteral(QFTBX_TEST_DATA_DIR "/") + QLatin1String(name);
}

void expectSameSystem(LtiSystem* oldSystem, LtiSystem* newSystem,
                      const QVector<qreal>& probes, const char* what)
{
    ASSERT_EQ(oldSystem == nullptr, newSystem == nullptr) << what;
    if (oldSystem == nullptr) {
        return;
    }
    EXPECT_EQ(oldSystem->type(), newSystem->type()) << what;
    EXPECT_EQ(oldSystem->name(), newSystem->name()) << what;
    EXPECT_EQ(oldSystem->numerator()->size(), newSystem->numerator()->size()) << what;
    EXPECT_EQ(oldSystem->denominator()->size(), newSystem->denominator()->size()) << what;

    for (qreal w : probes) {
        const Complex a = oldSystem->evaluate(w);
        const Complex b = newSystem->evaluate(w);
        EXPECT_EQ(a, b) << what << " at w=" << w;
    }
}

class ReaderParity : public ::testing::TestWithParam<const char*>
{
protected:
    void SetUp() override
    {
        oldFlags = oldParser.recuperarXmlDatos(fixture(GetParam()));
        newFlags = reader.load(fixture(GetParam()));
    }

    XmlParserLoad oldParser;
    ProjectReader reader;
    QVector<bool>* oldFlags = nullptr;
    QVector<bool>* newFlags = nullptr;
};

TEST_P(ReaderParity, SectionFlagsMatch)
{
    ASSERT_EQ(newFlags->size(), 8);
    ASSERT_EQ(oldFlags->size(), 8);
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(newFlags->at(i), oldFlags->at(i)) << "flag " << i;
    }
}

TEST_P(ReaderParity, PlantAndOmegaMatch)
{
    QVector<qreal> probes{0.5, 1.0, 7.3};
    if (oldFlags->at(2)) {
        ASSERT_NE(reader.omega(), nullptr);
        EXPECT_EQ(*oldParser.getOmega()->getValores(), *reader.omega()->getValores());
        EXPECT_EQ(oldParser.getOmega()->getInicio(), reader.omega()->getInicio());
        EXPECT_EQ(oldParser.getOmega()->getFinal(), reader.omega()->getFinal());
        EXPECT_EQ(oldParser.getOmega()->getNPuntos(), reader.omega()->getNPuntos());
        EXPECT_EQ(oldParser.getOmega()->getTipo(), reader.omega()->getTipo());
        probes = *reader.omega()->getValores();
    }

    if (oldFlags->at(0)) {
        expectSameSystem(oldParser.getPlanta(), reader.plant(), probes, "plant");
    }

    if (oldFlags->at(5)) {
        expectSameSystem(oldParser.getControlador(), reader.controller(), probes, "controller");
    }
}

TEST_P(ReaderParity, SpecificationsMatch)
{
    if (!oldFlags->at(1)) {
        return;
    }

    auto* oldSpecs = oldParser.getEspecificaciones();
    auto* newSpecs = reader.specifications();
    ASSERT_NE(newSpecs, nullptr);
    ASSERT_EQ(oldSpecs->size(), 7);
    ASSERT_EQ(newSpecs->size(), 7);

    for (int i = 0; i < 7; ++i) {
        tools::dBND* a = oldSpecs->at(i);
        tools::dBND* b = newSpecs->at(i);
        EXPECT_EQ(a->nombre, b->nombre) << "spec " << i;
        EXPECT_EQ(a->utilizado, b->utilizado) << "spec " << i;
        EXPECT_EQ(a->constante, b->constante) << "spec " << i;
        EXPECT_EQ(a->altura, b->altura) << "spec " << i;
        EXPECT_EQ(a->frecinicio, b->frecinicio) << "spec " << i;
        EXPECT_EQ(a->frecfinal, b->frecfinal) << "spec " << i;
        expectSameSystem(a->sistema, b->sistema, {a->frecinicio, a->frecfinal}, "spec plant");
    }
}

TEST_P(ReaderParity, TemplatesMatch)
{
    if (!oldFlags->at(3)) {
        return;
    }

    EXPECT_EQ(*oldParser.getEpsilon(), *reader.epsilon());

    auto* oldFull = oldParser.getTemplates();
    auto* newFull = reader.templates();
    ASSERT_NE(newFull, nullptr);
    ASSERT_EQ(oldFull->size(), newFull->size());
    for (int i = 0; i < oldFull->size(); ++i) {
        ASSERT_EQ(*oldFull->at(i), *newFull->at(i)) << "template " << i;
    }

    if (oldFlags->at(7)) {
        auto* oldContour = oldParser.getContorno();
        auto* newContour = reader.contour();
        ASSERT_NE(newContour, nullptr);
        ASSERT_EQ(oldContour->size(), newContour->size());
        for (int i = 0; i < oldContour->size(); ++i) {
            ASSERT_EQ(*oldContour->at(i), *newContour->at(i)) << "contour " << i;
        }
    }
}

TEST_P(ReaderParity, BoundariesMatch)
{
    if (!oldFlags->at(4)) {
        return;
    }

    BoundaryData* a = oldParser.getBoundaries();
    BoundaryData* b = reader.boundaries();
    ASSERT_NE(b, nullptr);

    EXPECT_EQ(a->phaseCount(), b->phaseCount());
    EXPECT_EQ(a->magnitudeCount(), b->magnitudeCount());
    EXPECT_EQ(a->phaseRange(), b->phaseRange());
    EXPECT_EQ(a->magnitudeRange(), b->magnitudeRange());
    EXPECT_EQ(*a->openFlags(), *b->openFlags());
    EXPECT_EQ(*a->upperFlags(), *b->upperFlags());

    ASSERT_EQ(a->boundaries()->size(), b->boundaries()->size());
    for (int f = 0; f < a->boundaries()->size(); ++f) {
        auto* mapA = a->boundaries()->at(f);
        auto* mapB = b->boundaries()->at(f);
        ASSERT_EQ(mapA->keys(), mapB->keys()) << "frequency " << f;
        foreach (const QString& key, mapA->keys()) {
            auto* tracesA = mapA->value(key);
            auto* tracesB = mapB->value(key);
            ASSERT_EQ(tracesA->size(), tracesB->size()) << "frequency " << f;
            for (int t = 0; t < tracesA->size(); ++t) {
                ASSERT_EQ(*tracesA->at(t), *tracesB->at(t))
                    << "frequency " << f << " trace " << t;
            }
        }
    }

    ASSERT_EQ(a->unionBoundaries()->size(), b->unionBoundaries()->size());
    for (int f = 0; f < a->unionBoundaries()->size(); ++f) {
        ASSERT_EQ(*a->unionBoundaries()->at(f), *b->unionBoundaries()->at(f)) << "union " << f;
    }

    ASSERT_EQ(a->unionBuckets()->size(), b->unionBuckets()->size());
    for (int f = 0; f < a->unionBuckets()->size(); ++f) {
        auto* bucketsA = a->unionBuckets()->at(f);
        auto* bucketsB = b->unionBuckets()->at(f);
        ASSERT_EQ(bucketsA->size(), bucketsB->size()) << "buckets " << f;
        for (int k = 0; k < bucketsA->size(); ++k) {
            ASSERT_EQ(*bucketsA->at(k), *bucketsB->at(k)) << "bucket " << f << "," << k;
        }
    }
}

TEST_P(ReaderParity, LoopShapingMatches)
{
    if (!oldFlags->at(6)) {
        return;
    }

    DatosLoopShaping* a = oldParser.getLoopShaping();
    DatosLoopShaping* b = reader.loopShaping();
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(a->getNPuntos(), b->getNPuntos());
    EXPECT_EQ(a->range(), b->range());
    expectSameSystem(a->getControlador(), b->getControlador(), {0.5, 2.0}, "loop shaping");
}

INSTANTIATE_TEST_SUITE_P(Fixtures, ReaderParity,
                         ::testing::Values("cervera.qft", "planta2.qft",
                                           "multivaluados.qft", "planta1.qft"),
                         [](const ::testing::TestParamInfo<const char*>& info) {
                             QString name = QLatin1String(info.param);
                             name.chop(4);
                             return name.toStdString();
                         });

TEST(ProjectReaderErrors, WrongRootAndCorruptContentThrowParseError)
{
    {
        ProjectReader reader;
        EXPECT_THROW(delete reader.load(fixture("invalid.qft")), qftbx::ParseError);
    }
    {
        ProjectReader reader;
        EXPECT_THROW(delete reader.load(fixture("corrupt_omega.qft")), qftbx::ParseError);
    }
    {
        ProjectReader reader;
        EXPECT_THROW(delete reader.load(fixture("corrupt_specs.qft")), qftbx::ParseError);
    }
    {
        ProjectReader reader;
        EXPECT_THROW(delete reader.load(QStringLiteral("/does/not/exist.qft")), qftbx::FileError);
    }
}

TEST(ProjectReaderErrors, ParseErrorCarriesTheLine)
{
    ProjectReader reader;
    try {
        delete reader.load(fixture("corrupt_omega.qft"));
        FAIL() << "expected ParseError";
    } catch (const qftbx::ParseError& e) {
        EXPECT_GT(e.line(), 0);
    }
}

} // namespace
