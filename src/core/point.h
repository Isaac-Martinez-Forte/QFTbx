#ifndef QFTBX_POINT_H
#define QFTBX_POINT_H

namespace qftbx {

/**
 * @brief A point of a plane, as a pair of named coordinates.
 *
 * Plain data members rather than accessors, like qftbx::Range: this is an
 * aggregate, and getters over two doubles buy nothing.
 *
 * WHY x AND y AND NOT phase AND magnitude, which is what a boundary trace
 * actually carries. In the core they would be the honest names - the
 * boundary engine computes over a phase x magnitude grid, and the cartesian
 * "Nyquist" projection was dropped from the algorithms (the thesis tried it
 * and discarded it, secs. 4.5-4.6). But MainWindow::showLoopDiagrams
 * reinterprets a Trace as the complex plane for the Nyquist view: it
 * converts each point to (Re, Im) and hands it back inside a fabricated
 * BoundaryData, whose tell is that it also has to pass empty bucket rows
 * because "this view is only drawn, never classified".
 *
 * So phase/magnitude would be a lie at exactly one place. The fix is not a
 * different name here: it is that the loop viewer should take the traces it
 * draws instead of a BoundaryData built to look like something it is not,
 * and then this type can say what it means. Recorded in the plan; not
 * smuggled into a type change.
 */
struct Point
{
    double x = 0.0;
    double y = 0.0;

    Point() = default;

    Point(double xCoordinate, double yCoordinate) : x(xCoordinate), y(yCoordinate) {}

    /**
     * @brief Exact equality, coordinate by coordinate.
     *
     * QPointF's was FUZZY (qFuzzyCompare), so this is not the same
     * operator. It is the honest one for a value type, and it is what the
     * one caller wants: BoundaryUnion1D erases the point it just picked out
     * of its own container, so it is looking for that element and not for
     * something near it - which fuzzy equality could have found first among
     * clustered trace points, erasing a different one. The boundary goldens
     * are what confirm the difference does not show up on real data.
     */
    bool operator==(const Point & other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Point & other) const
    {
        return !(*this == other);
    }
};

} // namespace qftbx

#endif // QFTBX_POINT_H
