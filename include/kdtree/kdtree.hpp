#ifndef KDTREE_KDTREE_HPP
#define KDTREE_KDTREE_HPP

#include "point.hpp"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace kdtree {
namespace detail {

using dimension_type = std::size_t;
using depth_type = std::size_t;

// Compute the default leaf bucket threshold from the point type. Leaf buckets
// are scanned linearly during queries, so the threshold balances construction
// savings (fewer nth_element calls) against leaf-scan cost. The value is
// derived from how many points fit in two cache lines: larger points produce
// smaller thresholds, naturally disabling bucketing for very large point types.
template <class Point> constexpr std::size_t default_leaf_threshold() {
  constexpr std::size_t cache_line = 64;
  constexpr std::size_t points_per_line = cache_line / sizeof(Point);
  return std::max<std::size_t>(1, points_per_line * 2);
}

// Compile-time constant divisor guarantees optimal codegen: bitwise AND for
// power-of-two d, multiply-by-reciprocal for other values.
template <std::size_t d> inline dimension_type next_dimension(dimension_type dim) {
  return (dim + 1) % d;
}

// Runtime dimension cycling for non-performance-critical paths (e.g., printing).
inline dimension_type dimension(dimension_type dimensionality, depth_type depth) {
  return depth % dimensionality;
}

template <class Metric, class RandomAccessIterator, class Point, class DistanceType>
void update_minimum_distance(RandomAccessIterator it, Point const &p, DistanceType &mindist,
                             RandomAccessIterator &closest) {
  auto dist = Metric::distance(*it, p);
  if (dist < mindist) {
    mindist = dist;
    closest = it;
  }
}

// Compare pairs by their first element (distance). Used as the comparator for
// the kNN max-heap so the most distant candidate is at the front.
struct compare_by_distance {
  template <class T> bool operator()(T const &lhs, T const &rhs) const {
    return lhs.first < rhs.first;
  }
};

// Maintain a max-heap of the k closest candidates. If fewer than k have been
// found, insert unconditionally. Otherwise, replace the worst (front) only if
// the new point is closer. pop_heap moves the max to the back; we overwrite it
// and push_heap restores the invariant.
template <class Metric, class RandomAccessIterator, class Point, class Heap, class Compare>
void update_heap(RandomAccessIterator it, Point const &p, Heap &heap, std::size_t k,
                 Compare const &comp) {
  auto dist = Metric::distance(*it, p);
  if (heap.size() < k) {
    heap.emplace_back(dist, it);
    std::push_heap(heap.begin(), heap.end(), comp);
  } else if (dist < heap.front().first) {
    std::pop_heap(heap.begin(), heap.end(), comp);
    heap.back() = {dist, it};
    std::push_heap(heap.begin(), heap.end(), comp);
  }
}

template <class Point>
bool hypercube_contains(Point const &lower, Point const &upper, Point const &needle) {
  for (std::size_t i = 0; i < Point::dimensionality(); ++i) {
    if (needle[i] < lower[i] || needle[i] > upper[i]) {
      return false;
    }
  }
  return true;
}

template <std::size_t LeafThreshold, class RandomAccessIterator>
void make_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end, dimension_type dim) {
  using point_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
  using diff_t = typename std::iterator_traits<RandomAccessIterator>::difference_type;
  constexpr auto d = point_type::dimensionality();
  std::size_t n = static_cast<std::size_t>(end - begin);
  if (n > LeafThreshold) {
    RandomAccessIterator median = begin + static_cast<diff_t>(n / 2);
    auto comp = [dim](auto const &lhs, auto const &rhs) {
      return *(lhs.begin() + dim) < *(rhs.begin() + dim);
    };
    std::nth_element(begin, median, end, comp);
    make_kdtree_helper<LeafThreshold>(begin, median, next_dimension<d>(dim));
    make_kdtree_helper<LeafThreshold>(median + diff_t{1}, end, next_dimension<d>(dim));
  }
}

template <class RandomAccessIterator>
void print_kdtree_node_helper(std::ostream &os, RandomAccessIterator median, depth_type depth,
                              std::size_t node_count) {
  using coordinate_type = decltype(*(median->cbegin()));
  os << "(";
  std::copy(median->cbegin(), median->cend() - 1, std::ostream_iterator<coordinate_type>(os, ","));
  os << *(median->cend() - 1) << ") [d=" << depth << ",n=" << node_count << "]";
}

