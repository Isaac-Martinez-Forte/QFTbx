// End-to-end characterisation: the plants recovered from the real .qft
// fixtures must match the documented examples (cervera: ACC Cervera & Baños
// 2013; planta1: PFC example plant 1).

#include <gtest/gtest.h>

#include <complex>

#include <QPointF>
#include <QString>

#include "Modelo/EstructuraSistema/sistema.h"
#include "Modelo/EstructurasDatos/var.h"
#include "Modelo/Objetos/omega.h"
#include "XmlParser/parserload.h"

namespace {

using Complex = std::complex<qreal>;

constexpr qreal kTolerance = 1e-9;

TEST(PlantFixture, CerveraRoundTrip)
{
    XmlParserLoad parser;
    delete parser.recuperarXmlDatos(QString(QFTBX_TEST_DATA_DIR "/cervera.qft"));

    Sistema* planta = parser.getPlanta();
    ASSERT_NE(planta, nullptr);

    EXPECT_EQ(planta->getClass(), Sistema::formato_libre);
    EXPECT_EQ(planta->getNumeradorString(), QStringLiteral("a"));
    EXPECT_EQ(planta->getDenominadorString(), QStringLiteral("(s^2)*((s^2) + a)"));

    ASSERT_EQ(planta->getNumerador()->size(), 1);
    Var* a = planta->getNumerador()->at(0);
    EXPECT_TRUE(a->isVariable());
    EXPECT_EQ(a->getNombre(), QStringLiteral("a"));
    EXPECT_DOUBLE_EQ(a->getN(), 2.0);
    EXPECT_EQ(a->getR(), QPointF(0.5, 2.0));

    // Numerator and denominator carry two distinct Var objects that share
    // the same name — the pointer-keyed template map relies on this.
    ASSERT_EQ(planta->getDenominador()->size(), 1);
    EXPECT_NE(planta->getDenominador()->at(0), a);
    EXPECT_EQ(planta->getDenominador()->at(0)->getNombre(), QStringLiteral("a"));

    EXPECT_FALSE(planta->getK()->isVariable());
    EXPECT_DOUBLE_EQ(planta->getK()->getNominal(), 1.0);
    EXPECT_FALSE(planta->getRet()->isVariable());
    EXPECT_DOUBLE_EQ(planta->getRet()->getNominal(), 0.0);

    // P(j0.1) with the nominal a = 2.
    const Complex s(0.0, 0.1);
    const Complex expected = 2.0 / ((s * s) * (s * s + 2.0));
    const Complex value = planta->getPunto(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);

    Omega* omega = parser.getOmega();
    ASSERT_NE(omega, nullptr);
    ASSERT_EQ(omega->getValores()->size(), 4);
}

TEST(PlantFixture, Planta1RoundTrip)
{
    XmlParserLoad parser;
    delete parser.recuperarXmlDatos(QString(QFTBX_TEST_DATA_DIR "/planta1.qft"));

    Sistema* planta = parser.getPlanta();
    ASSERT_NE(planta, nullptr);

    EXPECT_EQ(planta->getClass(), Sistema::k_ganancia);
    EXPECT_TRUE(planta->getNumerador()->isEmpty());
    ASSERT_EQ(planta->getDenominador()->size(), 2);
    EXPECT_EQ(planta->getDenominador()->at(0)->getNombre(), QStringLiteral("a"));
    EXPECT_EQ(planta->getDenominador()->at(1)->getNombre(), QStringLiteral("b"));

    EXPECT_TRUE(planta->getK()->isVariable());
    EXPECT_EQ(planta->getK()->getNombre(), QStringLiteral("kv"));
    EXPECT_EQ(planta->getK()->getR(), QPointF(1.0, 10.0));

    EXPECT_EQ(planta->getExpr(0.1),
              QStringLiteral("kv*(1) / (((0.1*i) + a) *((0.1*i) + b))"));

    // Nominals kv=1, a=5, b=30 at s = 0.1j.
    const Complex s(0.0, 0.1);
    const Complex expected = 1.0 / ((s + 5.0) * (s + 30.0));
    const Complex value = planta->getPunto(0.1);
    EXPECT_NEAR(value.real(), expected.real(), kTolerance);
    EXPECT_NEAR(value.imag(), expected.imag(), kTolerance);
}

} // namespace
