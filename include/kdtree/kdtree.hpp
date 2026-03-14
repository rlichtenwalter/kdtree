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

template <class RandomAccessIterator, class Point, class DistanceType>
void update_minimum_distance(RandomAccessIterator it, Point const &p, DistanceType &mindist,
                             RandomAccessIterator &closest) {
  auto dist = squared_euclidean_distance(*it, p);
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
template <class RandomAccessIterator, class Point, class Heap, class Compare>
void update_heap(RandomAccessIterator it, Point const &p, Heap &heap, std::size_t k,
                 Compare const &comp) {
  auto dist = squared_euclidean_distance(*it, p);
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
  constexpr auto d = point_type::dimensionality();
  std::size_t n = end - begin;
  if (n > LeafThreshold) {
    RandomAccessIterator median = begin + (n / 2);
    auto comp = [dim](auto const &lhs, auto const &rhs) {
      return *(lhs.begin() + dim) < *(rhs.begin() + dim);
    };
    std::nth_element(begin, median, end, comp);
    make_kdtree_helper<LeafThreshold>(begin, median, next_dimension<d>(dim));
    make_kdtree_helper<LeafThreshold>(median + 1, end, next_dimension<d>(dim));
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
  std::size_t n = end - begin;
  if (n > 0) {
    RandomAccessIterator median = begin + (n / 2);
    std::fill_n(std::ostream_iterator<std::string>(os), depth, " | ");
    print_kdtree_node_helper(os, median, depth, n);
    os << "\n";
    print_kdtree_helper(os, begin, median, depth + 1);
    print_kdtree_helper(os, median + 1, end, depth + 1);
  }
}

template <std::size_t LeafThreshold, class RandomAccessIterator, class Point, class DistanceType>
void nnsearch_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                            Point const &point, dimension_type dim, DistanceType &mindist,
                            RandomAccessIterator &closest) {
  constexpr auto d = Point::dimensionality();
  std::size_t n = end - begin;
  if (n == 0) {
    return;
  }
  if (n <= LeafThreshold) {
    // Leaf bucket: linear scan over all elements.
    for (auto it = begin; it != end; ++it) {
      update_minimum_distance(it, point, mindist, closest);
    }
    return;
  }
  RandomAccessIterator median = begin + (n / 2);
  // The median node is evaluated conditionally, gated on the same
  // pruning check as the opposite subtree. This is correct: the
  // median sits on the splitting hyperplane, so its squared distance
  // in the splitting dimension is exactly gap^2. If gap^2 > mindist,
  // the median cannot be closer than the current best.
  if (point[dim] <= (*median)[dim]) {
    nnsearch_kdtree_helper<LeafThreshold>(begin, median, point, next_dimension<d>(dim), mindist,
                                          closest);
    auto gap = (*median)[dim] - point[dim];
    if (gap * gap <= mindist) {
      update_minimum_distance(median, point, mindist, closest);
      nnsearch_kdtree_helper<LeafThreshold>(median + 1, end, point, next_dimension<d>(dim), mindist,
                                            closest);
    }
  } else {
    nnsearch_kdtree_helper<LeafThreshold>(median + 1, end, point, next_dimension<d>(dim), mindist,
                                          closest);
    auto gap = point[dim] - (*median)[dim];
    if (gap * gap <= mindist) {
      update_minimum_distance(median, point, mindist, closest);
      nnsearch_kdtree_helper<LeafThreshold>(begin, median, point, next_dimension<d>(dim), mindist,
                                            closest);
    }
  }
}

template <std::size_t LeafThreshold, class RandomAccessIterator, class Point, class Heap,
          class Compare>
void nnsearch_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                            Point const &point, std::size_t k, dimension_type dim, Heap &heap,
                            Compare const &comp) {
  constexpr auto d = Point::dimensionality();
  std::size_t n = end - begin;
  if (n == 0) {
    return;
  }
  if (n <= LeafThreshold) {
    // Leaf bucket: linear scan over all elements.
    for (auto it = begin; it != end; ++it) {
      update_heap(it, point, heap, k, comp);
    }
    return;
  }
  RandomAccessIterator median = begin + (n / 2);
  // See comment in the 1-NN overload above regarding conditional
  // median evaluation. The heap.size() < k guard ensures we always
  // explore both subtrees until k candidates have been collected.
  if (point[dim] <= (*median)[dim]) {
    nnsearch_kdtree_helper<LeafThreshold>(begin, median, point, k, next_dimension<d>(dim), heap,
                                          comp);
    auto gap = (*median)[dim] - point[dim];
    if (heap.size() < k || gap * gap <= heap.front().first) {
      update_heap(median, point, heap, k, comp);
      nnsearch_kdtree_helper<LeafThreshold>(median + 1, end, point, k, next_dimension<d>(dim), heap,
                                            comp);
    }
  } else {
    nnsearch_kdtree_helper<LeafThreshold>(median + 1, end, point, k, next_dimension<d>(dim), heap,
                                          comp);
    auto gap = point[dim] - (*median)[dim];
    if (heap.size() < k || gap * gap <= heap.front().first) {
      update_heap(median, point, heap, k, comp);
      nnsearch_kdtree_helper<LeafThreshold>(begin, median, point, k, next_dimension<d>(dim), heap,
                                            comp);
    }
  }
}

