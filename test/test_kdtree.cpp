#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <kdtree/kdtree.hpp>
#include <kdtree/point.hpp>

using Catch::Matchers::WithinAbs;

// --- Helper: brute-force nearest neighbor for verification ---

template <typename DistanceFn, typename PointType>
typename std::vector<PointType>::const_iterator
brute_force_nn(DistanceFn dist_fn, const std::vector<PointType> &data, const PointType &query) {
  auto best = data.cbegin();
  auto best_dist = dist_fn(query, *best);
  for (auto it = data.cbegin() + 1; it != data.cend(); ++it) {
    auto dist = dist_fn(query, *it);
    if (dist < best_dist) {
      best_dist = dist;
      best = it;
    }
  }
  return best;
}

// --- Helper: brute-force kNN for verification ---

template <typename DistanceFn, typename PointType>
std::vector<PointType> brute_force_knn(DistanceFn dist_fn, const std::vector<PointType> &data,
                                       const PointType &query, std::size_t k) {
  std::vector<std::pair<decltype(dist_fn(query, data[0])), std::size_t>> dists;
  dists.reserve(data.size());
  for (std::size_t i = 0; i < data.size(); ++i) {
    dists.emplace_back(dist_fn(query, data[i]), i);
  }
  std::sort(dists.begin(), dists.end());
  std::vector<PointType> result;
  for (std::size_t i = 0; i < std::min(k, dists.size()); ++i) {
    result.push_back(data[dists[i].second]);
  }
  std::sort(result.begin(), result.end());
  return result;
}

// Distance function objects for brute-force helpers
struct euclidean_dist {
  template <typename P> auto operator()(P const &a, P const &b) const {
    return kdtree::squared_euclidean_distance(a, b);
  }
};

struct chebyshev_dist {
  template <typename P> auto operator()(P const &a, P const &b) const {
    return kdtree::chebyshev_distance(a, b);
  }
};

// ============================================================
// make_kdtree
// ============================================================

TEST_CASE("make_kdtree with empty range", "[kdtree][make]") {
  std::vector<kdtree::point<int, 2>> data;
  // Should not crash
  kdtree::make_kdtree(data.begin(), data.end());
  REQUIRE(data.empty());
}

TEST_CASE("make_kdtree with single element", "[kdtree][make]") {
  std::vector<kdtree::point<int, 2>> data = {{5, 10}};
  kdtree::make_kdtree(data.begin(), data.end());
  REQUIRE(data[0] == kdtree::point<int, 2>(5, 10));
}

TEST_CASE("make_kdtree preserves all elements", "[kdtree][make]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3}, {2, 7}, {-3, 6}, {-2, -1}, {-7, 4}};
  auto sorted_before = data;
  std::ranges::sort(sorted_before);

  kdtree::make_kdtree(data.begin(), data.end());

  auto sorted_after = data;
  std::ranges::sort(sorted_after);
  REQUIRE(sorted_before == sorted_after);
}

TEST_CASE("make_kdtree median element satisfies partition property", "[kdtree][make]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3}, {5, 1}, {3, 7}, {0, 4}, {8, 2}};
  // Use LeafThreshold=1 to force full recursive partitioning
  kdtree::make_kdtree<1>(data.begin(), data.end());

  // Root median is at index n/2 = 2, split on dim 0
  auto median = data[2];
  // All elements before median should have dim-0 <= median
  for (std::size_t i = 0; i < 2; ++i) {
    REQUIRE(data[i][0] <= median[0]);
  }
  // All elements after median should have dim-0 >= median
  for (std::size_t i = 3; i < 5; ++i) {
    REQUIRE(data[i][0] >= median[0]);
  }
}

// ============================================================
// search_kdtree (exact search)
// ============================================================

TEST_CASE("search_kdtree finds existing point", "[kdtree][search]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3}, {-5, 2}, {2, 7}, {-3, 6}, {4, 1}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> target(-5, 2);
  auto it = kdtree::search_kdtree(data.cbegin(), data.cend(), target);
  REQUIRE(it != data.cend());
  REQUIRE(*it == target);
}

