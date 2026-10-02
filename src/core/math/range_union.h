/**
 * @file
 * @brief A finite union of closed intervals, kept in canonical form.
 *
 * The magnitudes a design frequency allows the nominal loop to take at one
 * phase are a union of closed intervals, not an interval: one for an open
 * boundary, two for a closed one, more for a multivalued one, and the
 * intersection over the specifications of the frequency can leave any
 * number of them (BoundaryColumns). A search that has to intersect those
 * sets over the design frequencies and then take the smallest gain left
 * cannot hold them in a pair of numbers: a pair collapses the branches into
 * their hull, and so loses the lower branch of a closed boundary, which is
 * where the smallest feasible gain often lies. Not to be confused with
 * Interval, the rounded interval arithmetic of the natural extension: this
 * is a plain set of reals built out of Range, with no directed rounding, and
 * it answers set questions, not arithmetic ones.
 *
 * Canonical form: the members are ascending, disjoint and non-touching, so
 * count() is the number of connected components of the set and at() the
 * i-th of them, ascending. Ends may be infinite. A member given with its
 * ends inverted is empty and is dropped, which is what makes an
 * intersection that misses compose as the empty set rather than as a
 * reversed interval, and touching members are merged, because their union
 * as closed intervals is connected. A default set is empty and whole() is
 * the real line. The intervals of a column, which BoundaryColumns hands out
 * as two parallel arrays, need not be sorted or disjoint: the set is brought
 * to canonical form either way. minimum() and maximum() are attained and
 * throw on the empty set, and contains() stops at the first member above
 * the value. intersectWith() takes one pass over both sets, since both are
 * canonical and the overlaps come out ascending and disjoint, and shiftBy()
 * translates the whole set, which is what carrying a magnitude set from the
 * boundary's frame to the gain's amounts to. The searches build and cut these
 * sets millions of times, so a set is reused rather than built: assign() and
 * clear() refill it, the intersection with one interval clips the members in
 * place, the merge of canonical form is done in place, and the intersection
 * of two sets collects its overlaps in a buffer of the thread and swaps it
 * in, which leaves the result alone to each thread under OpenMP.
 */

#ifndef QFTBX_RANGE_UNION_H
#define QFTBX_RANGE_UNION_H

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

#include "src/core/math/range.h"

namespace qftbx {

class RangeUnion
{
public:
    RangeUnion() = default;

    static RangeUnion whole()
    {
        return of(-std::numeric_limits<double>::infinity(),
                  std::numeric_limits<double>::infinity());
    }

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

    void clear() { m_parts.clear(); }

    void assign(double lower, double upper)
    {
        m_parts.clear();
        if (lower <= upper) {
            m_parts.push_back(Range(lower, upper));
        }
    }

    void assign(const double * lower, const double * upper, std::size_t count)
    {
        m_parts.clear();
        for (std::size_t i = 0; i < count; ++i) {
            if (lower[i] <= upper[i]) {
                m_parts.push_back(Range(lower[i], upper[i]));
            }
        }
        canonicalise();
    }

    bool isEmpty() const { return m_parts.empty(); }

    std::size_t count() const { return m_parts.size(); }

    const Range & at(std::size_t index) const { return m_parts.at(index); }

    const std::vector<Range> & components() const { return m_parts; }

    double minimum() const
    {
        if (m_parts.empty()) {
            throw std::domain_error("RangeUnion: the empty set has no minimum");
        }

        return m_parts.front().min;
    }

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
                return false;
            }

            if (value <= part.max) {
                return true;
            }
        }

        return false;
    }

    RangeUnion & intersectWith(const RangeUnion & other)
    {
        static thread_local std::vector<Range> overlaps;
        std::vector<Range> & parts = overlaps;
        parts.clear();
        std::size_t i = 0, j = 0;

        while (i < m_parts.size() && j < other.m_parts.size()) {
            const Range & a = m_parts[i];
            const Range & b = other.m_parts[j];

            const double lower = std::max(a.min, b.min);
            const double upper = std::min(a.max, b.max);

            if (lower <= upper) {
                parts.push_back(Range(lower, upper));
            }

            if (a.max < b.max) {
                ++i;
            } else {
                ++j;
            }
        }

        m_parts.swap(parts);
        return *this;
    }

    RangeUnion & intersectWith(Range range)
    {
        return intersectWith(range.min, range.max);
    }

    RangeUnion & intersectWith(double lower, double upper)
    {
        if (!(lower <= upper)) {
            m_parts.clear();
            return *this;
        }
        std::size_t kept = 0;
        for (std::size_t i = 0; i < m_parts.size(); ++i) {
            const Range part = m_parts[i];
            const double low = std::max(part.min, lower);
            const double high = std::min(part.max, upper);
            if (low <= high) {
                m_parts[kept++] = Range(low, high);
            }
            if (!(part.max < upper)) {
                break;
            }
        }
        m_parts.resize(kept);
        return *this;
    }

    RangeUnion & shiftBy(double delta)
    {
        for (Range & part : m_parts) {
            part.min += delta;
            part.max += delta;
        }

        return *this;
    }

private:
    void canonicalise()
    {
        std::sort(m_parts.begin(), m_parts.end(),
                  [](const Range & a, const Range & b) {
                      return a.min < b.min || (a.min == b.min && a.max < b.max);
                  });

        std::size_t kept = 0;

        for (std::size_t i = 0; i < m_parts.size(); ++i) {
            const Range part = m_parts[i];
            if (kept > 0 && part.min <= m_parts[kept - 1].max) {
                m_parts[kept - 1].max = std::max(m_parts[kept - 1].max, part.max);
            } else {
                m_parts[kept++] = part;
            }
        }

        m_parts.resize(kept);
    }

    std::vector<Range> m_parts;
};

}

#endif
