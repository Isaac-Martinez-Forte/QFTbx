// Golden tests for the template computation against tests/data/planta2.qft,
// which ships the full clouds and epsilon-hull contours computed by the
// original program (6 frequencies, 10x10 parameter grid, epsilon = 10).
//
// The fixture serialises with 6 significant digits, so comparisons use a
// relative tolerance. The tests run multithreaded: every computation writes
// at the index of its own frequency (the old thread-order permutation and
// its omega/epsilon aliasing repair are gone).

#include <gtest/gtest.h>

#include <algorithm>

#include <complex>

#include <QHash>
#include <QString>
#include <QVector>

#include "src/core/project_controller.h"
#include "src/core/exception.h"
#include "src/core/templates/template_engine.h"
#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/persistence/project_reader.h"

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
        parser.load(
            QStringLiteral(QFTBX_TEST_DATA_DIR "/planta2.qft"));
        planta = parser.plant();
        ASSERT_NE(planta, nullptr);

        // The fixture was computed on a 10x10 grid: a and kv in [1,10].
        mapa.clear();
        mapa[planta->numerator()[0].name()] = qftbx::math::linspace(1.0, 10.0, 10);
        mapa[planta->gain().name()] = qftbx::math::linspace(1.0, 10.0, 10);

        omegaCopy = new QVector<qreal>(*parser.omega()->values());
        epsilon = QVector<qreal>(6, 10.0);

        templates.setEpsilon(epsilon);
        templates.setGrids(mapa);
        templates.compute(planta, omegaCopy, false);
    }

    void TearDown() override
    {
        // TemplateEngine owns nothing it was given; free what we created.
        delete omegaCopy;
    }

    ProjectReader parser;
    LtiSystem* planta = nullptr;
    qftbx::ParameterGrids mapa;
    QVector<qreal>* omegaCopy = nullptr;
    QVector<qreal> epsilon;
    TemplateEngine templates;
};

TEST_F(TemplatesGolden, BruteForceMatchesFixture)
{
    const qftbx::CloudSet & computed = templates.clouds();
    const qftbx::CloudSet & expected = parser.templates();
    ASSERT_EQ(static_cast<int>(computed.size()), static_cast<int>(expected.size()));

    for (int f = 0; f < static_cast<int>(computed.size()); ++f) {
        ASSERT_EQ(computed.at(f).size(), expected.at(f).size())
            << "frequency " << f;
        for (int p = 0; p < computed.at(f).size(); ++p) {
            expectNear(computed.at(f).at(p), expected.at(f).at(p),
                       "template point");
        }
    }

    // Hard anchors fixing the sweep order (a is the fast digit).
    expectNear(computed.at(0).at(0), Complex(-0.990099, -9.90099), "t[0][0]");
    expectNear(computed.at(0).at(1), Complex(-0.498753, -9.97506), "t[0][1]");
}

