// Golden tests for the template computation against tests/data/planta2.qft,
// which ships the full clouds and epsilon-hull contours computed by the
// original program (6 frequencies, 10x10 parameter grid, epsilon = 10).
//
// The fixture serialises with 6 significant digits, so comparisons use a
// relative tolerance. The tests run multithreaded: every computation writes
// at the index of its own frequency (the old thread-order permutation and
// its omega/epsilon aliasing repair are gone).

#include <gtest/gtest.h>

#include <complex>

#include <QHash>
#include <QString>
#include <QVector>

#include "Modelo/controlador.h"
#include "Modelo/Herramientas/exception.h"
#include "Modelo/Templates/templates.h"
#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"
#include "Modelo/Objetos/omega.h"
#include "Modelo/Herramientas/tools.h"
#include "XmlParser/parserload.h"

namespace {

using Complex = std::complex<qreal>;

void expectNear(Complex actual, Complex expected, const char* where)
{
    const qreal tolR = std::max(1e-9, 1e-5 * std::abs(expected.real()));
    const qreal tolI = std::max(1e-9, 1e-5 * std::abs(expected.imag()));
    EXPECT_NEAR(actual.real(), expected.real(), tolR) << where;
    EXPECT_NEAR(actual.imag(), expected.imag(), tolI) << where;
}

class TemplatesGolden : public ::testing::Test
{
protected:
    void SetUp() override
    {
        delete parser.recuperarXmlDatos(
            QStringLiteral(QFTBX_TEST_DATA_DIR "/planta2.qft"));
        planta = parser.getPlanta();
        ASSERT_NE(planta, nullptr);

        // The fixture was computed on a 10x10 grid: a and kv in [1,10].
        mapa = new QHash<Parameter*, QVector<qreal>*>();
        mapa->insert(planta->numerator()->at(0), tools::linspace(1.0, 10.0, 10));
        mapa->insert(planta->gain(), tools::linspace(1.0, 10.0, 10));

        omegaCopy = new QVector<qreal>(*parser.getOmega()->getValores());
        epsilon = new QVector<qreal>(6, 10.0);

        templates.setEpsilon(epsilon);
        templates.setMapa(mapa);
        templates.lanzarCalculo(planta, omegaCopy, false);
    }

    void TearDown() override
    {
        // Templates owns nothing it was given; free what we created.
        for (QVector<qreal>* v : *mapa) {
            delete v;
        }
        delete mapa;
        delete omegaCopy;
        delete epsilon;
    }

