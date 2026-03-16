#ifndef KDTREE_CONVEX_POLYGON_HPP
#define KDTREE_CONVEX_POLYGON_HPP

#include "point.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace kdtree {
namespace detail {

template <typename T> double relative_location(point<T, 2> p0, point<T, 2> p1, point<T, 2> p2) {
  return ((p1[0] - p0[0]) * (p2[1] - p0[1]) - (p2[0] - p0[0]) * (p1[1] - p0[1]));
}

} // namespace detail

/**
 * @brief A convex polygon in 2D space.
 *
 * Stores vertices in counterclockwise order. Clockwise input is automatically
 * normalized. Construction validates convexity and rejects degenerate cases
 * (fewer than 3 vertices, collinear vertices, non-convex vertex sequences).
 *
 * Provides point-in-polygon containment testing via the winding number
 * algorithm.
 *
 * @tparam T Coordinate type of the polygon's vertices.
 */
template <typename T> class convex_polygon {
public:
  using point = kdtree::point<T, 2>;

private:
  using storage_type = std::vector<point>;
  using size_type = typename storage_type::size_type;
  std::vector<point> _points;
  void build_and_verify();

public:
  using iterator = typename storage_type::iterator;
  using const_iterator = typename storage_type::const_iterator;
  using reverse_iterator = typename storage_type::reverse_iterator;
  using const_reverse_iterator = typename storage_type::const_reverse_iterator;

  convex_polygon() : _points(1, point(T(0), T(0))) {}

  /** @brief Construct from a brace-enclosed list of vertices. */
  convex_polygon(std::initializer_list<point> pts) : _points(pts) { build_and_verify(); }
  template <class InputIterator>
  convex_polygon(InputIterator begin, InputIterator end) : _points(begin, end) {
    build_and_verify();
  }
  /** @brief Return the number of vertices. */
  size_type size() const noexcept { return _points.size() - 1; }

  /** @brief Test whether a point lies inside the polygon (winding number). */
  bool contains(point const &p) const noexcept;

  iterator begin() noexcept { return _points.begin(); }
  const_iterator begin() const noexcept { return _points.begin(); }
  const_iterator cbegin() const noexcept { return _points.cbegin(); }
  iterator end() noexcept { return _points.end() - 1; }
  const_iterator end() const noexcept { return _points.end() - 1; }
  const_iterator cend() const noexcept { return _points.cend() - 1; }
  reverse_iterator rbegin() noexcept { return _points.rbegin() + 1; }
  const_reverse_iterator rbegin() const noexcept { return _points.rbegin() + 1; }
  const_reverse_iterator crbegin() const noexcept { return _points.crbegin() + 1; }
  reverse_iterator rend() noexcept { return _points.rend(); }
  const_reverse_iterator rend() const noexcept { return _points.rend(); }
  const_reverse_iterator crend() const noexcept { return _points.crend(); }
};

template <typename T> void convex_polygon<T>::build_and_verify() {
  auto generate_error_message = [](size_type count) {
    std::stringstream error_ss("kdtree::convex_polygon requires a minimum of 3 points but only ");
    error_ss << count << " were observed";
    return error_ss.str();
  };
  if (_points.size() < 3) {
    throw std::out_of_range(generate_error_message(_points.size()));
  }
  _points.reserve(_points.size() + 1);
  _points.push_back(_points.front());
  bool counterclockwise = true;
  bool clockwise = true;
  bool colinear = true;
  auto it = _points.cbegin();
  while (it < _points.cend() - 2) {
    auto result = detail::relative_location(*it, *(it + 1), *(it + 2));
    if (result < 0) {
      counterclockwise = false;
      colinear = false;
    } else if (result > 0) {
      clockwise = false;
      colinear = false;
    }
    ++it;
  }
  if (colinear) {
    throw std::runtime_error("points provided to kdtree::convex_polygon were all colinear, but "
                             "this does not result in a valid polygon in Euclidean space");
  }
  if (!counterclockwise && !clockwise) {
    throw std::runtime_error(
        "points provided to kdtree::convex_polygon must be given in either counterclockwise or "
        "clockwise order and form a valid polygon in Euclidean space");
  }
  if (clockwise) {
    std::reverse(_points.begin(), _points.end());
  }
}

template <typename T>
bool convex_polygon<T>::contains(convex_polygon<T>::point const &p) const noexcept {
  if (size() == 0) {
    return false;
  } else {
    int winding_number = 0;
    auto it = cbegin();
    while (it != cend()) {
      auto const &p0 = *it;
      auto const &p1 = *(it + 1);
      if (p0[1] <= p[1]) {
        if (p1[1] > p[1]) {
          if (detail::relative_location(p0, p1, p) > 0) {
            ++winding_number;
          }
        }
      } else {
        if (p1[1] <= p[1]) {
          if (detail::relative_location(p0, p1, p) < 0) {
            --winding_number;
          }
        }
      }
      ++it;
    }
    return winding_number != 0;
  }
}

template <typename T>
std::ostream &operator<<(std::ostream &os, kdtree::convex_polygon<T> const &polygon) {
  os << '[';
  auto it = polygon.begin();
  while (it != polygon.end() - 1) {
    os << *it << ',';
    ++it;
  }
  os << *it << ']';
  return os;
}

template <typename T>
std::istream &operator>>(std::istream &is, kdtree::convex_polygon<T> &polygon) {
  auto generate_error_message = [](char c_expected, char c_given) {
    std::stringstream error_ss("invalid format for kdtree::polygon: expected '",
                               std::ios_base::in | std::ios_base::out | std::ios_base::app);
    error_ss << c_expected << "' but saw '";
    if (std::isprint(c_given)) {
      error_ss << c_given << '\'';
    } else if (c_given == '\t') {
      error_ss << "\\t";
    } else if (c_given == '\n') {
      error_ss << "\\n";
    } else {
      error_ss << "\\" << std::hex
               << static_cast<unsigned int>(static_cast<unsigned char>(c_given));
    }
    error_ss << '\'';
    return error_ss.str();
  };
  char c;
  is >> c;
  if (c != '[') {
    throw std::range_error(generate_error_message('[', c));
  }
  using point = typename kdtree::convex_polygon<T>::point;
  std::vector<point> points;
  while (is.peek() != ']') {
    point p;
    is >> p;
    points.emplace_back(p);
    if (is.peek() == ',') {
      is >> c;
      if (is.peek() == ']') {
        throw std::range_error(
            "invalid format for kdtree::polygon: expected kdtree::point but saw ']'");
      }
    }
  }
  is >> c;
  if (c != ']') {
    // it should be impossible to reach this
    throw std::range_error(generate_error_message(']', c));
  }
  if (!points.empty()) {
    polygon = kdtree::convex_polygon<T>(points.begin(), points.end());
  }
  return is;
}
} // namespace kdtree

#endif
