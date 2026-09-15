#include "src/core/templates/alpha_shape.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <map>
#include <set>
#include <utility>

namespace qftbx {

namespace {

using Complex = std::complex<double>;

//Points within epsilon of each point, by a grid of cells of side epsilon.
std::vector<std::vector<std::int32_t>> neighbourLists(const ComplexCloud & p, double epsilon)
{
    const std::int32_t n = static_cast<std::int32_t>(p.size());
    std::map<std::pair<long long, long long>, std::vector<std::int32_t>> cells;
    const auto cellOf = [epsilon](const Complex & z) {
        return std::make_pair(static_cast<long long>(std::floor(z.real() / epsilon)),
                              static_cast<long long>(std::floor(z.imag() / epsilon)));
    };
    for (std::int32_t i = 0; i < n; ++i) {
        cells[cellOf(p[static_cast<std::size_t>(i)])].push_back(i);
    }

    std::vector<std::vector<std::int32_t>> near(static_cast<std::size_t>(n));
    const double reach = epsilon * epsilon * (1.0 + 1e-9);
    for (std::int32_t i = 0; i < n; ++i) {
        const Complex & zi = p[static_cast<std::size_t>(i)];
        const auto c = cellOf(zi);
        for (long long dx = -1; dx <= 1; ++dx) {
            for (long long dy = -1; dy <= 1; ++dy) {
                const auto it = cells.find(std::make_pair(c.first + dx, c.second + dy));
                if (it == cells.end()) continue;
                for (const std::int32_t j : it->second) {
                    if (j == i) continue;
                    const Complex d = p[static_cast<std::size_t>(j)] - zi;
                    if (std::norm(d) <= reach) near[static_cast<std::size_t>(i)].push_back(j);
                }
            }
        }
    }
    return near;
}

} // namespace

AlphaShape alphaShape(const ComplexCloud & points, double epsilon)
{
    AlphaShape shape;
    const std::int32_t n = static_cast<std::int32_t>(points.size());
    if (n == 0 || !(epsilon > 0.0)) {
        return shape;
    }

    const std::vector<std::vector<std::int32_t>> near = neighbourLists(points, epsilon);

    //The boundary edges: one of the two discs of radius epsilon/2 through
    //the pair is empty of every other point. A point exactly on the disc
    //does not block it.
    const double radius = epsilon / 2.0;
    const double blocking = radius * radius * (1.0 - 1e-9);
    std::set<std::pair<std::int32_t, std::int32_t>> edges;
    for (std::int32_t i = 0; i < n; ++i) {
        const Complex & a = points[static_cast<std::size_t>(i)];
        for (const std::int32_t j : near[static_cast<std::size_t>(i)]) {
            if (j <= i) continue;
            const Complex & b = points[static_cast<std::size_t>(j)];
            const Complex chord = b - a;
            const double half = std::abs(chord) / 2.0;
            if (!(half > 0.0)) continue;   //a repeated point: not an edge
            const Complex mid = (a + b) / 2.0;
            const double sagitta = std::sqrt(std::max(0.0, radius * radius - half * half));
            const Complex unit = chord / std::abs(chord);
            const Complex normal(-unit.imag(), unit.real());
            for (const double side : {1.0, -1.0}) {
                const Complex centre = mid + side * sagitta * normal;
                bool empty = true;
                //Any blocker is within radius of the centre, hence within
                //epsilon of a: it is in a's neighbour list.
                for (const std::int32_t k : near[static_cast<std::size_t>(i)]) {
                    if (k == j) continue;
                    if (std::norm(points[static_cast<std::size_t>(k)] - centre) < blocking) {
                        empty = false;
                        break;
                    }
                }
                if (empty) {
                    edges.insert(std::make_pair(i, j));
                    break;
                }
            }
        }
    }

    //The faces of the planar graph of boundary edges: from a directed edge
    //u->v, the next edge leaves v as the first one clockwise after v->u.
    std::vector<std::vector<std::int32_t>> out(static_cast<std::size_t>(n));
    for (const auto & e : edges) {
        out[static_cast<std::size_t>(e.first)].push_back(e.second);
        out[static_cast<std::size_t>(e.second)].push_back(e.first);
    }
    const auto angle = [&points](std::int32_t from, std::int32_t to) {
        return std::arg(points[static_cast<std::size_t>(to)] - points[static_cast<std::size_t>(from)]);
    };
    for (std::int32_t v = 0; v < n; ++v) {
        std::vector<std::int32_t> & o = out[static_cast<std::size_t>(v)];
        std::sort(o.begin(), o.end(), [&](std::int32_t x, std::int32_t y) { return angle(v, x) < angle(v, y); });
    }

    //The connected components of the edge graph, so that each gets its own
    //outer loop.
    std::vector<std::int32_t> parent(static_cast<std::size_t>(n));
    for (std::int32_t v = 0; v < n; ++v) parent[static_cast<std::size_t>(v)] = v;
    const auto find = [&parent](std::int32_t v) {
        while (parent[static_cast<std::size_t>(v)] != v) {
            parent[static_cast<std::size_t>(v)] = parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(v)])];
            v = parent[static_cast<std::size_t>(v)];
        }
        return v;
    };
    for (const auto & e : edges) {
        const std::int32_t a = find(e.first), b = find(e.second);
        if (a != b) parent[static_cast<std::size_t>(a)] = b;
    }

    //Every face, each directed edge used once. The outer boundary of a
    //component is its face of largest area: the holes are inside it, and a
    //component with no area (a chain of spikes) has one face, walked out
    //and back. Holes are not returned: the contour of a template is its
    //outer border, as the walk gives it, and a hole treated as filled only
    //adds to the value set what the plants around it enclose.
    std::set<std::pair<std::int32_t, std::int32_t>> used;   //directed
    std::map<std::int32_t, std::pair<double, std::vector<std::int32_t>>> outer;   //component -> (area, loop)
    for (const auto & e : edges) {
        for (const auto start : {std::make_pair(e.first, e.second), std::make_pair(e.second, e.first)}) {
            if (used.count(start)) continue;
            std::vector<std::int32_t> loop;
            std::pair<std::int32_t, std::int32_t> cur = start;
            while (!used.count(cur)) {
                used.insert(cur);
                loop.push_back(cur.first);
                const std::int32_t u = cur.first, v = cur.second;
                const std::vector<std::int32_t> & o = out[static_cast<std::size_t>(v)];
                //The reverse edge v->u in the angular order at v, then the
                //previous one (clockwise), wrapping round.
                const auto it = std::find(o.begin(), o.end(), u);
                std::size_t pos = static_cast<std::size_t>(it - o.begin());
                pos = (pos + o.size() - 1) % o.size();
                cur = std::make_pair(v, o[pos]);
            }
            double area = 0.0;
            for (std::size_t k = 0; k < loop.size(); ++k) {
                const Complex & a = points[static_cast<std::size_t>(loop[k])];
                const Complex & b = points[static_cast<std::size_t>(loop[(k + 1) % loop.size()])];
                area += a.real() * b.imag() - b.real() * a.imag();
            }
            area = std::abs(area) / 2.0;
            auto & best = outer[find(start.first)];
            if (best.second.empty() || area > best.first) {
                best = std::make_pair(area, std::move(loop));
            }
        }
    }
    std::vector<std::vector<std::int32_t>> candidates;
    for (auto & entry : outer) {
        candidates.push_back(std::move(entry.second.second));
    }

    //A hole's edges form a component of their own, disconnected from the
    //border round it; its loop lies inside another's. Keep the loops that
    //lie inside no other: the outer borders.
    const auto inside = [&points](const Complex & z, const std::vector<std::int32_t> & poly) {
        bool in = false;
        for (std::size_t k = 0, m = poly.size(); k < m; ++k) {
            const Complex & a = points[static_cast<std::size_t>(poly[k])];
            const Complex & b = points[static_cast<std::size_t>(poly[(k + 1) % m])];
            if ((a.imag() > z.imag()) != (b.imag() > z.imag())) {
                const double x = a.real() + (z.imag() - a.imag()) * (b.real() - a.real()) / (b.imag() - a.imag());
                if (x > z.real()) in = !in;
            }
        }
        return in;
    };
    std::vector<std::vector<std::int32_t>> loops;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const Complex & z = points[static_cast<std::size_t>(candidates[i].front())];
        bool hole = false;
        for (std::size_t j = 0; j < candidates.size() && !hole; ++j) {
            if (j != i && candidates[j].size() > 2 && inside(z, candidates[j])) hole = true;
        }
        if (!hole) loops.push_back(std::move(candidates[i]));
    }

    //Points with no neighbour within epsilon: components of their own. (A
    //point with neighbours but no boundary edge is interior.)
    for (std::int32_t v = 0; v < n; ++v) {
        if (near[static_cast<std::size_t>(v)].empty()) {
            loops.push_back(std::vector<std::int32_t>(1, v));
        }
    }

    //Each loop from its rightmost point; loops rightmost first.
    const auto rightmost = [&points](const std::vector<std::int32_t> & loop) {
        std::size_t best = 0;
        for (std::size_t k = 1; k < loop.size(); ++k) {
            const Complex & a = points[static_cast<std::size_t>(loop[k])];
            const Complex & b = points[static_cast<std::size_t>(loop[best])];
            if (a.real() > b.real() || (a.real() == b.real() && a.imag() > b.imag())) best = k;
        }
        return best;
    };
    for (std::vector<std::int32_t> & loop : loops) {
        std::rotate(loop.begin(), loop.begin() + static_cast<std::ptrdiff_t>(rightmost(loop)), loop.end());
    }
    std::sort(loops.begin(), loops.end(), [&](const std::vector<std::int32_t> & x, const std::vector<std::int32_t> & y) {
        const Complex & a = points[static_cast<std::size_t>(x.front())];
        const Complex & b = points[static_cast<std::size_t>(y.front())];
        return a.real() != b.real() ? a.real() > b.real() : a.imag() > b.imag();
    });

    shape.loops = std::move(loops);
    return shape;
}

} // namespace qftbx
