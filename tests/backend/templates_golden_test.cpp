// Golden tests for the template computation against tests/data/planta2.qft,
// which ships the full clouds and epsilon-hull contours computed by the
// original program (6 frequencies, 10x10 parameter grid, epsilon = 10).
//
// The fixture serialises with 6 significant digits, so comparisons use a
// relative tolerance. NOTE: the whole test binary currently runs with
// OMP_NUM_THREADS=1 (see tests/CMakeLists.txt): calcularContorno applies a
// thread-order permutation that desynchronises templates from contours with
// more threads; the pin will be removed when that bug is fixed.

#include <gtest/gtest.h>

#include <complex>

#include <QHash>
#include <QString>
#include <QVector>

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
        // Templates owns nothing it was given; free what we created. The
        // vectors handed to lanzarCalculo/setEpsilon get cleared and
        // abandoned by calcularContorno (aliasing bug, pinned below).
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

TEST_F(TemplatesGolden, ContourMatchesFixture)
{
    auto* computed = templates.getContorno();
    auto* expected = parser.getContorno();
    ASSERT_NE(computed, nullptr);
    ASSERT_NE(expected, nullptr);
    ASSERT_EQ(computed->size(), expected->size());

    const int expectedSizes[] = {30, 28, 28, 28, 28, 28};
    for (int f = 0; f < computed->size(); ++f) {
        ASSERT_EQ(expected->at(f)->size(), expectedSizes[f]);
        ASSERT_EQ(computed->at(f)->size(), expected->at(f)->size())
            << "frequency " << f;
        for (int p = 0; p < computed->at(f)->size(); ++p) {
            expectNear(computed->at(f)->at(p), expected->at(f)->at(p),
                       "contour point");
        }
    }
}

TEST_F(TemplatesGolden, ContourStartsAtMaxImaginary)
{
    // Explicit anchor of divergence D1: the walk currently starts at the
    // point with the largest imaginary part; MATLAB and the PFC text use
    // the largest real part. When D1 is aligned, this flips and the golden
    // contours in the fixture must be regenerated.
    auto* temps = templates.getTemplates();
    auto* conts = templates.getContorno();

    for (int f = 0; f < conts->size(); ++f) {
        Complex maxImag = temps->at(f)->at(0);
        for (const Complex& p : *temps->at(f)) {
            if (p.imag() > maxImag.imag()) {
                maxImag = p;
            }
        }
        EXPECT_EQ(conts->at(f)->at(0), maxImag) << "frequency " << f;
    }
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
    // The i-th contour must correspond to the i-th frequency. With more
    // than one OpenMP thread this fails intermittently today (the critical
    // section renumbers with a thread counter); single-threaded it pins the
    // correct correspondence that the fix must preserve.
    const QVector<qreal> original{0.1, 0.5, 1.0, 2.0, 15.0, 100.0};
    auto* omegaOut = templates.getOmega();
    ASSERT_NE(omegaOut, nullptr);
    ASSERT_EQ(omegaOut->size(), original.size());
    for (int i = 0; i < original.size(); ++i) {
        EXPECT_DOUBLE_EQ(omegaOut->at(i), original.at(i)) << "index " << i;
    }
}

TEST_F(TemplatesGolden, InputVectorsAreClearedByTheComputation)
{
    // BUG (aliasing): calcularContorno clears the omega and epsilon vectors
    // it was handed (the caller's property) and abandons them.
    EXPECT_TRUE(omegaCopy->isEmpty());
    EXPECT_TRUE(epsilon->isEmpty());
}

} // namespace