TEST_CASE("search_kdtree returns end for missing point", "[kdtree][search]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3}, {-5, 2}, {2, 7}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> target(99, 99);
  auto it = kdtree::search_kdtree(data.cbegin(), data.cend(), target);
  REQUIRE(it == data.cend());
}

TEST_CASE("search_kdtree finds all inserted points", "[kdtree][search]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3}, {2, 7}, {-3, 6}, {-2, -1}, {-7, 4}};
  auto original = data;
  kdtree::make_kdtree(data.begin(), data.end());

  for (const auto &p : original) {
    auto it = kdtree::search_kdtree(data.cbegin(), data.cend(), p);
    REQUIRE(it != data.cend());
    REQUIRE(*it == p);
  }
}

// ============================================================
// nnsearch_kdtree (1-NN)
// ============================================================

TEST_CASE("nnsearch_kdtree single element", "[kdtree][nn]") {
  std::vector<kdtree::point<int, 2>> data = {{5, 10}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(0, 0);
  auto it = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query);
  REQUIRE(it != data.cend());
  REQUIRE(*it == kdtree::point<int, 2>(5, 10));
}

TEST_CASE("nnsearch_kdtree exact match", "[kdtree][nn]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3}, {-5, 2}, {2, 7}, {0, 0}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(0, 0);
  auto it = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query);
  REQUIRE(*it == query);
}

TEST_CASE("nnsearch_kdtree basic 2D int", "[kdtree][nn]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3},  {2, 7},   {-3, 6}, {-2, -1}, {-7, 4},
                                             {2, 3},  {-5, 2},  {-1, 9}, {6, -3},  {-4, 0},
                                             {0, -1}, {-2, -1}, {3, 3}};
  auto original = data;
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(-1, -1);
  auto it = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query);
  auto brute_it = brute_force_nn(euclidean_dist{}, data, query);

  REQUIRE(kdtree::squared_euclidean_distance(query, *it) ==
          kdtree::squared_euclidean_distance(query, *brute_it));
}

TEST_CASE("nnsearch_kdtree matches brute force for multiple queries", "[kdtree][nn]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3},  {2, 7},  {-3, 6}, {-2, -1}, {-7, 4}, {2, 3},
                                             {-5, 2}, {-1, 9}, {6, -3}, {-4, 0},  {0, -1}, {3, 3}};
  kdtree::make_kdtree(data.begin(), data.end());

  std::vector<kdtree::point<int, 2>> queries = {{0, 0}, {5, 5}, {-10, -10}, {3, 7}, {-3, -3}};

  for (const auto &q : queries) {
    auto it = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), q);
    auto brute_it = brute_force_nn(euclidean_dist{}, data, q);
    REQUIRE(kdtree::squared_euclidean_distance(q, *it) ==
            kdtree::squared_euclidean_distance(q, *brute_it));
  }
}

TEST_CASE("nnsearch_kdtree 3D", "[kdtree][nn]") {
  std::vector<kdtree::point<int, 3>> data = {
      {1, 3, 1}, {2, 7, 1}, {-3, 6, 1}, {-2, -1, 1}, {0, 0, 0}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 3> query(0, 0, 0);
  auto it = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query);
  REQUIRE(*it == kdtree::point<int, 3>(0, 0, 0));
}

TEST_CASE("nnsearch_kdtree with float points - pruning correctness", "[kdtree][nn][pruning]") {
  // This test case is specifically designed to expose the pruning bug where
  // squared distance is compared as a linear distance against the splitting
  // plane gap. The bug causes incorrect pruning when mindist is in [gap^2, gap).
  //
  // Setup: 3 points where the true NN is the median, but after searching the
  // left subtree first and finding a point at small squared distance, the
  // buggy pruning skips checking the median entirely.
  //
  // Points: A=(0, 0.3), B=(0.4, 0), C=(1, 10)
  // Query: Q=(0.1, 0)
  // Tree: median=B at dim 0. Left: A. Right: C.
  // Search goes left first, finds A at squared dist 0.1.
  // gap = 0.4 - 0.1 = 0.3, gap^2 = 0.09.
  // mindist=0.1 is in [0.09, 0.3) — buggy version prunes, correct explores.
  // True NN is B at squared dist 0.0025.
  std::vector<kdtree::point<double, 2>> data = {{0.0, 0.3}, {0.4, 0.0}, {1.0, 10.0}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<double, 2> query(0.1, 0.0);
  auto it = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query);
  auto brute_it = brute_force_nn(euclidean_dist{}, data, query);

  auto kdtree_dist = kdtree::squared_euclidean_distance(query, *it);
  auto brute_dist = kdtree::squared_euclidean_distance(query, *brute_it);

  INFO("kdtree returned: (" << (*it)[0] << ", " << (*it)[1] << ") at squared dist " << kdtree_dist);
  INFO("brute force NN:  (" << (*brute_it)[0] << ", " << (*brute_it)[1] << ") at squared dist "
                            << brute_dist);
  REQUIRE_THAT(kdtree_dist, WithinAbs(brute_dist, 1e-10));
}