template <class RandomAccessIterator>
void print_kdtree_helper(std::ostream &os, RandomAccessIterator begin, RandomAccessIterator end,
                         depth_type depth) {
  using diff_t = typename std::iterator_traits<RandomAccessIterator>::difference_type;
  std::size_t n = static_cast<std::size_t>(end - begin);
  if (n > 0) {
    RandomAccessIterator median = begin + static_cast<diff_t>(n / 2);
    std::fill_n(std::ostream_iterator<std::string>(os), depth, " | ");
    print_kdtree_node_helper(os, median, depth, n);
    os << "\n";
    print_kdtree_helper(os, begin, median, depth + 1);
    print_kdtree_helper(os, median + diff_t{1}, end, depth + 1);
  }
}

template <class Metric, std::size_t LeafThreshold, class RandomAccessIterator, class Point,
          class DistanceType>
void nnsearch_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                            Point const &point, dimension_type dim, DistanceType &mindist,
                            RandomAccessIterator &closest) {
  using diff_t = typename std::iterator_traits<RandomAccessIterator>::difference_type;
  constexpr auto d = Point::dimensionality();
  std::size_t n = static_cast<std::size_t>(end - begin);
  if (n == 0) {
    return;
  }
  if (n <= LeafThreshold) {
    // Leaf bucket: linear scan over all elements.
    for (auto it = begin; it != end; ++it) {
      update_minimum_distance<Metric>(it, point, mindist, closest);
    }
    return;
  }
  RandomAccessIterator median = begin + static_cast<diff_t>(n / 2);
  // The median node is evaluated conditionally, gated on the same
  // pruning check as the opposite subtree. This is correct: the
  // median sits on the splitting hyperplane, so its distance
  // in the splitting dimension is Metric::prune_distance(gap). If
  // that value > mindist, the median cannot be closer than the current best.
  if (point[dim] <= (*median)[dim]) {
    nnsearch_kdtree_helper<Metric, LeafThreshold>(begin, median, point, next_dimension<d>(dim),
                                                  mindist, closest);
    auto gap = (*median)[dim] - point[dim];
    if (Metric::prune_distance(gap) <= mindist) {
      update_minimum_distance<Metric>(median, point, mindist, closest);
      nnsearch_kdtree_helper<Metric, LeafThreshold>(median + diff_t{1}, end, point,
                                                    next_dimension<d>(dim), mindist, closest);
    }
  } else {
    nnsearch_kdtree_helper<Metric, LeafThreshold>(median + diff_t{1}, end, point,
                                                  next_dimension<d>(dim), mindist, closest);
    auto gap = point[dim] - (*median)[dim];
    if (Metric::prune_distance(gap) <= mindist) {
      update_minimum_distance<Metric>(median, point, mindist, closest);
      nnsearch_kdtree_helper<Metric, LeafThreshold>(begin, median, point, next_dimension<d>(dim),
                                                    mindist, closest);
    }
  }
}

template <class Metric, std::size_t LeafThreshold, class RandomAccessIterator, class Point,
          class Heap, class Compare>
void nnsearch_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                            Point const &point, std::size_t k, dimension_type dim, Heap &heap,
                            Compare const &comp) {
  using diff_t = typename std::iterator_traits<RandomAccessIterator>::difference_type;
  constexpr auto d = Point::dimensionality();
  std::size_t n = static_cast<std::size_t>(end - begin);
  if (n == 0) {
    return;
  }
  if (n <= LeafThreshold) {
    // Leaf bucket: linear scan over all elements.
    for (auto it = begin; it != end; ++it) {
      update_heap<Metric>(it, point, heap, k, comp);
    }
    return;
  }
  RandomAccessIterator median = begin + static_cast<diff_t>(n / 2);
  // See comment in the 1-NN overload above regarding conditional
  // median evaluation. The heap.size() < k guard ensures we always
  // explore both subtrees until k candidates have been collected.
  if (point[dim] <= (*median)[dim]) {
    nnsearch_kdtree_helper<Metric, LeafThreshold>(begin, median, point, k, next_dimension<d>(dim),
                                                  heap, comp);
    auto gap = (*median)[dim] - point[dim];
    if (heap.size() < k || Metric::prune_distance(gap) <= heap.front().first) {
      update_heap<Metric>(median, point, heap, k, comp);
      nnsearch_kdtree_helper<Metric, LeafThreshold>(median + diff_t{1}, end, point, k,
                                                    next_dimension<d>(dim), heap, comp);
    }
  } else {
    nnsearch_kdtree_helper<Metric, LeafThreshold>(median + diff_t{1}, end, point, k,
                                                  next_dimension<d>(dim), heap, comp);
    auto gap = point[dim] - (*median)[dim];
    if (heap.size() < k || Metric::prune_distance(gap) <= heap.front().first) {
      update_heap<Metric>(median, point, heap, k, comp);
      nnsearch_kdtree_helper<Metric, LeafThreshold>(begin, median, point, k, next_dimension<d>(dim),
                                                    heap, comp);
    }
  }
}

