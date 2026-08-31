// Shared deep-equality assertions between two loaded projects, used by the
// reader-parity and the save/load round-trip tests.

#ifndef QFTBX_TESTS_PROJECT_COMPARE_H
#define QFTBX_TESTS_PROJECT_COMPARE_H

#include <gtest/gtest.h>

#include <complex>

#include <QString>
#include <QVector>

#include "Modelo/EstructurasDatos/datosloopshaping.h"
#include "Modelo/EstructurasDatos/dbnd.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"

namespace qftbx_tests {

inline void expectSameSystem(LtiSystem* a, LtiSystem* b,
                             const QVector<qreal>& probes, const char* what)
{
    ASSERT_EQ(a == nullptr, b == nullptr) << what;
    if (a == nullptr) {
        return;
    }
    EXPECT_EQ(a->type(), b->type()) << what;
    EXPECT_EQ(a->name(), b->name()) << what;
    EXPECT_EQ(a->numerator()->size(), b->numerator()->size()) << what;
    EXPECT_EQ(a->denominator()->size(), b->denominator()->size()) << what;

    for (qreal w : probes) {
        const std::complex<qreal> va = a->evaluate(w);
        const std::complex<qreal> vb = b->evaluate(w);
        EXPECT_EQ(va, vb) << what << " at w=" << w;
    }
}

inline void expectSameSpecifications(QVector<tools::dBND*>* a, QVector<tools::dBND*>* b)
{
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    ASSERT_EQ(a->size(), 7);
    ASSERT_EQ(b->size(), 7);

    for (int i = 0; i < 7; ++i) {
        tools::dBND* sa = a->at(i);
        tools::dBND* sb = b->at(i);
        EXPECT_EQ(sa->nombre, sb->nombre) << "spec " << i;
        EXPECT_EQ(sa->utilizado, sb->utilizado) << "spec " << i;
        EXPECT_EQ(sa->constante, sb->constante) << "spec " << i;
        EXPECT_EQ(sa->altura, sb->altura) << "spec " << i;
        EXPECT_EQ(sa->frecinicio, sb->frecinicio) << "spec " << i;
        EXPECT_EQ(sa->frecfinal, sb->frecfinal) << "spec " << i;
        expectSameSystem(sa->sistema, sb->sistema, {sa->frecinicio, sa->frecfinal},
                         "spec plant");
    }
}

inline void expectSameComplexVectors(QVector<QVector<std::complex<qreal>>*>* a,
                                     QVector<QVector<std::complex<qreal>>*>* b,
                                     const char* what)
{
    ASSERT_NE(a, nullptr) << what;
    ASSERT_NE(b, nullptr) << what;
    ASSERT_EQ(a->size(), b->size()) << what;
    for (int i = 0; i < a->size(); ++i) {
        ASSERT_EQ(*a->at(i), *b->at(i)) << what << " " << i;
    }
}

inline void expectSameBoundaries(BoundaryData* a, BoundaryData* b)
{
    ASSERT_NE(a, nullptr);
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
        ASSERT_EQ(*a->unionBoundaries()->at(f), *b->unionBoundaries()->at(f))
            << "union " << f;
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

inline void expectSameLoopShaping(DatosLoopShaping* a, DatosLoopShaping* b)
{
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(a->getNPuntos(), b->getNPuntos());
    EXPECT_EQ(a->range(), b->range());
    expectSameSystem(a->getControlador(), b->getControlador(), {0.5, 2.0}, "loop shaping");
}

} // namespace qftbx_tests

#endif // QFTBX_TESTS_PROJECT_COMPARE_H