TEST_CASE("nnsearch_kdtree with float points - larger dataset", "[kdtree][nn][pruning]") {
  // Larger dataset with float coordinates to stress-test pruning across
  // multiple tree levels.
  std::vector<kdtree::point<double, 2>> data = {
      {0.1, 0.2}, {0.5, 0.8}, {0.3, 0.1},   {0.9, 0.4},   {0.7, 0.6},   {0.2, 0.9},  {0.4, 0.3},
      {0.6, 0.7}, {0.8, 0.5}, {0.15, 0.85}, {0.35, 0.55}, {0.65, 0.15}, {0.95, 0.95}};
  kdtree::make_kdtree(data.begin(), data.end());

  std::vector<kdtree::point<double, 2>> queries = {
      {0.25, 0.25}, {0.75, 0.75}, {0.0, 0.0}, {1.0, 1.0}, {0.45, 0.45}, {0.11, 0.81}, {0.33, 0.07}};

  for (const auto &q : queries) {
    auto it = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), q);
    auto brute_it = brute_force_nn(euclidean_dist{}, data, q);

    auto kdtree_dist = kdtree::squared_euclidean_distance(q, *it);
    auto brute_dist = kdtree::squared_euclidean_distance(q, *brute_it);

    INFO("query: (" << q[0] << ", " << q[1] << ")");
    INFO("kdtree: (" << (*it)[0] << ", " << (*it)[1] << ") dist=" << kdtree_dist);
    INFO("brute:  (" << (*brute_it)[0] << ", " << (*brute_it)[1] << ") dist=" << brute_dist);
    REQUIRE_THAT(kdtree_dist, WithinAbs(brute_dist, 1e-10));
  }
}

// ============================================================
// nnsearch_kdtree (kNN)
// ============================================================

TEST_CASE("nnsearch_kdtree kNN returns correct count", "[kdtree][knn]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3}, {2, 7}, {-3, 6}, {-2, -1}, {-7, 4}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(0, 0);
  auto results = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query, 3);
  REQUIRE(results.size() == 3);
}

TEST_CASE("nnsearch_kdtree kNN returns valid iterators", "[kdtree][knn]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3}, {2, 7}, {-3, 6}, {-2, -1}, {-7, 4}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(0, 0);
  auto results = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query, 3);

  for (const auto &it : results) {
    // Every returned iterator must be dereferenceable (not end)
    REQUIRE(it >= data.cbegin());
    REQUIRE(it < data.cend());
  }
}

TEST_CASE("nnsearch_kdtree kNN k=0 returns empty", "[kdtree][knn]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 1}, {2, 2}};
  kdtree::make_kdtree(data.begin(), data.end());
  kdtree::point<int, 2> query(0, 0);
  auto results = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query, 0);
  REQUIRE(results.empty());
}

TEST_CASE("nnsearch_kdtree kNN k=1 matches 1-NN", "[kdtree][knn]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3}, {2, 7}, {-3, 6}, {-2, -1}, {-7, 4}, {0, 0}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(1, 1);
  auto nn_it = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query);
  auto knn_results = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query, 1);

  REQUIRE(knn_results.size() == 1);
  auto nn_dist = kdtree::squared_euclidean_distance(query, *nn_it);
  auto knn_dist = kdtree::squared_euclidean_distance(query, *knn_results[0]);
  REQUIRE(nn_dist == knn_dist);
}

