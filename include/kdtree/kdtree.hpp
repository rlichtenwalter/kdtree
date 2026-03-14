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

template <class RandomAccessIterator>
void make_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end, depth_type depth) {
  using point_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
  dimension_type dim = dimension(point_type::dimensionality(), depth);
  std::size_t n = end - begin;
  if (n > 1) {
    RandomAccessIterator median = begin + (n / 2);
    auto comp = [dim](auto const &lhs, auto const &rhs) {
      return *(lhs.begin() + dim) < *(rhs.begin() + dim);
    };
    std::nth_element(begin, median, end, comp);
    make_kdtree_helper(begin, median, depth + 1);
    make_kdtree_helper(median + 1, end, depth + 1);
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

template <class RandomAccessIterator, class Point, class DistanceType>
void nnsearch_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                            Point const &point, depth_type depth, DistanceType &mindist,
                            RandomAccessIterator &closest) {
  dimension_type dim = dimension(Point::dimensionality(), depth);
  std::size_t n = end - begin;
  if (n > 0) {
    RandomAccessIterator median = begin + (n / 2);
    if (n > 1) {
      // The median node is evaluated conditionally, gated on the same
      // pruning check as the opposite subtree. This is correct: the
      // median sits on the splitting hyperplane, so its squared distance
      // in the splitting dimension is exactly gap^2. If gap^2 > mindist,
      // the median cannot be closer than the current best.
      if (point[dim] <= (*median)[dim]) {
        nnsearch_kdtree_helper(begin, median, point, depth + 1, mindist, closest);
        auto gap = (*median)[dim] - point[dim];
        if (gap * gap <= mindist) {
          update_minimum_distance(median, point, mindist, closest);
          nnsearch_kdtree_helper(median + 1, end, point, depth + 1, mindist, closest);
        }
      } else {
        nnsearch_kdtree_helper(median + 1, end, point, depth + 1, mindist, closest);
        auto gap = point[dim] - (*median)[dim];
        if (gap * gap <= mindist) {
          update_minimum_distance(median, point, mindist, closest);
          nnsearch_kdtree_helper(begin, median, point, depth + 1, mindist, closest);
        }
      }
    } else if (n == 1) {
      update_minimum_distance(median, point, mindist, closest);
    }
  }
}

template <class RandomAccessIterator, class Point, class Heap, class Compare>
void nnsearch_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                            Point const &point, std::size_t k, depth_type depth, Heap &heap,
                            Compare const &comp) {
  dimension_type dim = dimension(Point::dimensionality(), depth);
  std::size_t n = end - begin;
  if (n > 0) {
    RandomAccessIterator median = begin + (n / 2);
    if (n > 1) {
      // See comment in the 1-NN overload above regarding conditional
      // median evaluation. The heap.size() < k guard ensures we always
      // explore both subtrees until k candidates have been collected.
      if (point[dim] <= (*median)[dim]) {
        nnsearch_kdtree_helper(begin, median, point, k, depth + 1, heap, comp);
        auto gap = (*median)[dim] - point[dim];
        if (heap.size() < k || gap * gap <= heap.front().first) {
          update_heap(median, point, heap, k, comp);
          nnsearch_kdtree_helper(median + 1, end, point, k, depth + 1, heap, comp);
        }
      } else {
        nnsearch_kdtree_helper(median + 1, end, point, k, depth + 1, heap, comp);
        auto gap = point[dim] - (*median)[dim];
        if (heap.size() < k || gap * gap <= heap.front().first) {
          update_heap(median, point, heap, k, comp);
          nnsearch_kdtree_helper(begin, median, point, k, depth + 1, heap, comp);
        }
      }
    } else if (n == 1) {
      update_heap(median, point, heap, k, comp);
    }
  }
}

template <class RandomAccessIterator, class Point, class OutputIt>
void rangequery_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                              Point const &min, Point const &max, depth_type depth, OutputIt &out) {
  dimension_type dim = dimension(Point::dimensionality(), depth);
  std::size_t n = end - begin;
  if (n > 0) {
    RandomAccessIterator median = begin + (n / 2);
    bool left_oob = min[dim] > (*median)[dim];
    bool right_oob = max[dim] < (*median)[dim];
    if (!left_oob) {
      rangequery_kdtree_helper(begin, median, min, max, depth + 1, out);
    }
    if (!right_oob) {
      rangequery_kdtree_helper(median + 1, end, min, max, depth + 1, out);
    }
    if (!left_oob && !right_oob) {
      if (hypercube_contains(min, max, *median)) {
        *out++ = median;
      }
    }
  }
}