template <std::size_t LeafThreshold, class RandomAccessIterator, class Point, class OutputIt>
void rangequery_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                              Point const &min, Point const &max, dimension_type dim,
                              OutputIt &out) {
  using diff_t = typename std::iterator_traits<RandomAccessIterator>::difference_type;
  constexpr auto d = Point::dimensionality();
  std::size_t n = static_cast<std::size_t>(end - begin);
  if (n == 0) {
    return;
  }
  if (n <= LeafThreshold) {
    // Leaf bucket: check each element against the full bounding box.
    for (auto it = begin; it != end; ++it) {
      if (hypercube_contains(min, max, *it)) {
        *out++ = it;
      }
    }
    return;
  }
  RandomAccessIterator median = begin + static_cast<diff_t>(n / 2);
  bool left_oob = min[dim] > (*median)[dim];
  bool right_oob = max[dim] < (*median)[dim];
  if (!left_oob) {
    rangequery_kdtree_helper<LeafThreshold>(begin, median, min, max, next_dimension<d>(dim), out);
  }
  if (!right_oob) {
    rangequery_kdtree_helper<LeafThreshold>(median + diff_t{1}, end, min, max,
                                            next_dimension<d>(dim), out);
  }
  if (!left_oob && !right_oob) {
    if (hypercube_contains(min, max, *median)) {
      *out++ = median;
    }
  }
}

template <std::size_t LeafThreshold, class RandomAccessIterator, class Point, class DistanceType,
          class OutputIt>
void radiusquery_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                               Point const &center, DistanceType squared_radius, dimension_type dim,
                               OutputIt &out) {
  using diff_t = typename std::iterator_traits<RandomAccessIterator>::difference_type;
  constexpr auto d = Point::dimensionality();
  std::size_t n = static_cast<std::size_t>(end - begin);
  if (n == 0) {
    return;
  }
  if (n <= LeafThreshold) {
    // Leaf bucket: check each element against the squared radius.
    for (auto it = begin; it != end; ++it) {
      if (squared_euclidean_distance(center, *it) <= squared_radius) {
        *out++ = it;
      }
    }
    return;
  }
  RandomAccessIterator median = begin + static_cast<diff_t>(n / 2);

  if (squared_euclidean_distance(center, *median) <= squared_radius) {
    *out++ = median;
  }

  if (center[dim] <= (*median)[dim]) {
    radiusquery_kdtree_helper<LeafThreshold>(begin, median, center, squared_radius,
                                             next_dimension<d>(dim), out);
    auto gap = (*median)[dim] - center[dim];
    if (gap * gap <= squared_radius) {
      radiusquery_kdtree_helper<LeafThreshold>(median + diff_t{1}, end, center, squared_radius,
                                               next_dimension<d>(dim), out);
    }
  } else {
    radiusquery_kdtree_helper<LeafThreshold>(median + diff_t{1}, end, center, squared_radius,
                                             next_dimension<d>(dim), out);
    auto gap = center[dim] - (*median)[dim];
    if (gap * gap <= squared_radius) {
      radiusquery_kdtree_helper<LeafThreshold>(begin, median, center, squared_radius,
                                               next_dimension<d>(dim), out);
    }
  }
}

} // namespace detail

// --- Public API ---

/**
 * @brief Build a k-d tree in-place over the range [begin, end).
 *
 * Rearranges elements using std::nth_element so that the midpoint of each
 * subrange is the median along the cycling splitting dimension. The resulting
 * layout is an implicit balanced binary tree: no auxiliary data structure is
 * allocated.
 *
 * @tparam LeafThreshold Bucket size below which recursion stops (0 = auto).
 *         The default auto-selects based on point size and cache line width.
 *         Use 1 for classic fully-recursive construction. All query functions
 *         must use the same LeafThreshold that was used for construction;
 *         mismatched values produce undefined results. The default (0) is
 *         consistent across all functions for the same point type.
 * @param begin Iterator to the first element.
 * @param end Iterator past the last element.
 */
template <std::size_t LeafThreshold = 0, class RandomAccessIterator>
void make_kdtree(RandomAccessIterator begin, RandomAccessIterator end) {
  using iterator_tag = typename std::iterator_traits<RandomAccessIterator>::iterator_category;
  using point_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
  static_assert(std::is_convertible<iterator_tag, std::random_access_iterator_tag>::value,
                "kdtree::make_kdtree only accepts random access iterators or raw pointers.\n");
  constexpr std::size_t threshold =
      (LeafThreshold == 0) ? detail::default_leaf_threshold<point_type>() : LeafThreshold;
  detail::make_kdtree_helper<threshold>(begin, end, 0);
}