TEST_CASE("nnsearch_kdtree kNN matches brute force", "[kdtree][knn]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3},  {2, 7},  {-3, 6}, {-2, -1}, {-7, 4}, {2, 3},
                                             {-5, 2}, {-1, 9}, {6, -3}, {-4, 0},  {0, -1}, {3, 3}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(-1, -1);
  std::size_t k = 4;
  auto results = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query, k);
  auto expected = brute_force_knn(euclidean_dist{}, data, query, k);

  // Collect kdtree results as sorted points
  std::vector<kdtree::point<int, 2>> result_points;
  result_points.reserve(results.size());
  for (const auto &it : results) {
    result_points.push_back(*it);
  }
  std::ranges::sort(result_points);

  REQUIRE(result_points == expected);
}

TEST_CASE("nnsearch_kdtree kNN with k >= n returns all points", "[kdtree][knn]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 1}, {2, 2}, {3, 3}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(0, 0);
  auto results = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query, 5);

  // Should return at most n results, all valid
  REQUIRE(results.size() <= data.size());
  for (const auto &it : results) {
    REQUIRE(it >= data.cbegin());
    REQUIRE(it < data.cend());
  }
}

// ============================================================
// rangequery_kdtree
// ============================================================

TEST_CASE("rangequery_kdtree basic", "[kdtree][range]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3},  {2, 7},   {-3, 6}, {-2, -1}, {-7, 4},
                                             {2, 3},  {-5, 2},  {-1, 9}, {6, -3},  {-4, 0},
                                             {0, -1}, {-2, -1}, {3, 3}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> lower(-2, -3);
  kdtree::point<int, 2> upper(3, 3);
  auto results = kdtree::rangequery_kdtree(data.cbegin(), data.cend(), lower, upper);

  // Verify all results are within range
  for (const auto &it : results) {
    REQUIRE((*it)[0] >= -2);
    REQUIRE((*it)[0] <= 3);
    REQUIRE((*it)[1] >= -3);
    REQUIRE((*it)[1] <= 3);
  }

  // Verify against brute force
  std::size_t brute_count = 0;
  for (const auto &p : data) {
    if (p[0] >= -2 && p[0] <= 3 && p[1] >= -3 && p[1] <= 3) {
      ++brute_count;
    }
  }
  REQUIRE(results.size() == brute_count);
}

TEST_CASE("rangequery_kdtree empty result", "[kdtree][range]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 1}, {2, 2}, {3, 3}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> lower(10, 10);
  kdtree::point<int, 2> upper(20, 20);
  auto results = kdtree::rangequery_kdtree(data.cbegin(), data.cend(), lower, upper);
  REQUIRE(results.empty());
}

TEST_CASE("rangequery_kdtree all points in range", "[kdtree][range]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 1}, {2, 2}, {3, 3}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> lower(0, 0);
  kdtree::point<int, 2> upper(10, 10);
  auto results = kdtree::rangequery_kdtree(data.cbegin(), data.cend(), lower, upper);
  REQUIRE(results.size() == 3);
}