template <class RandomAccessIterator, class Point, class DistanceType, class OutputIt>
void radiusquery_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                               Point const &center, DistanceType squared_radius, depth_type depth,
                               OutputIt &out) {
  std::size_t n = end - begin;
  if (n == 0) {
    return;
  }
  dimension_type dim = dimension(Point::dimensionality(), depth);
  RandomAccessIterator median = begin + (n / 2);

  if (squared_euclidean_distance(center, *median) <= squared_radius) {
    *out++ = median;
  }

  if (n == 1) {
    return;
  }

  auto gap = center[dim] - (*median)[dim];

  if (gap <= 0) {
    radiusquery_kdtree_helper(begin, median, center, squared_radius, depth + 1, out);
    if (gap * gap <= squared_radius) {
      radiusquery_kdtree_helper(median + 1, end, center, squared_radius, depth + 1, out);
    }
  } else {
    radiusquery_kdtree_helper(median + 1, end, center, squared_radius, depth + 1, out);
    if (gap * gap <= squared_radius) {
      radiusquery_kdtree_helper(begin, median, center, squared_radius, depth + 1, out);
    }
  }
}

} // namespace detail

// --- Public API ---

template <class RandomAccessIterator>
void make_kdtree(RandomAccessIterator begin, RandomAccessIterator end) {
  using iterator_tag = typename std::iterator_traits<RandomAccessIterator>::iterator_category;
  static_assert(std::is_convertible<iterator_tag, std::random_access_iterator_tag>::value,
                "kdtree::make_kdtree only accepts random access iterators or raw pointers.\n");
  detail::make_kdtree_helper(begin, end, 0);
}

template <class RandomAccessIterator>
void print_kdtree(std::ostream &os, RandomAccessIterator begin, RandomAccessIterator end) {
  detail::print_kdtree_helper(os, begin, end, 0);
}

template <class RandomAccessIterator, class Point>
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
  using coordinate_type = typename Point::coordinate_type;
  coordinate_type distance = std::numeric_limits<coordinate_type>::max();
  RandomAccessIterator location = end;
  detail::nnsearch_kdtree_helper(begin, end, point, 0, distance, location);
  return location;
}

template <class RandomAccessIterator, class Point>
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
  using coordinate_type = typename Point::coordinate_type;
  using heap_entry = std::pair<coordinate_type, RandomAccessIterator>;
  // Max-heap: largest distance at front, so we can efficiently replace
  // the worst candidate when a closer point is found.
  auto heap_compare = [](heap_entry const &lhs, heap_entry const &rhs) {
    return lhs.first < rhs.first;
  };
  std::vector<heap_entry> heap;
  heap.reserve(k);
  detail::nnsearch_kdtree_helper(begin, end, point, k, 0, heap, heap_compare);
  // Extract iterators directly from the heap vector — O(k) instead of
  // O(k log k) priority queue drain.
  std::vector<RandomAccessIterator> result;
  result.reserve(heap.size());
  for (auto const &entry : heap) {
    result.push_back(entry.second);
  }
  return result;
}

template <class RandomAccessIterator, class Point>
RandomAccessIterator search_kdtree(RandomAccessIterator begin, RandomAccessIterator end,
                                   Point const &point) {
  using iterator_tag = typename std::iterator_traits<RandomAccessIterator>::iterator_category;
  using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
  static_assert(std::is_convertible<iterator_tag, std::random_access_iterator_tag>::value,
                "kdtree::search_kdtree only accepts random access iterators or raw pointers.\n");
  static_assert(std::is_convertible<Point, value_type>::value,
                "kdtree::search_kdtree requires Point convertible to the iterator's value_type.\n");
  RandomAccessIterator it = nnsearch_kdtree(begin, end, point);
  if (it == end) {
    return end;
  }
  return point == *it ? it : end;
}

// Output-iterator overloads: write results directly to the caller's output
// iterator, avoiding internal allocation. Callers can reuse storage across
// repeated queries by clearing a vector and passing std::back_inserter.
template <class RandomAccessIterator, class Point, class OutputIt>
OutputIt rangequery_kdtree(RandomAccessIterator begin, RandomAccessIterator end, Point const &min,
                           Point const &max, OutputIt out) {
  detail::rangequery_kdtree_helper(begin, end, min, max, 0, out);
  return out;
}

template <class RandomAccessIterator, class Point, class OutputIt>
OutputIt radiusquery_kdtree(RandomAccessIterator begin, RandomAccessIterator end,
                            Point const &point, typename Point::coordinate_type radius,
                            OutputIt out) {
  if (radius > 0) {
    auto squared_radius = radius * radius;
    detail::radiusquery_kdtree_helper(begin, end, point, squared_radius, 0, out);
  }
  return out;
}

// Convenience overloads: return results in a new vector. For repeated queries,
// prefer the output-iterator overloads above to avoid per-call allocation.
template <class RandomAccessIterator, class Point>
std::vector<RandomAccessIterator> rangequery_kdtree(RandomAccessIterator begin,
                                                    RandomAccessIterator end, Point const &min,
                                                    Point const &max) {
  std::vector<RandomAccessIterator> locations;
  rangequery_kdtree(begin, end, min, max, std::back_inserter(locations));
  return locations;
}

template <class RandomAccessIterator, class Point>
std::vector<RandomAccessIterator> radiusquery_kdtree(RandomAccessIterator begin,
                                                     RandomAccessIterator end, Point const &point,
                                                     typename Point::coordinate_type radius) {
  std::vector<RandomAccessIterator> locations;
  radiusquery_kdtree(begin, end, point, radius, std::back_inserter(locations));
  return locations;
}

} // namespace kdtree

#endif
