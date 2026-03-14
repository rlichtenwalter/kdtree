#ifndef KDTREE_POINT_IN_POLYGON_HPP
#define KDTREE_POINT_IN_POLYGON_HPP

#include "point.hpp"

namespace kdtree {
namespace detail {

template <class Point2d> double relative_location_2d(Point2d p0, Point2d p1, Point2d p2) {
  return ((p1[0] - p0[0]) * (p2[1] - p0[1]) - (p2[0] - p0[0]) * (p1[1] - p0[1]));
}

template <class Point2d, class RandomAccessIterator>
int winding_number_2d(Point2d const &point, RandomAccessIterator begin, RandomAccessIterator end) {
  int winding_number = 0;
  auto it = begin;
  while (it != end) {
    auto const &p0 = *it;
    auto p1 = it + 1 == end ? *begin : *(it + 1);
    if (p0[1] <= point[1]) {
      if (p1[1] > point[1]) {
        if (relative_location_2d(p0, p1, point) > 0) {
          ++winding_number;
        }
      }
    } else {
      if (p1[1] <= point[1]) {
        if (relative_location_2d(p0, p1, point) < 0) {
          --winding_number;
        }
      }
    }
    ++it;
  }
  return winding_number;
}

} // namespace detail

/**
 * @brief Test whether a 2D point lies inside a polygon defined by its vertices.
 *
 * Uses the winding number algorithm. Works for both convex and non-convex
 * simple polygons. The polygon is implicitly closed (an edge from the last
 * vertex back to the first is assumed).
 *
 * @param p The query point.
 * @param begin Iterator to the first vertex of the polygon.
 * @param end Iterator past the last vertex of the polygon.
 * @return True if @p p is strictly inside the polygon.
 */
template <class Point2d, class RandomAccessIterator>
bool point_in_polygon(Point2d const &p, RandomAccessIterator begin, RandomAccessIterator end) {
  return detail::winding_number_2d(p, begin, end) != 0;
}

} // namespace kdtree

#endif