TEST_CASE("rangequery_kdtree 3D", "[kdtree][range]") {
  std::vector<kdtree::point<int, 3>> data = {{1, 1, 1}, {2, 2, 2}, {3, 3, 3}, {0, 0, 0}, {5, 5, 5}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 3> lower(0, 0, 0);
  kdtree::point<int, 3> upper(2, 2, 2);
  auto results = kdtree::rangequery_kdtree(data.cbegin(), data.cend(), lower, upper);

  for (const auto &it : results) {
    for (std::size_t d = 0; d < 3; ++d) {
      REQUIRE((*it)[d] >= 0);
      REQUIRE((*it)[d] <= 2);
    }
  }
}

TEST_CASE("rangequery_kdtree output iterator overload", "[kdtree][range]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 1}, {2, 2}, {3, 3}, {10, 10}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> lower(0, 0);
  kdtree::point<int, 2> upper(5, 5);

  // Use output iterator with pre-allocated vector
  std::vector<decltype(data.cbegin())> results;
  kdtree::rangequery_kdtree(data.cbegin(), data.cend(), lower, upper, std::back_inserter(results));
  REQUIRE(results.size() == 3);

  // Verify reuse: clear and query again without reallocation
  results.clear();
  kdtree::rangequery_kdtree(data.cbegin(), data.cend(), lower, upper, std::back_inserter(results));
  REQUIRE(results.size() == 3);
}

// ============================================================
// radiusquery_kdtree
// ============================================================

TEST_CASE("radiusquery_kdtree basic", "[kdtree][radius]") {
  std::vector<kdtree::point<int, 2>> data = {{0, 0}, {1, 0}, {0, 1}, {3, 3}, {-5, -5}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> center(0, 0);
  int radius = 1;
  auto results = kdtree::radiusquery_kdtree(data.cbegin(), data.cend(), center, radius);

  // Points within radius 1 of origin: (0,0) at d=0, (1,0) at d=1, (0,1) at d=1
  REQUIRE(results.size() == 3);
  for (const auto &it : results) {
    auto dist = std::sqrt(static_cast<double>(kdtree::squared_euclidean_distance(center, *it)));
    REQUIRE(dist <= static_cast<double>(radius));
  }
}

TEST_CASE("radiusquery_kdtree zero radius returns empty", "[kdtree][radius]") {
  std::vector<kdtree::point<int, 2>> data = {{0, 0}, {1, 1}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> center(0, 0);
  auto results = kdtree::radiusquery_kdtree(data.cbegin(), data.cend(), center, 0.0);
  REQUIRE(results.empty());
}

TEST_CASE("radiusquery_kdtree negative radius returns empty", "[kdtree][radius]") {
  std::vector<kdtree::point<int, 2>> data = {{0, 0}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> center(0, 0);
  auto results = kdtree::radiusquery_kdtree(data.cbegin(), data.cend(), center, -1.0);
  REQUIRE(results.empty());
}

TEST_CASE("radiusquery_kdtree matches brute force", "[kdtree][radius]") {
  std::vector<kdtree::point<double, 2>> data = {{0.1, 0.2}, {0.5, 0.8}, {0.3, 0.1}, {0.9, 0.4},
                                                {0.7, 0.6}, {0.2, 0.9}, {0.4, 0.3}, {0.6, 0.7}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<double, 2> center(0.5, 0.5);
  double radius = 0.35;
  auto results = kdtree::radiusquery_kdtree(data.cbegin(), data.cend(), center, radius);

  // Brute-force count
  double sq_radius = radius * radius;
  std::size_t brute_count = 0;
  for (const auto &p : data) {
    if (kdtree::squared_euclidean_distance(center, p) <= sq_radius) {
      ++brute_count;
    }
  }
  REQUIRE(results.size() == brute_count);
}

TEST_CASE("radiusquery_kdtree output iterator overload", "[kdtree][radius]") {
  std::vector<kdtree::point<int, 2>> data = {{0, 0}, {1, 0}, {0, 1}, {3, 3}, {-5, -5}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> center(0, 0);
  int radius = 1;

  std::vector<decltype(data.cbegin())> results;
  kdtree::radiusquery_kdtree(data.cbegin(), data.cend(), center, radius,
                             std::back_inserter(results));
  REQUIRE(results.size() == 3);

  // Verify reuse
  results.clear();
  kdtree::radiusquery_kdtree(data.cbegin(), data.cend(), center, radius,
                             std::back_inserter(results));
  REQUIRE(results.size() == 3);
}

// ============================================================
// print_kdtree
// ============================================================

TEST_CASE("print_kdtree produces output", "[kdtree][print]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3}, {2, 7}, {-3, 6}};
  kdtree::make_kdtree(data.begin(), data.end());

  std::ostringstream oss;
  kdtree::print_kdtree(oss, data.cbegin(), data.cend());
  REQUIRE_FALSE(oss.str().empty());
}

TEST_CASE("print_kdtree empty range produces no output", "[kdtree][print]") {
  std::vector<kdtree::point<int, 2>> data;
  std::ostringstream oss;
  kdtree::print_kdtree(oss, data.cbegin(), data.cend());
  REQUIRE(oss.str().empty());
}

// ============================================================
// Edge cases
// ============================================================

TEST_CASE("kdtree operations with duplicate points", "[kdtree][edge]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 1}, {1, 1}, {1, 1}, {2, 2}, {3, 3}};
  kdtree::make_kdtree(data.begin(), data.end());

  SECTION("search finds duplicate") {
    auto it = kdtree::search_kdtree(data.cbegin(), data.cend(), kdtree::point<int, 2>(1, 1));
    REQUIRE(it != data.cend());
    REQUIRE(*it == kdtree::point<int, 2>(1, 1));
  }

  SECTION("nn of duplicate") {
    auto it = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), kdtree::point<int, 2>(1, 1));
    REQUIRE(*it == kdtree::point<int, 2>(1, 1));
  }
}

TEST_CASE("kdtree with two elements", "[kdtree][edge]") {
  std::vector<kdtree::point<int, 2>> data = {{0, 0}, {10, 10}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(1, 1);
  auto it = kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), query);
  REQUIRE(*it == kdtree::point<int, 2>(0, 0));
}

// ============================================================
// nnsearch_kdtree with chebyshev_metric (1-NN)
// ============================================================

TEST_CASE("nnsearch_kdtree chebyshev 1-NN matches brute force", "[kdtree][nn][chebyshev]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3},  {2, 7},  {-3, 6}, {-2, -1}, {-7, 4}, {2, 3},
                                             {-5, 2}, {-1, 9}, {6, -3}, {-4, 0},  {0, -1}, {3, 3}};
  kdtree::make_kdtree(data.begin(), data.end());

  std::vector<kdtree::point<int, 2>> queries = {{0, 0}, {5, 5}, {-10, -10}, {3, 7}, {-3, -3}};

  for (const auto &q : queries) {
    auto it = kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), q);
    auto brute_it = brute_force_nn(chebyshev_dist{}, data, q);
    REQUIRE(kdtree::chebyshev_distance(q, *it) == kdtree::chebyshev_distance(q, *brute_it));
  }
}