template <std::size_t LeafThreshold, class RandomAccessIterator, class Point, class OutputIt>
void rangequery_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                              Point const &min, Point const &max, dimension_type dim,
                              OutputIt &out) {
  constexpr auto d = Point::dimensionality();
  std::size_t n = end - begin;
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
  RandomAccessIterator median = begin + (n / 2);
  bool left_oob = min[dim] > (*median)[dim];
  bool right_oob = max[dim] < (*median)[dim];
  if (!left_oob) {
    rangequery_kdtree_helper<LeafThreshold>(begin, median, min, max, next_dimension<d>(dim), out);
  }
  if (!right_oob) {
    rangequery_kdtree_helper<LeafThreshold>(median + 1, end, min, max, next_dimension<d>(dim), out);
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
  constexpr auto d = Point::dimensionality();
  std::size_t n = end - begin;
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
  RandomAccessIterator median = begin + (n / 2);

  if (squared_euclidean_distance(center, *median) <= squared_radius) {
    *out++ = median;
  }

  auto gap = center[dim] - (*median)[dim];

  if (gap <= 0) {
    radiusquery_kdtree_helper<LeafThreshold>(begin, median, center, squared_radius,
                                             next_dimension<d>(dim), out);
    if (gap * gap <= squared_radius) {
      radiusquery_kdtree_helper<LeafThreshold>(median + 1, end, center, squared_radius,
                                               next_dimension<d>(dim), out);
    }
  } else {
    radiusquery_kdtree_helper<LeafThreshold>(median + 1, end, center, squared_radius,
                                             next_dimension<d>(dim), out);
    if (gap * gap <= squared_radius) {
      radiusquery_kdtree_helper<LeafThreshold>(begin, median, center, squared_radius,
                                               next_dimension<d>(dim), out);
    }
  }
}

} // namespace detail

// --- Public API ---

// Build a k-d tree in-place over the range [begin, end). LeafThreshold controls
// the bucket size at which recursion stops and leaves are left unsorted. The
// default (0) auto-selects a threshold based on point size and cache line width.
// Use LeafThreshold=1 for the classic fully-recursive construction.
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

template <class RandomAccessIterator>
void print_kdtree(std::ostream &os, RandomAccessIterator begin, RandomAccessIterator end) {
  detail::print_kdtree_helper(os, begin, end, 0);
}

// Query functions accept an optional LeafThreshold template parameter that must
// match the value used during construction. The default (0) auto-selects the
// same threshold that make_kdtree uses by default.

template <std::size_t LeafThreshold = 0, class RandomAccessIterator, class Point>
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
  using coordinate_type = typename Point::coordinate_type;
  coordinate_type distance = std::numeric_limits<coordinate_type>::max();
  RandomAccessIterator location = end;
  detail::nnsearch_kdtree_helper<threshold>(begin, end, point, 0, distance, location);
  return location;
}

template <std::size_t LeafThreshold = 0, class RandomAccessIterator, class Point>
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
  constexpr std::size_t threshold =
      (LeafThreshold == 0) ? detail::default_leaf_threshold<value_type>() : LeafThreshold;
  using coordinate_type = typename Point::coordinate_type;
  using heap_entry = std::pair<coordinate_type, RandomAccessIterator>;
  // Max-heap: largest distance at front, so we can efficiently replace
  // the worst candidate when a closer point is found.
  std::vector<heap_entry> heap;
  heap.reserve(k);
  detail::nnsearch_kdtree_helper<threshold>(begin, end, point, k, 0, heap,
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

template <std::size_t LeafThreshold = 0, class RandomAccessIterator, class Point>
RandomAccessIterator search_kdtree(RandomAccessIterator begin, RandomAccessIterator end,
                                   Point const &point) {
  using iterator_tag = typename std::iterator_traits<RandomAccessIterator>::iterator_category;
  using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
  static_assert(std::is_convertible<iterator_tag, std::random_access_iterator_tag>::value,
                "kdtree::search_kdtree only accepts random access iterators or raw pointers.\n");
  static_assert(std::is_convertible<Point, value_type>::value,
                "kdtree::search_kdtree requires Point convertible to the iterator's value_type.\n");
  RandomAccessIterator it = nnsearch_kdtree<LeafThreshold>(begin, end, point);
  if (it == end) {
    return end;
  }
  return point == *it ? it : end;
}

// Output-iterator overloads: write results directly to the caller's output
// iterator, avoiding internal allocation. Callers can reuse storage across
// repeated queries by clearing a vector and passing std::back_inserter.
template <std::size_t LeafThreshold = 0, class RandomAccessIterator, class Point, class OutputIt>
OutputIt rangequery_kdtree(RandomAccessIterator begin, RandomAccessIterator end, Point const &min,
                           Point const &max, OutputIt out) {
  using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
  constexpr std::size_t threshold =
      (LeafThreshold == 0) ? detail::default_leaf_threshold<value_type>() : LeafThreshold;
  detail::rangequery_kdtree_helper<threshold>(begin, end, min, max, 0, out);
  return out;
}

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

// Convenience overloads: return results in a new vector. For repeated queries,
// prefer the output-iterator overloads above to avoid per-call allocation.
template <std::size_t LeafThreshold = 0, class RandomAccessIterator, class Point>
std::vector<RandomAccessIterator> rangequery_kdtree(RandomAccessIterator begin,
                                                    RandomAccessIterator end, Point const &min,
                                                    Point const &max) {
  std::vector<RandomAccessIterator> locations;
  rangequery_kdtree<LeafThreshold>(begin, end, min, max, std::back_inserter(locations));
  return locations;
}

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