    XmlParserLoad parser;
    LtiSystem* planta = nullptr;
    QHash<Parameter*, QVector<qreal>*>* mapa = nullptr;
    QVector<qreal>* omegaCopy = nullptr;
    QVector<qreal>* epsilon = nullptr;
    Templates templates;
};

TEST_F(TemplatesGolden, BruteForceMatchesFixture)
{
    auto* computed = templates.getTemplates();
    auto* expected = parser.getTemplates();
    ASSERT_NE(computed, nullptr);
    ASSERT_NE(expected, nullptr);
    ASSERT_EQ(computed->size(), expected->size());

    for (int f = 0; f < computed->size(); ++f) {
        ASSERT_EQ(computed->at(f)->size(), expected->at(f)->size())
            << "frequency " << f;
        for (int p = 0; p < computed->at(f)->size(); ++p) {
            expectNear(computed->at(f)->at(p), expected->at(f)->at(p),
                       "template point");
        }
    }

    // Hard anchors fixing the sweep order (a is the fast digit).
    expectNear(computed->at(0)->at(0), Complex(-0.990099, -9.90099), "t[0][0]");
    expectNear(computed->at(0)->at(1), Complex(-0.498753, -9.97506), "t[0][1]");
}

TEST_F(TemplatesGolden, ContourMatchesFixtureAsACycle)
{
    // The faithful EPSHULL.M walk returns the same cycle as the historical
    // golden but closed (last point repeats the first) and rotated (it
    // starts at max real instead of max imaginary), so the comparison is
    // cyclic: same sequence, same direction, any starting point. The
    // fallback frequencies (0 and 2, where the reference walk cycles)
    // reproduce the historical sequence exactly.
    auto* computed = templates.getContorno();
    auto* expected = parser.getContorno();
    ASSERT_NE(computed, nullptr);
    ASSERT_NE(expected, nullptr);
    ASSERT_EQ(computed->size(), expected->size());

    const int expectedSizes[] = {30, 28, 28, 28, 28, 28};
    for (int f = 0; f < computed->size(); ++f) {
        ASSERT_EQ(expected->at(f)->size(), expectedSizes[f]);

        QVector<Complex> cycle = *computed->at(f);
        if (cycle.size() > 1 && cycle.first() == cycle.last()) {
            cycle.removeLast(); // closing duplicate
        }
        ASSERT_EQ(cycle.size(), expected->at(f)->size()) << "frequency " << f;

        // Locate the rotation offset: the computed point closest to the
        // first expected point.
        int offset = 0;
        qreal best = std::abs(cycle.at(0) - expected->at(f)->at(0));
        for (int i = 1; i < cycle.size(); ++i) {
            const qreal d = std::abs(cycle.at(i) - expected->at(f)->at(0));
            if (d < best) {
                best = d;
                offset = i;
            }
        }

        for (int p = 0; p < cycle.size(); ++p) {
            expectNear(cycle.at((offset + p) % cycle.size()),
                       expected->at(f)->at(p), "contour point");
        }
    }
}

TEST_F(TemplatesGolden, ContourStartHonoursTheHybridRule)
{
    // Faithful walks return CLOSED contours (last == first) starting at the
    // max-real point (EPSHULL.M and the PFC text); when the reference walk
    // cycles, the fallback returns the historical open, deduplicated
    // contour starting at the max-imaginary point. WHICH frequencies fall
    // back depends on last-digit noise of the cloud (the reference walk is
    // that sensitive), so the rule is detected per frequency, not fixed.
    auto* temps = templates.getTemplates();
    auto* conts = templates.getContorno();

    int fallbacks = 0;
    for (int f = 0; f < conts->size(); ++f) {
        const QVector<Complex>* c = conts->at(f);
        const bool faithful = c->size() > 1 && c->first() == c->last();
        if (!faithful) {
            ++fallbacks;
        }
        Complex extreme = temps->at(f)->at(0);
        for (const Complex& p : *temps->at(f)) {
            const bool better = faithful ? p.real() > extreme.real()
                                         : p.imag() > extreme.imag();
            if (better) {
                extreme = p;
            }
        }
        EXPECT_EQ(c->at(0), extreme)
            << "frequency " << f << (faithful ? " (faithful)" : " (fallback)");
    }

    // The clustered clouds of this fixture make some frequencies fall back.
    EXPECT_GT(fallbacks, 0);
    EXPECT_LT(fallbacks, conts->size());
}

TEST_F(TemplatesGolden, ContourIsSubsetOfTemplate)
{
    auto* temps = templates.getTemplates();
    auto* conts = templates.getContorno();

    for (int f = 0; f < conts->size(); ++f) {
        for (const Complex& p : *conts->at(f)) {
            EXPECT_TRUE(temps->at(f)->contains(p))
                << "contour point not in template, frequency " << f;
        }
    }
}

TEST_F(TemplatesGolden, FrequencyAlignmentPreserved)
{
    // The i-th contour must correspond to the i-th frequency, with any
    // number of OpenMP threads (the old thread-counter renumbering broke
    // this intermittently).
    const QVector<qreal> original{0.1, 0.5, 1.0, 2.0, 15.0, 100.0};
    auto* omegaOut = templates.getOmega();
    ASSERT_NE(omegaOut, nullptr);
    ASSERT_EQ(omegaOut->size(), original.size());
    for (int i = 0; i < original.size(); ++i) {
        EXPECT_DOUBLE_EQ(omegaOut->at(i), original.at(i)) << "index " << i;
    }
}

TEST_F(TemplatesGolden, InputVectorsSurviveTheComputation)
{
    // Fixed (aliasing): the computation no longer clears or replaces the
    // omega and epsilon vectors it was handed; the caller's data survives.
    ASSERT_EQ(omegaCopy->size(), 6);
    EXPECT_DOUBLE_EQ(omegaCopy->at(0), 0.1);
    EXPECT_DOUBLE_EQ(omegaCopy->at(5), 100.0);
    ASSERT_EQ(epsilon->size(), 6);
    EXPECT_DOUBLE_EQ(epsilon->at(0), 10.0);
}

TEST(TemplatesReload, RecalculateContourAfterLoadingAProject)
{
    // Fixed crash: loading a project fed only the DAO, so recalculating
    // the contour dereferenced a null templates vector inside the engine.
    Controlador controlador;
    delete controlador.cargarSistema(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/planta2.qft"));

    auto* epsilon = new QVector<qreal>(6, 10.0);
    auto* contornos = controlador.recalcularContorno(epsilon);
    ASSERT_NE(contornos, nullptr);
    ASSERT_EQ(contornos->size(), 6);
    for (const QVector<Complex>* c : *contornos) {
        EXPECT_FALSE(c->isEmpty());
    }
}

TEST(TemplatesValidation, MissingSweepGridThrowsInvalidInput)
{
    // Hardened: a map without an entry for some uncertain parameter used to
    // dereference null; now it reports which grid is missing.
    XmlParserLoad parser;
    delete parser.recuperarXmlDatos(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/planta2.qft"));
    LtiSystem* planta = parser.getPlanta();

    auto* mapa = new QHash<Parameter*, QVector<qreal>*>();
    mapa->insert(planta->numerator()->at(0), tools::linspace(1.0, 10.0, 10));
    // no grid for the uncertain gain "kv"

    auto* epsilon = new QVector<qreal>(6, 10.0);
    QVector<qreal> omega{0.1, 0.5, 1.0, 2.0, 15.0, 100.0};

    Templates t;
    t.setEpsilon(epsilon);
    t.setMapa(mapa);
    EXPECT_THROW(t.lanzarCalculo(planta, &omega, false), qftbx::InvalidInput);

    qDeleteAll(*mapa);
    delete mapa;
    delete epsilon;
}

TEST(TemplatesValidation, RecontourWithoutTemplatesThrowsInvalidInput)
{
    Templates t;
    QVector<qreal> epsilon{10.0};
    EXPECT_THROW(t.lanzarCalculoContorno(&epsilon), qftbx::InvalidInput);
}

} // namespace