TEST_CASE("nnsearch_kdtree chebyshev 1-NN with double points", "[kdtree][nn][chebyshev]") {
  std::vector<kdtree::point<double, 2>> data = {
      {0.1, 0.2}, {0.5, 0.8}, {0.3, 0.1},   {0.9, 0.4},   {0.7, 0.6},   {0.2, 0.9},  {0.4, 0.3},
      {0.6, 0.7}, {0.8, 0.5}, {0.15, 0.85}, {0.35, 0.55}, {0.65, 0.15}, {0.95, 0.95}};
  kdtree::make_kdtree(data.begin(), data.end());

  std::vector<kdtree::point<double, 2>> queries = {
      {0.25, 0.25}, {0.75, 0.75}, {0.0, 0.0}, {1.0, 1.0}, {0.45, 0.45}, {0.11, 0.81}, {0.33, 0.07}};

  for (const auto &q : queries) {
    auto it = kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), q);
    auto brute_it = brute_force_nn(chebyshev_dist{}, data, q);

    auto kdtree_dist = kdtree::chebyshev_distance(q, *it);
    auto brute_dist = kdtree::chebyshev_distance(q, *brute_it);

    INFO("query: (" << q[0] << ", " << q[1] << ")");
    INFO("kdtree: (" << (*it)[0] << ", " << (*it)[1] << ") dist=" << kdtree_dist);
    INFO("brute:  (" << (*brute_it)[0] << ", " << (*brute_it)[1] << ") dist=" << brute_dist);
    REQUIRE_THAT(kdtree_dist, WithinAbs(brute_dist, 1e-10));
  }
}

