#ifndef KDTREE_POINT_HPP
#define KDTREE_POINT_HPP

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace kdtree {

/**
 * @brief A fixed-size point in d-dimensional space.
 *
 * Compositional facade over std::array providing contiguous storage with no
 * dynamic allocation. Satisfies the RandomAccessIterator concept required by
 * the k-d tree container adaptor functions.
 *
 * @tparam T Coordinate type (e.g., float, double, int).
 * @tparam d Number of dimensions.
 */
template <typename T, std::size_t d> class point {
private:
  using storage_type = std::array<T, d>;
  using size_type = typename storage_type::size_type;
  storage_type _coordinates;

public:
  using iterator = typename storage_type::iterator;
  using const_iterator = typename storage_type::const_iterator;
  using reverse_iterator = typename storage_type::reverse_iterator;
  using const_reverse_iterator = typename storage_type::const_reverse_iterator;
  using coordinate_type = T;

  point() = default;

  /**
   * @brief Construct a point from exactly d coordinate values.
   *
   * Enabled only when the number of arguments matches the dimensionality.
   */
  template <class... T2, typename std::enable_if<sizeof...(T2) == d, int>::type = 0>
  point(T2... args) : _coordinates{std::forward<T2>(args)...} {}

  /** @brief Return the number of dimensions (compile-time constant). */
  static constexpr typename storage_type::size_type dimensionality() noexcept { return d; }

  constexpr coordinate_type &operator[](size_type dimension) { return _coordinates[dimension]; }
  constexpr coordinate_type const &operator[](size_type dimension) const {
    return _coordinates[dimension];
  }

  bool operator==(point const &other) const {
    return std::equal(this->begin(), this->end(), other.begin(), other.end());
  }

  /** @brief Lexicographic ordering over coordinates. */
  bool operator<(point const &other) const {
    return std::lexicographical_compare(this->begin(), this->end(), other.begin(), other.end());
  }

  point operator+(point const &other) const {
    point result;
    std::transform(this->begin(), this->end(), other.begin(), result.begin(),
                   [](auto xi1, auto xi2) { return xi1 + xi2; });
    return result;
  }
  point operator-(point const &other) const {
    point result;
    std::transform(this->begin(), this->end(), other.begin(), result.begin(),
                   [](auto xi1, auto xi2) { return xi1 - xi2; });
    return result;
  }

  iterator begin() noexcept { return _coordinates.begin(); }
  const_iterator begin() const noexcept { return _coordinates.begin(); }
  const_iterator cbegin() const noexcept { return _coordinates.cbegin(); }
  iterator end() noexcept { return _coordinates.end(); }
  const_iterator end() const noexcept { return _coordinates.end(); }
  const_iterator cend() const noexcept { return _coordinates.cend(); }
  reverse_iterator rbegin() noexcept { return _coordinates.rbegin(); }
  const_reverse_iterator rbegin() const noexcept { return _coordinates.rbegin(); }
  const_reverse_iterator crbegin() const noexcept { return _coordinates.crbegin(); }
  reverse_iterator rend() noexcept { return _coordinates.rend(); }
  const_reverse_iterator rend() const noexcept { return _coordinates.rend(); }
  const_reverse_iterator crend() const noexcept { return _coordinates.crend(); }
};

/**
 * @brief Compute the squared Euclidean distance between two same-type points.
 *
 * Preferred overload when both points share a coordinate type. The index-based
 * loop with compile-time-constant bound enables auto-vectorization.
 *
 * @return Sum of squared coordinate differences, as type T.
 */
template <class T, std::size_t d>
T squared_euclidean_distance(point<T, d> const &p1, point<T, d> const &p2) {
  T dist = 0;
  for (std::size_t i = 0; i < d; ++i) {
    T diff = p1[i] - p2[i];
    dist += diff * diff;
  }
  return dist;
}

/**
 * @brief Compute the squared Euclidean distance between two mixed-type points.
 *
 * Supports distance between points with different coordinate types (e.g.,
 * point<int,2> and point<double,2>). The return type is the common type
 * produced by subtracting the two coordinate types.
 */
template <class T, class U, std::size_t d>
auto squared_euclidean_distance(point<T, d> const &p1, point<U, d> const &p2)
    -> decltype(T{} - U{}) {
  using result_type = decltype(T{} - U{});
  result_type dist = 0;
  for (std::size_t i = 0; i < d; ++i) {
    result_type diff = p1[i] - p2[i];
    dist += diff * diff;
  }
  return dist;
}

namespace detail {

/** @brief Absolute difference safe for both signed and unsigned types. */
template <class T> typename std::enable_if<std::is_signed<T>::value, T>::type abs_diff(T a, T b) {
  T diff = a - b;
  return diff < 0 ? -diff : diff;
}

template <class T> typename std::enable_if<std::is_unsigned<T>::value, T>::type abs_diff(T a, T b) {
  return a >= b ? a - b : b - a;
}

} // namespace detail

/**
 * @brief Compute the Chebyshev (max-norm / L-infinity) distance between two same-type points.
 *
 * The Chebyshev distance is the maximum absolute coordinate difference:
 * d(p1, p2) = max_i |p1[i] - p2[i]|. Used in KSG mutual information estimation.
 *
 * @return Maximum absolute coordinate difference, as type T.
 */
template <class T, std::size_t d>
T chebyshev_distance(point<T, d> const &p1, point<T, d> const &p2) {
  T dist = 0;
  for (std::size_t i = 0; i < d; ++i) {
    T ad = detail::abs_diff(p1[i], p2[i]);
    if (ad > dist) {
      dist = ad;
    }
  }
  return dist;
}

/**
 * @brief Compute the Chebyshev distance between two mixed-type points.
 */
template <class T, class U, std::size_t d>
auto chebyshev_distance(point<T, d> const &p1, point<U, d> const &p2) -> decltype(T{} - U{}) {
  using result_type = decltype(T{} - U{});
  result_type dist = 0;
  for (std::size_t i = 0; i < d; ++i) {
    result_type ad =
        detail::abs_diff(static_cast<result_type>(p1[i]), static_cast<result_type>(p2[i]));
    if (ad > dist) {
      dist = ad;
    }
  }
  return dist;
}

// --- Distance metric policies for kd-tree search ---
// These allow the search algorithms to be parameterized on the distance metric
// at compile time, with zero runtime overhead (all methods are inlined).

/**
 * @brief Squared Euclidean distance metric policy (default).
 *
 * Computes squared Euclidean distance for full points and uses squared
 * single-dimension gap for kd-tree pruning. This is the standard metric
 * for nearest neighbor search.
 */
struct squared_euclidean_metric {
  /** @brief Compute distance between two points. */
  template <class P> static auto distance(P const &a, P const &b) -> decltype(a[0] - b[0]) {
    return squared_euclidean_distance(a, b);
  }

  /** @brief Compute the pruning distance from a single-dimension gap.
   *  For squared Euclidean, this is gap^2 (lower bound on full distance). */
  template <class T> static T prune_distance(T gap) { return gap * gap; }
};

/**
 * @brief Chebyshev (max-norm / L-infinity) distance metric policy.
 *
 * Computes Chebyshev distance for full points and uses absolute
 * single-dimension gap for pruning. Required by the KSG mutual
 * information estimator (Kraskov et al., 2004).
 */
struct chebyshev_metric {
  /** @brief Compute distance between two points. */
  template <class P> static auto distance(P const &a, P const &b) -> decltype(a[0] - b[0]) {
    return chebyshev_distance(a, b);
  }

  /** @brief Compute pruning distance from a single-dimension gap.
   *  For Chebyshev, this is |gap| (exact lower bound on full distance). */
  template <class T> static T prune_distance(T gap) { return detail::abs_diff(gap, T{0}); }
};

/**
 * @brief Write a point in parenthesized format: (x1,x2,...,xd).
 */
template <typename T, std::size_t d>
std::ostream &operator<<(std::ostream &os, kdtree::point<T, d> const &p) {
  os << '(';
  if (d > 0) {
    for (auto it = p.cbegin(); it != p.cend() - 1; ++it) {
      os << *it << ',';
    }
    os << *(p.cend() - 1);
  }
  os << ')';
  return os;
}

/**
 * @brief Read a point from parenthesized format: (x1,x2,...,xd).
 *
 * @throws std::range_error If the input does not match the expected format.
 */
template <typename T, std::size_t d>
std::istream &operator>>(std::istream &is, kdtree::point<T, d> &p) {
  auto generate_error_message = [](char c_expected, char c_given) {
    std::stringstream error_ss("invalid format for kdtree:point: expected '",
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
  if (c != '(') {
    throw std::range_error(generate_error_message('(', c));
  }
  if (d > 0) {
    for (auto it = p.begin(); it != p.end() - 1; ++it) {
      try {
        is >> *it;
      } catch (...) {
        throw std::range_error("invalid format for kdtree:point: expected coordinate");
      }
      is >> c;
      if (c != ',') {
        throw std::range_error(generate_error_message(',', c));
      }
    }
    try {
      is >> *(p.end() - 1);
    } catch (...) {
      throw std::range_error("invalid format for kdtree:point: expected coordinate");
    }
  }
  is >> c;
  if (c != ')') {
    throw std::range_error(generate_error_message(')', c));
  }
  return is;
}
} // namespace kdtree

namespace std {
/** @brief Hash specialization for kdtree::point using boost-style combining. */
template <typename T, std::size_t d> struct hash<kdtree::point<T, d>> {
  using argument_type = kdtree::point<T, d>;
  using result_type = std::size_t;
  result_type operator()(argument_type const &key) const noexcept {
    result_type hash = 0;
    for (auto const xi : key) {
      hash ^= std::hash<T>{}(xi) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
  }
};
} // namespace std

#endif
