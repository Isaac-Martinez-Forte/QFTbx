/**
 * @file
 * @brief The long kernels of the templates stop when the computation is
 * cancelled.
 *
 * With a card, a walk round a circle of 2^18 points, the spanning tree and
 * the diameter of 2^17 and the faces of the alpha-shape of 2^16 each take
 * a long while in one kernel. Cancelled 20 ms after they start, they must
 * say so and end in less than a quarter of what they take uncancelled,
 * which has to be long enough on the card to tell.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <complex>
#include <cstdio>
#include <functional>
#include <thread>
#include <vector>

#include "src/core/gpu/templates/alpha_contour.h"
#include "src/core/gpu/templates/contour_walk.h"
#include "src/core/gpu/templates/epsilon_proposal.h"
#include "src/core/gpu/templates/walk_preparation.h"
#include "src/core/math/constants.h"
#include "src/core/pipeline/cancellation.h"
#include "tests/gpu/card.h"

using namespace qftbx;

namespace {

struct Timed
{
    bool cancelled = false;
    double milliseconds = 0.0;
};

Timed timed(const std::function<bool(const CancellationToken *)> & run, bool cancel)
{
    CancellationToken token;
    std::thread canceller;
    if (cancel) {
        canceller = std::thread([&token] {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            token.cancel();
        });
    }
    const auto started = std::chrono::steady_clock::now();
    Timed out;
    out.cancelled = run(&token);
    out.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    if (canceller.joinable()) {
        canceller.join();
    }
    return out;
}

CloudSet circle(std::size_t points)
{
    ComplexCloud cloud;
    for (std::size_t k = 0; k < points; ++k) {
        cloud.push_back(std::polar(1.0, 2.0 * math::kPi * static_cast<double>(k) / static_cast<double>(points)));
    }
    return {cloud};
}

double spacingEpsilon(std::size_t points)
{
    return 2.5 * 2.0 * math::kPi / static_cast<double>(points);
}

void expectStops(const char * what, const std::function<bool(const CancellationToken *)> & run)
{
    const Timed whole = timed(run, false);
    ASSERT_FALSE(whole.cancelled) << what;
    ASSERT_GT(whole.milliseconds, 200.0) << what << ": too small a problem to tell on this card";
    const Timed cut = timed(run, true);
    EXPECT_TRUE(cut.cancelled) << what;
    EXPECT_LT(cut.milliseconds, whole.milliseconds / 4.0) << what;
    std::printf("%s: %.0f ms, cancelled after 20 ms: %.0f ms\n", what, whole.milliseconds, cut.milliseconds);
}

}

TEST(GpuTemplates, TheLongKernelsStopWhenTheComputationIsCancelled)
{
    QFTBX_REQUIRE_CARD();

    const std::size_t walked = std::size_t{1} << 18;
    const auto walkClouds = gpu::cloudsOnCard(circle(walked));
    ASSERT_NE(walkClouds, nullptr);
    expectStops("walk", [&](const CancellationToken * token) {
        return gpu::walkedOnCard(*walkClouds, {spacingEpsilon(walked)}, HullMetric::ComplexPlane, 1.0, token)
                .cancelled;
    });

    const auto treeClouds = gpu::cloudsOnCard(circle(std::size_t{1} << 17));
    ASSERT_NE(treeClouds, nullptr);
    expectStops("spanning tree and diameter", [&](const CancellationToken * token) {
        return gpu::proposedOnCard(*treeClouds, HullMetric::ComplexPlane, 1.0, false, token).cancelled;
    });

    const std::size_t shaped = std::size_t{1} << 16;
    const auto alphaClouds = gpu::cloudsOnCard(circle(shaped));
    ASSERT_NE(alphaClouds, nullptr);
    expectStops("alpha-shape", [&](const CancellationToken * token) {
        return gpu::alphaOnCard(*alphaClouds, {spacingEpsilon(shaped)}, HullMetric::ComplexPlane, 1.0, token)
                .cancelled;
    });
}
