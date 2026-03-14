#include <cstddef>
#include <random>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <kdtree/kdtree.hpp>
#include <kdtree/point.hpp>

// --- Random data generation ---

template <typename T, std::size_t d>
std::vector<kdtree::point<T, d>> generate_random_points(std::size_t n, unsigned seed = 42) {
  std::mt19937 gen(seed);
  std::uniform_real_distribution<T> dist(-1000.0, 1000.0);
  std::vector<kdtree::point<T, d>> points(n);
  for (auto &p : points) {
    for (std::size_t i = 0; i < d; ++i) {
      p[i] = dist(gen);
    }
  }
  return points;
}

// ============================================================
// Construction benchmarks
// ============================================================

TEST_CASE("bench: make_kdtree construction", "[!benchmark][construction]") {
  BENCHMARK_ADVANCED("2D double 1K")(Catch::Benchmark::Chronometer meter) {
    auto data = generate_random_points<double, 2>(1000);
    meter.measure([&] { kdtree::make_kdtree(data.begin(), data.end()); });
  };

  BENCHMARK_ADVANCED("2D double 10K")(Catch::Benchmark::Chronometer meter) {
    auto data = generate_random_points<double, 2>(10000);
    meter.measure([&] { kdtree::make_kdtree(data.begin(), data.end()); });
  };

  BENCHMARK_ADVANCED("2D double 100K")(Catch::Benchmark::Chronometer meter) {
    auto data = generate_random_points<double, 2>(100000);
    meter.measure([&] { kdtree::make_kdtree(data.begin(), data.end()); });
  };

  BENCHMARK_ADVANCED("2D double 1M")(Catch::Benchmark::Chronometer meter) {
    auto data = generate_random_points<double, 2>(1000000);
    meter.measure([&] { kdtree::make_kdtree(data.begin(), data.end()); });
  };

  BENCHMARK_ADVANCED("3D double 100K")(Catch::Benchmark::Chronometer meter) {
    auto data = generate_random_points<double, 3>(100000);
    meter.measure([&] { kdtree::make_kdtree(data.begin(), data.end()); });
  };

  BENCHMARK_ADVANCED("2D float 100K")(Catch::Benchmark::Chronometer meter) {
    auto data = generate_random_points<float, 2>(100000);
    meter.measure([&] { kdtree::make_kdtree(data.begin(), data.end()); });
  };
}

// ============================================================
// 1-NN search benchmarks
// ============================================================

TEST_CASE("bench: nnsearch_kdtree 1-NN", "[!benchmark][nn]") {
  auto data = generate_random_points<double, 2>(100000);
  kdtree::make_kdtree(data.begin(), data.end());
  auto queries = generate_random_points<double, 2>(1000, 123);

  BENCHMARK_ADVANCED("2D double 100K tree, 1000 queries")(Catch::Benchmark::Chronometer meter) {
    meter.measure([&] {
      for (auto const &q : queries) {
        kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), q);
      }
    });
  };

  auto data3d = generate_random_points<double, 3>(100000);
  kdtree::make_kdtree(data3d.begin(), data3d.end());
  auto queries3d = generate_random_points<double, 3>(1000, 123);

  BENCHMARK_ADVANCED("3D double 100K tree, 1000 queries")(Catch::Benchmark::Chronometer meter) {
    meter.measure([&] {
      for (auto const &q : queries3d) {
        kdtree::nnsearch_kdtree(data3d.cbegin(), data3d.cend(), q);
      }
    });
  };
}

// ============================================================
// kNN search benchmarks
// ============================================================

TEST_CASE("bench: nnsearch_kdtree kNN", "[!benchmark][knn]") {
  auto data = generate_random_points<double, 2>(100000);
  kdtree::make_kdtree(data.begin(), data.end());
  auto queries = generate_random_points<double, 2>(100, 123);

  BENCHMARK_ADVANCED("2D 100K tree, k=10, 100 queries")(Catch::Benchmark::Chronometer meter) {
    meter.measure([&] {
      for (auto const &q : queries) {
        kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), q, 10);
      }
    });
  };

  BENCHMARK_ADVANCED("2D 100K tree, k=100, 100 queries")(Catch::Benchmark::Chronometer meter) {
    meter.measure([&] {
      for (auto const &q : queries) {
        kdtree::nnsearch_kdtree(data.cbegin(), data.cend(), q, 100);
      }
    });
  };
}

// ============================================================
// Range query benchmarks
// ============================================================

TEST_CASE("bench: rangequery_kdtree", "[!benchmark][range]") {
  auto data = generate_random_points<double, 2>(100000);
  kdtree::make_kdtree(data.begin(), data.end());

  kdtree::point<double, 2> lower(-10.0, -10.0);
  kdtree::point<double, 2> upper(10.0, 10.0);

  BENCHMARK_ADVANCED("2D 100K tree, small range, 100 queries")(
      Catch::Benchmark::Chronometer meter) {
    meter.measure([&] {
      for (int i = 0; i < 100; ++i) {
        kdtree::rangequery_kdtree(data.cbegin(), data.cend(), lower, upper);
      }
    });
  };

  BENCHMARK_ADVANCED("2D 100K tree, small range, 100 queries (output iterator)")(
      Catch::Benchmark::Chronometer meter) {
    std::vector<decltype(data.cbegin())> results;
    meter.measure([&] {
      for (int i = 0; i < 100; ++i) {
        results.clear();
        kdtree::rangequery_kdtree(data.cbegin(), data.cend(), lower, upper,
                                  std::back_inserter(results));
      }
    });
  };
}

// ============================================================
// Radius query benchmarks
// ============================================================

TEST_CASE("bench: radiusquery_kdtree", "[!benchmark][radius]") {
  auto data = generate_random_points<double, 2>(100000);
  kdtree::make_kdtree(data.begin(), data.end());
  auto queries = generate_random_points<double, 2>(100, 123);

  BENCHMARK_ADVANCED("2D 100K tree, r=50, 100 queries")(Catch::Benchmark::Chronometer meter) {
    meter.measure([&] {
      for (auto const &q : queries) {
        kdtree::radiusquery_kdtree(data.cbegin(), data.cend(), q, 50.0);
      }
    });
  };

  BENCHMARK_ADVANCED("2D 100K tree, r=50, 100 queries (output iterator)")(
      Catch::Benchmark::Chronometer meter) {
    std::vector<decltype(data.cbegin())> results;
    meter.measure([&] {
      for (auto const &q : queries) {
        results.clear();
        kdtree::radiusquery_kdtree(data.cbegin(), data.cend(), q, 50.0,
                                   std::back_inserter(results));
      }
    });
  };
}
