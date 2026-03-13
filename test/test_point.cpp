#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_set>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <point.hpp>

using Catch::Matchers::WithinAbs;

// --- Construction ---

TEST_CASE("point default construction", "[point][construction]") {
  kdtree::point<int, 2> p;
  // default-constructed std::array has indeterminate values for non-class types,
  // so we just verify it compiles and is usable
  p[0] = 1;
  p[1] = 2;
  REQUIRE(p[0] == 1);
  REQUIRE(p[1] == 2);
}

TEST_CASE("point variadic construction", "[point][construction]") {
  kdtree::point<int, 3> p(1, 2, 3);
  REQUIRE(p[0] == 1);
  REQUIRE(p[1] == 2);
  REQUIRE(p[2] == 3);
}

TEST_CASE("point brace initialization", "[point][construction]") {
  kdtree::point<double, 2> p = {3.14, 2.72};
  REQUIRE_THAT(p[0], WithinAbs(3.14, 1e-10));
  REQUIRE_THAT(p[1], WithinAbs(2.72, 1e-10));
}

// --- Static properties ---

TEST_CASE("point dimensionality", "[point]") {
  REQUIRE(kdtree::point<int, 2>::dimensionality() == 2);
  REQUIRE(kdtree::point<float, 5>::dimensionality() == 5);
  REQUIRE(kdtree::point<double, 1>::dimensionality() == 1);
}

// --- Element access ---

TEST_CASE("point operator[] read and write", "[point]") {
  kdtree::point<int, 3> p(10, 20, 30);
  REQUIRE(p[0] == 10);
  p[0] = 99;
  REQUIRE(p[0] == 99);
}

TEST_CASE("point const operator[]", "[point]") {
  const kdtree::point<int, 2> p(5, 10);
  REQUIRE(p[0] == 5);
  REQUIRE(p[1] == 10);
}

// --- Comparison operators ---

TEST_CASE("point equality", "[point][comparison]") {
  kdtree::point<int, 2> a(1, 2);
  kdtree::point<int, 2> b(1, 2);
  kdtree::point<int, 2> c(1, 3);

  REQUIRE(a == b);
  REQUIRE_FALSE(a == c);
}

TEST_CASE("point less-than (lexicographic)", "[point][comparison]") {
  kdtree::point<int, 2> a(1, 2);
  kdtree::point<int, 2> b(1, 3);
  kdtree::point<int, 2> c(2, 0);

  REQUIRE(a < b);
  REQUIRE(a < c);
  REQUIRE_FALSE(b < a);
  REQUIRE_FALSE(a < a);
}

// --- Arithmetic operators ---

TEST_CASE("point addition", "[point][arithmetic]") {
  kdtree::point<int, 3> a(1, 2, 3);
  kdtree::point<int, 3> b(4, 5, 6);
  auto c = a + b;
  REQUIRE(c[0] == 5);
  REQUIRE(c[1] == 7);
  REQUIRE(c[2] == 9);
}

TEST_CASE("point subtraction", "[point][arithmetic]") {
  kdtree::point<int, 3> a(10, 20, 30);
  kdtree::point<int, 3> b(1, 2, 3);
  auto c = a - b;
  REQUIRE(c[0] == 9);
  REQUIRE(c[1] == 18);
  REQUIRE(c[2] == 27);
}

// --- Iterators ---

TEST_CASE("point iterators", "[point][iterator]") {
  kdtree::point<int, 3> p(10, 20, 30);

  SECTION("forward iteration") {
    std::vector<int> values(p.begin(), p.end());
    REQUIRE(values == std::vector<int>{10, 20, 30});
  }

  SECTION("const iteration") {
    const auto &cp = p;
    std::vector<int> values(cp.begin(), cp.end());
    REQUIRE(values == std::vector<int>{10, 20, 30});
  }

  SECTION("reverse iteration") {
    std::vector<int> values(p.rbegin(), p.rend());
    REQUIRE(values == std::vector<int>{30, 20, 10});
  }

  SECTION("mutation via iterator") {
    *p.begin() = 99;
    REQUIRE(p[0] == 99);
  }
}

// --- Stream output ---

TEST_CASE("point output format", "[point][io]") {
  SECTION("2D int") {
    kdtree::point<int, 2> p(3, 7);
    std::ostringstream oss;
    oss << p;
    REQUIRE(oss.str() == "(3,7)");
  }

  SECTION("3D float") {
    kdtree::point<float, 3> p(1.5f, 2.5f, 3.5f);
    std::ostringstream oss;
    oss << p;
    REQUIRE(oss.str() == "(1.5,2.5,3.5)");
  }

  SECTION("1D") {
    kdtree::point<int, 1> p(42);
    std::ostringstream oss;
    oss << p;
    REQUIRE(oss.str() == "(42)");
  }
}

// --- Stream input ---

