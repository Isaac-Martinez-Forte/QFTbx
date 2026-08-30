// Characterisation tests for the QFT specification record (tools::dBND, in
// its transitional home) and its persistence, pinning the current behaviour
// before the phase-4 move to qftbx::Specification. "// BUG:" cases document
// known defects and must be flipped by the fix that closes them.

#include <gtest/gtest.h>

#include <cmath>
#include <complex>

#include <QString>
#include <QVector>

#include "Modelo/EstructurasDatos/dbnd.h"
#include "DAO/adaptadorespecificacionesdao.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/parameter.h"
#include "XmlParser/parserload.h"

namespace {

using Complex = std::complex<qreal>;

tools::dBND makeConstantStability(qreal linearHeight)
{
    tools::dBND spec{};
    spec.nombre = QStringLiteral("estabilidad");
    spec.utilizado = true;
    spec.constante = true;
    spec.sistema = nullptr;
    spec.altura = linearHeight; // linear magnitude, not dB
    spec.frecinicio = 0.1;
    spec.frecfinal = 100.0;
    return spec;
}

// 120 / (s^3 + 17 s^2 + 82 s + 120): the tracking specification plant of
// tests/data/planta2.qft.
LtiSystem* makeTrackingPlant()
{
    auto* num = new QVector<Parameter*>{new Parameter(120.0)};
    auto* den = new QVector<Parameter*>{new Parameter(1.0), new Parameter(17.0),
                                        new Parameter(82.0), new Parameter(120.0)};
    return new PolynomialForm(QStringLiteral("seguimiento"), num, den,
                              new Parameter(1.0), new Parameter(0.0));
}

qreal analyticTrackingDb(qreal w)
{
    const Complex s(0.0, w);
    const Complex value = 120.0 / (s * s * s + 17.0 * s * s + 82.0 * s + 120.0);
    return 20.0 * std::log10(std::abs(value));
}

TEST(Specification, ConstantHeightIsDbAndIgnoresOmega)
{
    tools::dBND spec = makeConstantStability(1.2);

    const qreal expected = 20.0 * std::log10(1.2); // 1.5836249...
    EXPECT_NEAR(spec.getAltura(0.5), expected, 1e-12);
    EXPECT_NEAR(spec.getAltura(50.0), expected, 1e-12); // omega is ignored
}

TEST(Specification, SystemHeightMatchesTheAnalyticValue)
{
    tools::dBND spec{};
    spec.nombre = QStringLiteral("seguimiento");
    spec.utilizado = true;
    spec.constante = false;
    spec.sistema = makeTrackingPlant();
    spec.frecinicio = 0.1;
    spec.frecfinal = 10.0;

    EXPECT_NEAR(spec.getAltura(1.0), analyticTrackingDb(1.0), 1e-9);
    EXPECT_NEAR(spec.getAltura(1.0), -0.76398, 1e-4); // hand-checked anchor
    delete spec.sistema;
}

TEST(Specification, ZeroHeightYieldsMinusInfinity)
{
    // BUG: altura == 0 (the default!) gives -inf, and every grid point then
    // passes the contour threshold: the boundary silently degenerates to
    // the window frame. Will become an InvalidInput at construction.
    tools::dBND spec = makeConstantStability(0.0);
    EXPECT_TRUE(std::isinf(spec.getAltura(1.0)));
}

TEST(Specification, NegativeHeightYieldsNaN)
{
    // BUG: altura < 0 gives NaN and the boundary silently comes out empty.
    tools::dBND spec = makeConstantStability(-1.0);
    EXPECT_TRUE(std::isnan(spec.getAltura(1.0)));
}

TEST(SpecificationDao, OwnsReplacesAndToleratesIdentity)
{
    // Fixed: the DAO now owns the records and their embedded plants (deep
    // deletes on replacement and destruction, leak-checked under ASan);
    // handing it the vector it already holds is a no-op.
    auto* first = new QVector<tools::dBND*>();
    for (int i = 0; i < 7; ++i) {
        auto* spec = new tools::dBND{};
        if (i == 0) {
            spec->utilizado = true;
            spec->constante = false;
            spec->sistema = makeTrackingPlant(); // deep-owned
        }
        first->append(spec);
    }

    AdaptadorEspecificacionesDAO dao;
    dao.setEspecificaciones(first);
    dao.setEspecificaciones(first); // identity: must not double-delete
    EXPECT_EQ(dao.getEspecificaciones(), first);

    auto* second = new QVector<tools::dBND*>();
    second->append(new tools::dBND{});
    dao.setEspecificaciones(second); // deep-deletes 'first' and its plant
    EXPECT_EQ(dao.getEspecificaciones(), second);
    // 'second' is deep-deleted by the DAO's destructor.
}

TEST(SpecificationPersistence, MultivaluadosSpecificationsRoundTrip)
{
    XmlParserLoad parser;
    delete parser.recuperarXmlDatos(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/multivaluados.qft"));

    QVector<tools::dBND*>* specs = parser.getEspecificaciones();
    ASSERT_NE(specs, nullptr);
    ASSERT_EQ(specs->size(), 7);

    tools::dBND* lower = specs->at(0);
    EXPECT_EQ(lower->nombre, QStringLiteral("seguimiento"));
    EXPECT_TRUE(lower->utilizado);
    EXPECT_FALSE(lower->constante);
    EXPECT_DOUBLE_EQ(lower->frecinicio, 1.0);
    EXPECT_DOUBLE_EQ(lower->frecfinal, 18.0);
    ASSERT_NE(lower->sistema, nullptr);
    EXPECT_EQ(lower->sistema->type(), LtiSystem::SystemType::PolynomialForm);
    EXPECT_EQ(lower->sistema->numerator()->size(), 2);

    tools::dBND* upper = specs->at(1);
    EXPECT_EQ(upper->nombre, QStringLiteral("seguimiento_1"));
    EXPECT_TRUE(upper->utilizado);
    ASSERT_NE(upper->sistema, nullptr);
    EXPECT_EQ(upper->sistema->numerator()->size(), 3);

    for (int i = 2; i < 7; ++i) {
        EXPECT_FALSE(specs->at(i)->utilizado) << "index " << i;
    }
}

TEST(SpecificationPersistence, Planta2RecoversBothTrackingPlants)
{
    XmlParserLoad parser;
    delete parser.recuperarXmlDatos(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/planta2.qft"));

    QVector<tools::dBND*>* specs = parser.getEspecificaciones();
    ASSERT_NE(specs, nullptr);
    ASSERT_EQ(specs->size(), 7);

    tools::dBND* lower = specs->at(0);
    ASSERT_NE(lower->sistema, nullptr);
    EXPECT_EQ(lower->sistema->type(), LtiSystem::SystemType::PolynomialForm);
    EXPECT_DOUBLE_EQ(lower->frecinicio, 0.1);
    EXPECT_DOUBLE_EQ(lower->frecfinal, 10.0);
    // The recovered plant must evaluate like the analytic reference.
    EXPECT_NEAR(lower->getAltura(1.0), analyticTrackingDb(1.0), 1e-9);

    tools::dBND* upper = specs->at(1);
    ASSERT_NE(upper->sistema, nullptr);
    EXPECT_EQ(upper->sistema->type(), LtiSystem::SystemType::FreeForm);
}

TEST(SpecificationPersistence, Planta1RecoversTheConstantStability)
{
    XmlParserLoad parser;
    delete parser.recuperarXmlDatos(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/planta1.qft"));

    QVector<tools::dBND*>* specs = parser.getEspecificaciones();
    ASSERT_NE(specs, nullptr);
    ASSERT_EQ(specs->size(), 7);

    tools::dBND* stability = specs->at(2);
    EXPECT_TRUE(stability->utilizado);
    EXPECT_TRUE(stability->constante);
    EXPECT_DOUBLE_EQ(stability->altura, 1.2);
    EXPECT_NEAR(stability->getAltura(3.0), 20.0 * std::log10(1.2), 1e-12);

    EXPECT_TRUE(specs->at(4)->utilizado);   // RPS
    EXPECT_FALSE(specs->at(4)->constante);
}

} // namespace
