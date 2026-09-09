#ifndef QFTBX_RANGE_UNION_H
#define QFTBX_RANGE_UNION_H

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

#include "src/core/math/range.h"

namespace qftbx {

/**
 * @brief A finite union of closed real intervals, kept in canonical form.
 *
 * The magnitudes a design frequency allows the nominal loop to take at one
 * phase are a union of closed intervals, not an interval: one for an open
 * boundary, two for a closed one, more for a multivalued one, and the
 * intersection over the specifications of the frequency can leave any
 * number of them (BoundaryColumns). A search that has to intersect those
 * sets over the design frequencies and then take the smallest gain left
 * cannot hold them in a pair of numbers: a pair collapses the branches into
 * their hull, and so loses the lower branch of a closed boundary, which is
 * where the smallest feasible gain often lies.
 *
 * Not to be confused with Interval, the rounded interval arithmetic of the
 * natural extension. This is a plain set of reals built out of Range, with
 * no directed rounding: it answers set questions, not arithmetic ones.
 *
 * Canonical form: the members are ascending, disjoint and non-touching, so
 * count() is the number of connected components of the set and the members
 * are those components. Ends may be infinite. A member given with its ends
 * inverted is empty and is dropped, which is what makes an intersection
 * that misses compose as the empty set rather than as a reversed interval.
 */
class RangeUnion
{
public:
    /// The empty set.
    RangeUnion() = default;

    /// The whole real line.
    static RangeUnion whole()
    {
        return of(-std::numeric_limits<double>::infinity(),
                  std::numeric_limits<double>::infinity());
    }

    /// One closed interval; empty when the ends are inverted.
    static RangeUnion of(double lower, double upper)
    {
        RangeUnion set;

        if (lower <= upper) {
            set.m_parts.push_back(Range(lower, upper));
        }

        return set;
    }

    static RangeUnion of(Range range)
    {
        return of(range.min, range.max);
    }

    /**
     * @brief The union of 'count' intervals held as two parallel arrays,
     * which is how BoundaryColumns hands out the intervals of a column.
     *
     * The arrays need not be sorted or disjoint: the result is brought to
     * canonical form either way.
     */
    static RangeUnion of(const double * lower, const double * upper, std::size_t count)
    {
        RangeUnion set;
        set.m_parts.reserve(count);

        for (std::size_t i = 0; i < count; ++i) {
            if (lower[i] <= upper[i]) {
                set.m_parts.push_back(Range(lower[i], upper[i]));
            }
        }

        set.canonicalise();
        return set;
    }

    bool isEmpty() const { return m_parts.empty(); }

    /// The number of connected components of the set.
    std::size_t count() const { return m_parts.size(); }

    /// The i-th component, ascending.
    const Range & at(std::size_t index) const { return m_parts.at(index); }

    const std::vector<Range> & components() const { return m_parts; }

    /// The infimum, which the set attains. Throws when the set is empty.
    double minimum() const
    {
        if (m_parts.empty()) {
            throw std::domain_error("RangeUnion: the empty set has no minimum");
        }

        return m_parts.front().min;
    }

    /// The supremum, which the set attains. Throws when the set is empty.
    double maximum() const
    {
        if (m_parts.empty()) {
            throw std::domain_error("RangeUnion: the empty set has no maximum");
        }

        return m_parts.back().max;
    }

    bool contains(double value) const
    {
        for (const Range & part : m_parts) {
            if (value < part.min) {
                return false;   //ascending: no later member can hold it
            }

            if (value <= part.max) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Intersects with another set, in one pass over both.
     *
     * Both are canonical, so the overlaps come out ascending and disjoint
     * and no second pass is needed.
     */
    RangeUnion & intersectWith(const RangeUnion & other)
    {
        std::vector<Range> parts;
        std::size_t i = 0, j = 0;

        while (i < m_parts.size() && j < other.m_parts.size()) {
            const Range & a = m_parts[i];
            const Range & b = other.m_parts[j];

            const double lower = std::max(a.min, b.min);
            const double upper = std::min(a.max, b.max);

            if (lower <= upper) {
                parts.push_back(Range(lower, upper));
            }

            //Advance the one that ends first; the other may still meet the
            //next member of this one.
            if (a.max < b.max) {
                ++i;
            } else {
                ++j;
            }
        }

        m_parts = std::move(parts);
        return *this;
    }

    RangeUnion & intersectWith(Range range)
    {
        return intersectWith(of(range));
    }

    RangeUnion & intersectWith(double lower, double upper)
    {
        return intersectWith(of(lower, upper));
    }

    /// Translates the whole set, which is what carrying a magnitude set
    /// from the boundary's frame to the gain's amounts to.
    RangeUnion & shiftBy(double delta)
    {
        for (Range & part : m_parts) {
            part.min += delta;
            part.max += delta;
        }

        return *this;
    }

private:
    /// Sorts and merges, so the members end up ascending, disjoint and
    /// non-touching. Touching members are merged because their union as
    /// closed intervals is connected.
    void canonicalise()
    {
        std::sort(m_parts.begin(), m_parts.end(),
                  [](const Range & a, const Range & b) {
                      return a.min < b.min || (a.min == b.min && a.max < b.max);
                  });

        std::vector<Range> merged;

        for (const Range & part : m_parts) {
            if (!merged.empty() && part.min <= merged.back().max) {
                merged.back().max = std::max(merged.back().max, part.max);
            } else {
                merged.push_back(part);
            }
        }

        m_parts = std::move(merged);
    }

    std::vector<Range> m_parts;
};

} // namespace qftbx

#endif // QFTBX_RANGE_UNION_H