/**
 * @brief Print the k-d tree structure to an output stream.
 *
 * Produces a human-readable indented representation showing each node's
 * coordinates, splitting depth, and subtree size.
 */
template <class RandomAccessIterator>
void print_kdtree(std::ostream &os, RandomAccessIterator begin, RandomAccessIterator end) {
  detail::print_kdtree_helper(os, begin, end, 0);
}

/**
 * @brief Find the nearest neighbor in a constructed k-d tree.
 *
 * @tparam LeafThreshold Must match the value used during construction (0 = auto).
 *         Mismatched values produce undefined results.
 * @param begin Iterator to the first element of the constructed tree.
 * @param end Iterator past the last element.
 * @param point The query point.
 * @return Iterator to the nearest neighbor, or @p end if the range is empty.
 */
template <typename Metric = squared_euclidean_metric, std::size_t LeafThreshold = 0,
          class RandomAccessIterator, class Point>
RandomAccessIterator nnsearch_kdtree(RandomAccessIterator begin, RandomAccessIterator end,
                                     Point const &point) {
  using iterator_tag = typename std::iterator_traits<RandomAccessIterator>::iterator_category;
  using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
  static_assert(std::is_convertible<iterator_tag, std::random_access_iterator_tag>::value,
                "kdtree::nnsearch_kdtree only accepts random access iterators or raw pointers.\n");
  static_assert(
      std::is_convertible<Point, value_type>::value,
      "kdtree::nnsearch_kdtree requires Point convertible to the iterator's value_type.\n");
  if (begin == end) {
    return end;
  }
  constexpr std::size_t threshold =
      (LeafThreshold == 0) ? detail::default_leaf_threshold<value_type>() : LeafThreshold;
  using distance_type = decltype(Metric::distance(*begin, point));
  distance_type distance = std::numeric_limits<distance_type>::max();
  RandomAccessIterator location = end;
  detail::nnsearch_kdtree_helper<Metric, threshold>(begin, end, point, 0, distance, location);
  return location;
}

/**
 * @brief Find the k nearest neighbors in a constructed k-d tree.
 *
 * @tparam LeafThreshold Must match the value used during construction (0 = auto).
 *         Mismatched values produce undefined results.
 * @param begin Iterator to the first element of the constructed tree.
 * @param end Iterator past the last element.
 * @param point The query point.
 * @param k Number of nearest neighbors to find.
 * @return Vector of iterators to the k nearest neighbors (unordered). If the
 *         tree contains fewer than k points, all points are returned.
 */
template <typename Metric = squared_euclidean_metric, std::size_t LeafThreshold = 0,
          class RandomAccessIterator, class Point>
std::vector<RandomAccessIterator> nnsearch_kdtree(RandomAccessIterator begin,
                                                  RandomAccessIterator end, Point const &point,
                                                  std::size_t k) {
  using iterator_tag = typename std::iterator_traits<RandomAccessIterator>::iterator_category;
  using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
  static_assert(std::is_convertible<iterator_tag, std::random_access_iterator_tag>::value,
                "kdtree::nnsearch_kdtree only accepts random access iterators or raw pointers.\n");
  static_assert(
      std::is_convertible<Point, value_type>::value,
      "kdtree::nnsearch_kdtree requires Point convertible to the iterator's value_type.\n");
  if (begin == end || k == 0) {
    return {};
  }
  constexpr std::size_t threshold =
      (LeafThreshold == 0) ? detail::default_leaf_threshold<value_type>() : LeafThreshold;
  using distance_type = decltype(Metric::distance(*begin, point));
  using heap_entry = std::pair<distance_type, RandomAccessIterator>;
  // Max-heap: largest distance at front, so we can efficiently replace
  // the worst candidate when a closer point is found.
  std::vector<heap_entry> heap;
  heap.reserve(k);
  detail::nnsearch_kdtree_helper<Metric, threshold>(begin, end, point, k, 0, heap,
                                                    detail::compare_by_distance{});
  // Extract iterators directly from the heap vector — O(k) instead of
  // O(k log k) priority queue drain.
  std::vector<RandomAccessIterator> result;
  result.reserve(heap.size());
  for (auto const &entry : heap) {
    result.push_back(entry.second);
  }
  return result;
}

