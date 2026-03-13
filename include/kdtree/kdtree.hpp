#ifndef KDTREE_KDTREE_HPP
#define KDTREE_KDTREE_HPP

#include "point.hpp"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <limits>
#include <queue>
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

template <class RandomAccessIterator, class Point, class PriorityQueue>
void update_priority_queue(RandomAccessIterator it, Point const &p, PriorityQueue &pq,
                           std::size_t k) {
  auto dist = squared_euclidean_distance(*it, p);
  if (pq.size() < k) {
    pq.emplace(dist, it);
  } else {
    if (dist < pq.top().first) {
      pq.pop();
      pq.emplace(dist, it);
    }
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
  dimension_type dim = dimension(begin->dimensionality(), depth);
  std::size_t n = end - begin;
  if (n > 1) {
    RandomAccessIterator median = begin + (n / 2);
    auto comp = [dim](auto lhs, auto rhs) { return *(lhs.begin() + dim) < *(rhs.begin() + dim); };
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

template <class RandomAccessIterator, class Point, class PriorityQueue>
void nnsearch_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                            Point const &point, std::size_t k, depth_type depth,
                            PriorityQueue &pq) {
  dimension_type dim = dimension(Point::dimensionality(), depth);
  std::size_t n = end - begin;
  if (n > 0) {
    RandomAccessIterator median = begin + (n / 2);
    if (n > 1) {
      // See comment in the 1-NN overload above regarding conditional
      // median evaluation. The pq.size() < k guard ensures we always
      // explore both subtrees until k candidates have been collected.
      if (point[dim] <= (*median)[dim]) {
        nnsearch_kdtree_helper(begin, median, point, k, depth + 1, pq);
        auto gap = (*median)[dim] - point[dim];
        if (pq.size() < k || gap * gap <= pq.top().first) {
          update_priority_queue(median, point, pq, k);
          nnsearch_kdtree_helper(median + 1, end, point, k, depth + 1, pq);
        }
      } else {
        nnsearch_kdtree_helper(median + 1, end, point, k, depth + 1, pq);
        auto gap = point[dim] - (*median)[dim];
        if (pq.size() < k || gap * gap <= pq.top().first) {
          update_priority_queue(median, point, pq, k);
          nnsearch_kdtree_helper(begin, median, point, k, depth + 1, pq);
        }
      }
    } else if (n == 1) {
      update_priority_queue(median, point, pq, k);
    }
  }
}

template <class RandomAccessIterator, class Point>
void rangequery_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                              Point const &min, Point const &max, depth_type depth,
                              std::vector<RandomAccessIterator> &locations) {
  dimension_type dim = dimension(Point::dimensionality(), depth);
  std::size_t n = end - begin;
  if (n > 0) {
    RandomAccessIterator median = begin + (n / 2);
    bool left_oob = min[dim] > (*median)[dim];
    bool right_oob = max[dim] < (*median)[dim];
    if (!left_oob) {
      rangequery_kdtree_helper(begin, median, min, max, depth + 1, locations);
    }
    if (!right_oob) {
      rangequery_kdtree_helper(median + 1, end, min, max, depth + 1, locations);
    }
    if (!left_oob && !right_oob) {
      if (hypercube_contains(min, max, *median)) {
        locations.push_back(median);
      }
    }
  }
}

template <class RandomAccessIterator, class Point, class DistanceType>
void radiusquery_kdtree_helper(RandomAccessIterator begin, RandomAccessIterator end,
                               Point const &center, DistanceType squared_radius, depth_type depth,
                               std::vector<RandomAccessIterator> &locations) {
  std::size_t n = end - begin;
  if (n == 0) {
    return;
  }
  dimension_type dim = dimension(Point::dimensionality(), depth);
  RandomAccessIterator median = begin + (n / 2);

  if (squared_euclidean_distance(center, *median) <= squared_radius) {
    locations.push_back(median);
  }

  if (n == 1) {
    return;
  }

  auto gap = center[dim] - (*median)[dim];

  if (gap <= 0) {
    radiusquery_kdtree_helper(begin, median, center, squared_radius, depth + 1, locations);
    if (gap * gap <= squared_radius) {
      radiusquery_kdtree_helper(median + 1, end, center, squared_radius, depth + 1, locations);
    }
  } else {
    radiusquery_kdtree_helper(median + 1, end, center, squared_radius, depth + 1, locations);
    if (gap * gap <= squared_radius) {
      radiusquery_kdtree_helper(begin, median, center, squared_radius, depth + 1, locations);
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
  using pq_data_package = typename std::pair<coordinate_type, RandomAccessIterator>;
  auto pq_compare = [](pq_data_package const &lhs, pq_data_package const &rhs) {
    return lhs.first < rhs.first;
  };
  using vector = std::vector<pq_data_package>;
  using pq_type = std::priority_queue<pq_data_package, vector, decltype(pq_compare)>;
  pq_type pq(pq_compare);
  detail::nnsearch_kdtree_helper(begin, end, point, k, 0, pq);
  std::vector<RandomAccessIterator> result;
  result.reserve(pq.size());
  while (!pq.empty()) {
    result.push_back(pq.top().second);
    pq.pop();
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

template <class RandomAccessIterator, class Point>
std::vector<RandomAccessIterator> rangequery_kdtree(RandomAccessIterator begin,
                                                    RandomAccessIterator end, Point const &min,
                                                    Point const &max) {
  std::vector<RandomAccessIterator> locations;
  detail::rangequery_kdtree_helper(begin, end, min, max, 0, locations);
  return locations;
}

template <class RandomAccessIterator, class Point>
std::vector<RandomAccessIterator> radiusquery_kdtree(RandomAccessIterator begin,
                                                     RandomAccessIterator end, Point const &point,
                                                     double radius) {
  std::vector<RandomAccessIterator> locations;
  if (radius > 0) {
    auto squared_radius = radius * radius;
    detail::radiusquery_kdtree_helper(begin, end, point, squared_radius, 0, locations);
  }
  return locations;
}

} // namespace kdtree

#endif