TEST_F(TemplatesGolden, ContourMatchesFixtureAsACycle)
{
    // The faithful EPSHULL.M walk returns the same cycle as the historical
    // golden but closed (last point repeats the first) and rotated (it
    // starts at max real instead of max imaginary), so the comparison is
    // cyclic: same sequence, same direction, any starting point. The
    // fallback frequencies (0 and 2, where the reference walk cycles)
    // reproduce the historical sequence exactly.
    const qftbx::CloudSet & computed = templates.contours();
    const qftbx::CloudSet & expected = parser.contour();
    ASSERT_EQ(static_cast<int>(computed.size()), static_cast<int>(expected.size()));

    const int expectedSizes[] = {30, 28, 28, 28, 28, 28};
    for (int f = 0; f < static_cast<int>(computed.size()); ++f) {
        ASSERT_EQ(expected.at(f).size(), expectedSizes[f]);

        QVector<Complex> cycle(computed.at(f).begin(), computed.at(f).end());
        if (cycle.size() > 1 && cycle.first() == cycle.last()) {
            cycle.removeLast(); // closing duplicate
        }
        ASSERT_EQ(cycle.size(), expected.at(f).size()) << "frequency " << f;

        // Locate the rotation offset: the computed point closest to the
        // first expected point.
        int offset = 0;
        qreal best = std::abs(cycle.at(0) - expected.at(f).at(0));
        for (int i = 1; i < cycle.size(); ++i) {
            const qreal d = std::abs(cycle.at(i) - expected.at(f).at(0));
            if (d < best) {
                best = d;
                offset = i;
            }
        }

        for (int p = 0; p < cycle.size(); ++p) {
            expectNear(cycle.at((offset + p) % cycle.size()),
                       expected.at(f).at(p), "contour point");
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
    const qftbx::CloudSet & temps = templates.clouds();
    const qftbx::CloudSet & conts = templates.contours();

    int fallbacks = 0;
    for (int f = 0; f < static_cast<int>(conts.size()); ++f) {
        const qftbx::ComplexCloud & c = conts.at(static_cast<std::size_t>(f));
        const bool faithful = c.size() > 1 && c.front() == c.back();
        if (!faithful) {
            ++fallbacks;
        }
        Complex extreme = temps.at(f).at(0);
        for (const Complex& p : temps.at(static_cast<std::size_t>(f))) {
            const bool better = faithful ? p.real() > extreme.real()
                                         : p.imag() > extreme.imag();
            if (better) {
                extreme = p;
            }
        }
        EXPECT_EQ(c.at(0), extreme)
            << "frequency " << f << (faithful ? " (faithful)" : " (fallback)");
    }

    // The clustered clouds of this fixture make some frequencies fall back.
    EXPECT_GT(fallbacks, 0);
    EXPECT_LT(fallbacks, static_cast<int>(conts.size()));
}

TEST_F(TemplatesGolden, ContourIsSubsetOfTemplate)
{
    const qftbx::CloudSet & temps = templates.clouds();
    const qftbx::CloudSet & conts = templates.contours();

    for (int f = 0; f < static_cast<int>(conts.size()); ++f) {
        const qftbx::ComplexCloud & cloud = temps.at(static_cast<std::size_t>(f));
        for (const Complex& p : conts.at(static_cast<std::size_t>(f))) {
            EXPECT_TRUE(std::find(cloud.begin(), cloud.end(), p) != cloud.end())
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
    auto* omegaOut = templates.omega();
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
    ASSERT_EQ(epsilon.size(), 6);
    EXPECT_DOUBLE_EQ(epsilon.at(0), 10.0);
}

TEST(TemplatesReload, RecalculateContourAfterLoadingAProject)
{
    // Fixed crash: loading a project fed only the DAO, so recalculating
    // the contour dereferenced a null templates vector inside the engine.
    ProjectController controlador;
    controlador.load(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/planta2.qft"));

    const qftbx::CloudSet & contornos = controlador.recomputeContour(QVector<qreal>(6, 10.0));
    ASSERT_EQ(contornos.size(), 6u);
    for (const qftbx::ComplexCloud & c : contornos) {
        EXPECT_FALSE(c.empty());
    }

    // Second recalculation. This used to be about the DAO deep-deleting the
    // previous contour without a double free; with the set held by value
    // there is no deletion to get wrong, and the assertion is just that the
    // recomputation still produces one contour per frequency.
    const qftbx::CloudSet & contornos2 = controlador.recomputeContour(QVector<qreal>(6, 8.0));
    ASSERT_EQ(contornos2.size(), 6u);
}

TEST(TemplatesValidation, MissingSweepGridThrowsInvalidInput)
{
    // Hardened: a map without an entry for some uncertain parameter used to
    // dereference null; now it reports which grid is missing.
    ProjectReader parser;
    parser.load(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/planta2.qft"));
    LtiSystem* planta = parser.plant();

    qftbx::ParameterGrids mapa;
    mapa[planta->numerator()[0].name()] = qftbx::math::linspace(1.0, 10.0, 10);
    // no grid for the uncertain gain "kv"

    QVector<qreal> omega{0.1, 0.5, 1.0, 2.0, 15.0, 100.0};

    TemplateEngine t;
    t.setEpsilon(QVector<qreal>(6, 10.0));
    t.setGrids(mapa);
    EXPECT_THROW(t.compute(planta, &omega, false), qftbx::InvalidInput);
}

TEST(TemplatesValidation, RecontourWithoutTemplatesThrowsInvalidInput)
{
    TemplateEngine t;
    EXPECT_THROW(t.computeContours(QVector<qreal>{10.0}), qftbx::InvalidInput);
}

} // namespace