TEST_CASE("nnsearch_kdtree chebyshev pruning stress test", "[kdtree][nn][chebyshev][pruning]") {
  // Point near splitting plane in one dimension but far in another.
  // Forces the Chebyshev pruning path to be exercised: the gap in the
  // splitting dimension is small, but the true NN may be across the plane.
  std::vector<kdtree::point<double, 2>> data = {{0.0, 10.0}, {0.5, 0.0}, {1.0, 0.1}};
  kdtree::make_kdtree(data.begin(), data.end());

  // Query near the splitting plane — Chebyshev NN differs from Euclidean NN
  kdtree::point<double, 2> query(0.4, 0.0);
  auto it = kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), query);
  auto brute_it = brute_force_nn(chebyshev_dist{}, data, query);

  auto kdtree_dist = kdtree::chebyshev_distance(query, *it);
  auto brute_dist = kdtree::chebyshev_distance(query, *brute_it);

  INFO("kdtree: (" << (*it)[0] << ", " << (*it)[1] << ") dist=" << kdtree_dist);
  INFO("brute:  (" << (*brute_it)[0] << ", " << (*brute_it)[1] << ") dist=" << brute_dist);
  REQUIRE_THAT(kdtree_dist, WithinAbs(brute_dist, 1e-10));
}

TEST_CASE("nnsearch_kdtree chebyshev 3D", "[kdtree][nn][chebyshev]") {
  std::vector<kdtree::point<int, 3>> data = {{1, 3, 1},   {2, 7, 1}, {-3, 6, 1},
                                             {-2, -1, 1}, {0, 0, 0}, {4, 2, -3}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 3> query(0, 0, 0);
  auto it = kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), query);
  auto brute_it = brute_force_nn(chebyshev_dist{}, data, query);
  REQUIRE(kdtree::chebyshev_distance(query, *it) == kdtree::chebyshev_distance(query, *brute_it));
}

// ============================================================
// nnsearch_kdtree with chebyshev_metric (kNN)
// ============================================================

TEST_CASE("nnsearch_kdtree chebyshev kNN matches brute force", "[kdtree][knn][chebyshev]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3},  {2, 7},  {-3, 6}, {-2, -1}, {-7, 4}, {2, 3},
                                             {-5, 2}, {-1, 9}, {6, -3}, {-4, 0},  {0, -1}, {3, 3}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(-1, -1);
  std::size_t k = 4;
  auto results =
      kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), query, k);
  REQUIRE(results.size() == k);

  // Compare sorted distance multisets to handle ties correctly
  std::vector<int> kdtree_dists;
  kdtree_dists.reserve(results.size());
  for (const auto &it : results) {
    kdtree_dists.push_back(kdtree::chebyshev_distance(query, *it));
  }
  std::ranges::sort(kdtree_dists);

  // Brute force: compute all distances, sort, take first k
  std::vector<int> all_dists;
  all_dists.reserve(data.size());
  for (const auto &p : data) {
    all_dists.push_back(kdtree::chebyshev_distance(query, p));
  }
  std::ranges::sort(all_dists);
  std::vector<int> expected_dists(all_dists.begin(), all_dists.begin() + static_cast<long>(k));

  REQUIRE(kdtree_dists == expected_dists);
}

TEST_CASE("nnsearch_kdtree chebyshev kNN with double points", "[kdtree][knn][chebyshev]") {
  std::vector<kdtree::point<double, 2>> data = {
      {0.1, 0.2}, {0.5, 0.8}, {0.3, 0.1},   {0.9, 0.4},   {0.7, 0.6},   {0.2, 0.9},  {0.4, 0.3},
      {0.6, 0.7}, {0.8, 0.5}, {0.15, 0.85}, {0.35, 0.55}, {0.65, 0.15}, {0.95, 0.95}};
  kdtree::make_kdtree(data.begin(), data.end());

  std::vector<kdtree::point<double, 2>> queries = {{0.25, 0.25}, {0.75, 0.75}, {0.5, 0.5}};

  for (const auto &q : queries) {
    std::size_t k = 3;
    auto results =
        kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), q, k);
    auto expected = brute_force_knn(chebyshev_dist{}, data, q, k);

    std::vector<kdtree::point<double, 2>> result_points;
    result_points.reserve(results.size());
    for (const auto &it : results) {
      result_points.push_back(*it);
    }
    std::ranges::sort(result_points);

    REQUIRE(result_points.size() == expected.size());
    for (std::size_t i = 0; i < result_points.size(); ++i) {
      for (std::size_t dim = 0; dim < 2; ++dim) {
        REQUIRE_THAT(result_points[i][dim], WithinAbs(expected[i][dim], 1e-10));
      }
    }
  }
}

