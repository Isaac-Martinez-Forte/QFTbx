/**
 * @file
 * @brief A finite union of closed intervals, kept in canonical form.
 *
 * The magnitudes a design frequency allows at one phase are a union of
 * intervals, one for an open boundary, two for a closed one, more for a
 * multivalued one; a pair of numbers would lose the lower branch of a
 * closed boundary, where the smallest feasible gain often lies. This is a
 * plain set of reals built out of Range, with no directed rounding, unlike
 * Interval.
 *
 * The members are ascending, disjoint and non-touching, so count() is the
 * number of connected components. Ends may be infinite; a member given
 * with its ends inverted is empty and dropped, and touching members merge.
 * A default set is empty and whole() is the real line. minimum() and
 * maximum() throw on the empty set. intersectWith() is one pass over both
 * sets, and shiftBy() translates the set, as carrying magnitudes to the
 * gain's frame does. A set is reused rather than built: assign() and
 * clear() refill it and the operations work in place, the intersection of
 * two sets through a buffer of the thread, which keeps it safe under
 * OpenMP.
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