TEST_CASE("point input parsing", "[point][io]") {
  SECTION("valid 2D int") {
    kdtree::point<int, 2> p;
    std::istringstream iss("(3,7)");
    iss >> p;
    REQUIRE(p[0] == 3);
    REQUIRE(p[1] == 7);
  }

  SECTION("valid 2D float") {
    kdtree::point<float, 2> p;
    std::istringstream iss("(4.2,-3.7)");
    iss >> p;
    REQUIRE_THAT(p[0], WithinAbs(4.2, 0.01));
    REQUIRE_THAT(p[1], WithinAbs(-3.7, 0.01));
  }

  SECTION("valid 3D with whitespace") {
    kdtree::point<int, 3> p;
    std::istringstream iss("( 1 , 2 , 3 )");
    iss >> p;
    REQUIRE(p[0] == 1);
    REQUIRE(p[1] == 2);
    REQUIRE(p[2] == 3);
  }

  SECTION("missing opening paren throws") {
    kdtree::point<int, 2> p;
    std::istringstream iss("[1,2]");
    REQUIRE_THROWS_AS(iss >> p, std::range_error);
  }

  SECTION("missing closing paren throws") {
    kdtree::point<int, 2> p;
    std::istringstream iss("(1,2]");
    REQUIRE_THROWS_AS(iss >> p, std::range_error);
  }

  SECTION("missing comma throws") {
    kdtree::point<int, 2> p;
    std::istringstream iss("(1 2)");
    REQUIRE_THROWS_AS(iss >> p, std::range_error);
  }
}

TEST_CASE("point roundtrip through streams", "[point][io]") {
  kdtree::point<int, 3> original(5, -3, 17);
  std::ostringstream oss;
  oss << original;

  kdtree::point<int, 3> parsed;
  std::istringstream iss(oss.str());
  iss >> parsed;

  REQUIRE(original == parsed);
}

// --- Squared Euclidean distance ---

TEST_CASE("squared_euclidean_distance with int points", "[point][distance]") {
  kdtree::point<int, 2> a(0, 0);
  kdtree::point<int, 2> b(3, 4);
  REQUIRE(kdtree::squared_euclidean_distance(a, b) == 25);
}

TEST_CASE("squared_euclidean_distance same point is zero", "[point][distance]") {
  kdtree::point<int, 3> p(5, 10, 15);
  REQUIRE(kdtree::squared_euclidean_distance(p, p) == 0);
}

TEST_CASE("squared_euclidean_distance is symmetric", "[point][distance]") {
  kdtree::point<double, 2> a(1.5, 2.5);
  kdtree::point<double, 2> b(-0.5, 3.5);
  REQUIRE_THAT(kdtree::squared_euclidean_distance(a, b),
               WithinAbs(kdtree::squared_euclidean_distance(b, a), 1e-10));
}

TEST_CASE("squared_euclidean_distance 3D", "[point][distance]") {
  kdtree::point<double, 3> a(1.0, 2.0, 3.0);
  kdtree::point<double, 3> b(4.0, 6.0, 3.0);
  // (3^2 + 4^2 + 0^2) = 25
  REQUIRE_THAT(kdtree::squared_euclidean_distance(a, b), WithinAbs(25.0, 1e-10));
}

TEST_CASE("squared_euclidean_distance with negative coordinates", "[point][distance]") {
  kdtree::point<int, 2> a(-3, -4);
  kdtree::point<int, 2> b(0, 0);
  REQUIRE(kdtree::squared_euclidean_distance(a, b) == 25);
}

// --- Hash ---

TEST_CASE("point hash: equal points hash equal", "[point][hash]") {
  kdtree::point<int, 2> a(1, 2);
  kdtree::point<int, 2> b(1, 2);
  std::hash<kdtree::point<int, 2>> hasher;
  REQUIRE(hasher(a) == hasher(b));
}

TEST_CASE("point hash: can be used in unordered_set", "[point][hash]") {
  std::unordered_set<kdtree::point<int, 2>> s;
  s.insert(kdtree::point<int, 2>(1, 2));
  s.insert(kdtree::point<int, 2>(3, 4));
  s.insert(kdtree::point<int, 2>(1, 2)); // duplicate
  REQUIRE(s.size() == 2);
}

TEST_CASE("point hash: permuted coordinates should ideally differ", "[point][hash]") {
  // Note: the current XOR-based hash produces identical hashes for
  // permuted coordinates. This test documents the known weakness.
  kdtree::point<int, 2> a(1, 2);
  kdtree::point<int, 2> b(2, 1);
  std::hash<kdtree::point<int, 2>> hasher;
  // These SHOULD differ for a good hash, but currently don't.
  // When the hash is fixed, change REQUIRE to REQUIRE_FALSE.
  REQUIRE(hasher(a) == hasher(b));
}