/**
 * @brief Search for an exact match in a constructed k-d tree.
 *
 * Finds the nearest neighbor and returns it only if it is exactly equal
 * to @p point. Returns @p end if no exact match exists.
 *
 * @tparam LeafThreshold Must match the value used during construction (0 = auto).
 *         Mismatched values produce undefined results.
 */
template <std::size_t LeafThreshold = 0, class RandomAccessIterator, class Point>
RandomAccessIterator search_kdtree(RandomAccessIterator begin, RandomAccessIterator end,
                                   Point const &point) {
  using iterator_tag = typename std::iterator_traits<RandomAccessIterator>::iterator_category;
  using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
  static_assert(std::is_convertible<iterator_tag, std::random_access_iterator_tag>::value,
                "kdtree::search_kdtree only accepts random access iterators or raw pointers.\n");
  static_assert(std::is_convertible<Point, value_type>::value,
                "kdtree::search_kdtree requires Point convertible to the iterator's value_type.\n");
  RandomAccessIterator it =
      nnsearch_kdtree<squared_euclidean_metric, LeafThreshold>(begin, end, point);
  if (it == end) {
    return end;
  }
  return point == *it ? it : end;
}

/**
 * @brief Find all points within an axis-aligned bounding box (output iterator).
 *
 * Writes iterators to matching points through @p out. Callers can reuse
 * storage across repeated queries by clearing a vector and passing
 * std::back_inserter.
 *
 * @tparam LeafThreshold Must match the value used during construction (0 = auto).
 *         Mismatched values produce undefined results.
 * @param min Lower corner of the bounding box (inclusive).
 * @param max Upper corner of the bounding box (inclusive).
 * @param out Output iterator receiving iterators to matching points.
 * @return The output iterator after all results have been written.
 */
template <std::size_t LeafThreshold = 0, class RandomAccessIterator, class Point, class OutputIt>
OutputIt rangequery_kdtree(RandomAccessIterator begin, RandomAccessIterator end, Point const &min,
                           Point const &max, OutputIt out) {
  using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
  constexpr std::size_t threshold =
      (LeafThreshold == 0) ? detail::default_leaf_threshold<value_type>() : LeafThreshold;
  detail::rangequery_kdtree_helper<threshold>(begin, end, min, max, 0, out);
  return out;
}

/**
 * @brief Find all points within a given radius of a center point (output iterator).
 *
 * @tparam LeafThreshold Must match the value used during construction (0 = auto).
 *         Mismatched values produce undefined results.
 * @param point The center of the search sphere.
 * @param radius Search radius (must be positive; returns nothing if <= 0).
 * @param out Output iterator receiving iterators to matching points.
 * @return The output iterator after all results have been written.
 */
template <std::size_t LeafThreshold = 0, class RandomAccessIterator, class Point, class OutputIt>
OutputIt radiusquery_kdtree(RandomAccessIterator begin, RandomAccessIterator end,
                            Point const &point, typename Point::coordinate_type radius,
                            OutputIt out) {
  using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
  constexpr std::size_t threshold =
      (LeafThreshold == 0) ? detail::default_leaf_threshold<value_type>() : LeafThreshold;
  if (radius > 0) {
    auto squared_radius = radius * radius;
    detail::radiusquery_kdtree_helper<threshold>(begin, end, point, squared_radius, 0, out);
  }
  return out;
}

/** @brief Find all points within an axis-aligned bounding box.
 *  @return Vector of iterators to matching points.
 *  @see rangequery_kdtree(begin, end, min, max, out) for the allocation-free overload.
 */
template <std::size_t LeafThreshold = 0, class RandomAccessIterator, class Point>
std::vector<RandomAccessIterator> rangequery_kdtree(RandomAccessIterator begin,
                                                    RandomAccessIterator end, Point const &min,
                                                    Point const &max) {
  std::vector<RandomAccessIterator> locations;
  rangequery_kdtree<LeafThreshold>(begin, end, min, max, std::back_inserter(locations));
  return locations;
}

/** @brief Find all points within a given radius of a center point.
 *  @return Vector of iterators to matching points.
 *  @see radiusquery_kdtree(begin, end, point, radius, out) for the allocation-free overload.
 */
template <std::size_t LeafThreshold = 0, class RandomAccessIterator, class Point>
std::vector<RandomAccessIterator> radiusquery_kdtree(RandomAccessIterator begin,
                                                     RandomAccessIterator end, Point const &point,
                                                     typename Point::coordinate_type radius) {
  std::vector<RandomAccessIterator> locations;
  radiusquery_kdtree<LeafThreshold>(begin, end, point, radius, std::back_inserter(locations));
  return locations;
}

} // namespace kdtree

#endif
