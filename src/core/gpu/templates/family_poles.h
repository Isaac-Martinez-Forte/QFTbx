/**
 * @file
 * @brief The check of the poles of a plant family on the card.
 *
 * A family's poles depend only on its denominator, so the card goes through
 * the distinct denominators rather than the combinations: the grids of the
 * parameters the denominator reads, with every other at its first value,
 * or the points of the border. Each is the first combination with that
 * denominator, whose index the card keeps. One warp finds the poles of a
 * denominator: for a free form it recovers the polynomial from 64 samples
 * on two circles, then it finds the roots and counts those in the right
 * half-plane and on the imaginary axis.
 *
 * familyOnCard() reduces them on the card, with no array per denominator:
 * the count of the first combination, the first combination whose poles
 * cannot be placed, the first whose count differs and the band each
 * frequency on the axis sweeps. It stops where the processor's check would,
 * at the first denominator it cannot place: a count that differs is kept
 * only before it, and the bands are taken again over the combinations
 * before it. Plain C++, so the glue includes it without the toolkit; a
 * failure on the card throws CudaError. familyOnProcessor() runs the same
 * arithmetic one denominator after another on the processor, which is how
 * the tests check the card's work without one.
 */

#ifndef QFTBX_GPU_FAMILY_POLES_H
#define QFTBX_GPU_FAMILY_POLES_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "src/core/gpu/templates/plant_model.h"

namespace qftbx::gpu {

struct FamilyOnCard
{
    std::optional<int> firstCount;
    std::optional<std::uint64_t> unplaceableAt;
    std::optional<std::uint64_t> otherCountAt;
    int otherCount = 0;
    std::vector<std::array<double, 2>> bands;
    std::size_t denominators = 0;
    double kernelMilliseconds = 0.0;
};

FamilyOnCard familyOnCard(const PlantModel & plant, const SweepGrid & grid);

FamilyOnCard familyOnProcessor(const PlantModel & plant, const SweepGrid & grid);

}

#endif