TEST_CASE("nnsearch_kdtree chebyshev kNN k=1 matches 1-NN", "[kdtree][knn][chebyshev]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 3}, {2, 7}, {-3, 6}, {-2, -1}, {-7, 4}, {0, 0}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(1, 1);
  auto nn_it = kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), query);
  auto knn_results =
      kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), query, 1);

  REQUIRE(knn_results.size() == 1);
  REQUIRE(kdtree::chebyshev_distance(query, *nn_it) ==
          kdtree::chebyshev_distance(query, *knn_results[0]));
}

// ============================================================
// Chebyshev edge cases
// ============================================================

TEST_CASE("nnsearch_kdtree chebyshev kNN k >= n returns all points", "[kdtree][knn][chebyshev]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 1}, {2, 2}, {3, 3}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(0, 0);
  auto results =
      kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), query, 10);
  REQUIRE(results.size() <= data.size());
  REQUIRE(results.size() == 3);
}

TEST_CASE("nnsearch_kdtree chebyshev 1-NN empty range", "[kdtree][nn][chebyshev][edge]") {
  std::vector<kdtree::point<int, 2>> data;
  kdtree::point<int, 2> query(0, 0);
  auto it = kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), query);
  REQUIRE(it == data.cend());
}

TEST_CASE("nnsearch_kdtree chebyshev kNN empty range", "[kdtree][knn][chebyshev][edge]") {
  std::vector<kdtree::point<int, 2>> data;
  kdtree::point<int, 2> query(0, 0);
  auto results =
      kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), query, 3);
  REQUIRE(results.empty());
}

TEST_CASE("nnsearch_kdtree chebyshev with duplicate points", "[kdtree][nn][chebyshev][edge]") {
  std::vector<kdtree::point<int, 2>> data = {{1, 1}, {1, 1}, {1, 1}, {5, 5}, {10, 10}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<int, 2> query(1, 1);
  auto it = kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), query);
  REQUIRE(kdtree::chebyshev_distance(query, *it) == 0);

  auto results =
      kdtree::nnsearch_kdtree<kdtree::chebyshev_metric>(data.cbegin(), data.cend(), query, 4);
  REQUIRE(results.size() == 4);
  // The 3 duplicates at distance 0, plus one more
  int zero_count = 0;
  for (auto const &r : results) {
    if (kdtree::chebyshev_distance(query, *r) == 0) {
      ++zero_count;
    }
  }
  REQUIRE(zero_count == 3);
}

// ============================================================
// radiusquery_kdtree with unsigned coordinate types (regression)
// ============================================================

TEST_CASE("radiusquery_kdtree with unsigned coordinates", "[kdtree][radius][unsigned]") {
  std::vector<kdtree::point<unsigned int, 2>> data = {
      {0u, 0u}, {1u, 0u}, {0u, 1u}, {3u, 3u}, {10u, 10u}};
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<unsigned int, 2> center(0u, 0u);
  unsigned int radius = 1;
  auto results = kdtree::radiusquery_kdtree(data.cbegin(), data.cend(), center, radius);

  // Points within radius 1 of origin: (0,0) at d=0, (1,0) at d=1, (0,1) at d=1
  REQUIRE(results.size() == 3);
  for (const auto &it : results) {
    auto dist = std::sqrt(static_cast<double>(kdtree::squared_euclidean_distance(center, *it)));
    REQUIRE(dist <= static_cast<double>(radius));
  }
}

TEST_CASE("radiusquery_kdtree with unsigned coordinates - center far from origin",
          "[kdtree][radius][unsigned]") {
  std::vector<kdtree::point<unsigned int, 2>> data = {
      {0u, 0u}, {5u, 5u}, {10u, 10u}, {15u, 15u}, {20u, 20u}};
  kdtree::make_kdtree(data.begin(), data.end());

  // Query centered at (10,10) with radius 1 — only (10,10) should match
  kdtree::point<unsigned int, 2> center(10u, 10u);
  unsigned int radius = 1;
  auto results = kdtree::radiusquery_kdtree(data.cbegin(), data.cend(), center, radius);

  REQUIRE(results.size() == 1);
  REQUIRE(*results[0] == kdtree::point<unsigned int, 2>(10u, 10u));
}
