/**
 * @file
 * @brief Deep-equality assertions between two loaded projects.
 *
 * Two systems compare by type, name, coefficient counts and their values at a
 * set of probe frequencies; specifications, templates, boundaries and a
 * loop-shaping result compare member by member. The boundaries are walked per
 * frequency only so that a failure names the frequency instead of dumping
 * every trace of the project.
 */

#ifndef QFTBX_TESTS_PROJECT_COMPARE_H
#define QFTBX_TESTS_PROJECT_COMPARE_H

#include "src/core/templates/cloud_set.h"
#include <gtest/gtest.h>

#include <string>

#include <vector>

#include <complex>

#include "src/core/loopshaping/loop_shaping_result.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"

using namespace qftbx;

namespace qftbx_tests {

inline void expectSameSystem(LtiSystem* a, LtiSystem* b,
                             const std::vector<double>& probes, const char* what)
{
    ASSERT_EQ(a == nullptr, b == nullptr) << what;
    if (a == nullptr) {
        return;
    }
    EXPECT_EQ(a->type(), b->type()) << what;
    EXPECT_EQ(a->name(), b->name()) << what;
    EXPECT_EQ(a->numerator().size(), b->numerator().size()) << what;
    EXPECT_EQ(a->denominator().size(), b->denominator().size()) << what;

    for (double w : probes) {
        const std::complex<double> va = a->evaluate(w);
        const std::complex<double> vb = b->evaluate(w);
        EXPECT_EQ(va, vb) << what << " at w=" << w;
    }
}

inline void expectSameSpecifications(const qftbx::SpecificationRecords * a,
                                     const qftbx::SpecificationRecords * b)
{
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    for (std::size_t i = 0; i < a->size(); ++i) {
        const qftbx::SpecificationRecord & sa = a->at(i);
        const qftbx::SpecificationRecord & sb = b->at(i);
        EXPECT_EQ(sa.name, sb.name) << "spec " << i;
        EXPECT_EQ(sa.used, sb.used) << "spec " << i;
        EXPECT_EQ(sa.constant, sb.constant) << "spec " << i;
        EXPECT_EQ(sa.height, sb.height) << "spec " << i;
        EXPECT_EQ(sa.omegaStart, sb.omegaStart) << "spec " << i;
        EXPECT_EQ(sa.omegaEnd, sb.omegaEnd) << "spec " << i;
        expectSameSystem(sa.system.get(), sb.system.get(),
                         {sa.omegaStart, sa.omegaEnd}, "spec plant");
    }
}

inline void expectSameComplexVectors(const qftbx::CloudSet & a,
                                     const qftbx::CloudSet & b,
                                     const char* what)
{
    ASSERT_EQ(a.size(), b.size()) << what;
    EXPECT_EQ(a, b) << what;
}

inline void expectSameBoundaries(const BoundaryData* a, const BoundaryData* b)
{
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    EXPECT_EQ(a->phaseCount(), b->phaseCount());
    EXPECT_EQ(a->magnitudeCount(), b->magnitudeCount());
    EXPECT_EQ(a->phaseRange(), b->phaseRange());
    EXPECT_EQ(a->magnitudeRange(), b->magnitudeRange());
    EXPECT_EQ(a->openFlags(), b->openFlags());
    EXPECT_EQ(a->upperFlags(), b->upperFlags());

    ASSERT_EQ(a->boundaries().size(), b->boundaries().size());
    for (std::size_t f = 0; f < a->boundaries().size(); ++f) {
        EXPECT_EQ(a->boundaries()[f], b->boundaries()[f]) << "frequency " << f;
    }

    ASSERT_EQ(a->specificationColumns().size(), b->specificationColumns().size());
    for (std::size_t f = 0; f < a->specificationColumns().size(); ++f) {
        EXPECT_EQ(a->specificationColumns()[f], b->specificationColumns()[f]) << "columns " << f;
    }

    ASSERT_EQ(a->unionBoundaries().size(), b->unionBoundaries().size());
    for (std::size_t f = 0; f < a->unionBoundaries().size(); ++f) {
        EXPECT_EQ(a->unionBoundaries()[f], b->unionBoundaries()[f]) << "union " << f;
    }

    ASSERT_EQ(a->unionBuckets().size(), b->unionBuckets().size());
    for (std::size_t f = 0; f < a->unionBuckets().size(); ++f) {
        EXPECT_EQ(a->unionBuckets()[f], b->unionBuckets()[f]) << "buckets " << f;
    }
}

inline void expectSameLoopShaping(LoopShapingResult* a, LoopShapingResult* b)
{
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(a->pointCount(), b->pointCount());
    EXPECT_EQ(a->range(), b->range());
    expectSameSystem(a->controller(), b->controller(), {0.5, 2.0}, "loop shaping");
}

}

#endif
