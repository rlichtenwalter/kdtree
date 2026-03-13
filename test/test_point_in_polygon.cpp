#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <point.hpp>
#include <point_in_polygon.hpp>

using point = kdtree::point<double, 2>;

// ============================================================
// Triangle containment
// ============================================================

TEST_CASE("point_in_polygon triangle interior", "[pip]") {
  std::vector<point> triangle = {point(0.0, 0.0), point(4.0, 0.0), point(2.0, 4.0)};

  REQUIRE(kdtree::point_in_polygon(point(2.0, 1.0), triangle.begin(), triangle.end()));
  REQUIRE(kdtree::point_in_polygon(point(1.0, 0.5), triangle.begin(), triangle.end()));
}

TEST_CASE("point_in_polygon triangle exterior", "[pip]") {
  std::vector<point> triangle = {point(0.0, 0.0), point(4.0, 0.0), point(2.0, 4.0)};

  REQUIRE_FALSE(kdtree::point_in_polygon(point(5.0, 5.0), triangle.begin(), triangle.end()));
  REQUIRE_FALSE(kdtree::point_in_polygon(point(-1.0, -1.0), triangle.begin(), triangle.end()));
  REQUIRE_FALSE(kdtree::point_in_polygon(point(0.0, 3.0), triangle.begin(), triangle.end()));
}

// ============================================================
// Square containment
// ============================================================

TEST_CASE("point_in_polygon square", "[pip]") {
  std::vector<point> square = {point(0.0, 0.0), point(2.0, 0.0), point(2.0, 2.0), point(0.0, 2.0)};

  SECTION("interior points") {
    REQUIRE(kdtree::point_in_polygon(point(1.0, 1.0), square.begin(), square.end()));
    REQUIRE(kdtree::point_in_polygon(point(0.5, 0.5), square.begin(), square.end()));
    REQUIRE(kdtree::point_in_polygon(point(1.5, 1.5), square.begin(), square.end()));
  }

  SECTION("exterior points") {
    REQUIRE_FALSE(kdtree::point_in_polygon(point(3.0, 1.0), square.begin(), square.end()));
    REQUIRE_FALSE(kdtree::point_in_polygon(point(-1.0, 1.0), square.begin(), square.end()));
    REQUIRE_FALSE(kdtree::point_in_polygon(point(1.0, 3.0), square.begin(), square.end()));
    REQUIRE_FALSE(kdtree::point_in_polygon(point(1.0, -1.0), square.begin(), square.end()));
  }
}

// ============================================================
// Pentagon containment
// ============================================================

TEST_CASE("point_in_polygon pentagon", "[pip]") {
  // Regular-ish convex pentagon
  std::vector<point> pentagon = {point(2.0, 0.0), point(4.0, 1.5), point(3.0, 4.0), point(1.0, 4.0),
                                 point(0.0, 1.5)};

  REQUIRE(kdtree::point_in_polygon(point(2.0, 2.0), pentagon.begin(), pentagon.end()));
  REQUIRE_FALSE(kdtree::point_in_polygon(point(5.0, 5.0), pentagon.begin(), pentagon.end()));
  REQUIRE_FALSE(kdtree::point_in_polygon(point(-1.0, 0.0), pentagon.begin(), pentagon.end()));
}

// ============================================================
// Non-convex polygon
// ============================================================

TEST_CASE("point_in_polygon non-convex (L-shape)", "[pip]") {
  // L-shaped polygon (non-convex)
  std::vector<point> l_shape = {point(0.0, 0.0), point(2.0, 0.0), point(2.0, 1.0),
                                point(1.0, 1.0), point(1.0, 2.0), point(0.0, 2.0)};

  REQUIRE(kdtree::point_in_polygon(point(0.5, 0.5), l_shape.begin(), l_shape.end()));
  REQUIRE(kdtree::point_in_polygon(point(0.5, 1.5), l_shape.begin(), l_shape.end()));
  // In the concave notch — should be outside
  REQUIRE_FALSE(kdtree::point_in_polygon(point(1.5, 1.5), l_shape.begin(), l_shape.end()));
}

// ============================================================
// Edge cases
// ============================================================

TEST_CASE("point_in_polygon far away point", "[pip]") {
  std::vector<point> triangle = {point(0.0, 0.0), point(1.0, 0.0), point(0.5, 1.0)};

  REQUIRE_FALSE(kdtree::point_in_polygon(point(1000.0, 1000.0), triangle.begin(), triangle.end()));
}

TEST_CASE("point_in_polygon point at origin with offset polygon", "[pip]") {
  std::vector<point> triangle = {point(10.0, 10.0), point(20.0, 10.0), point(15.0, 20.0)};

  REQUIRE_FALSE(kdtree::point_in_polygon(point(0.0, 0.0), triangle.begin(), triangle.end()));
  REQUIRE(kdtree::point_in_polygon(point(15.0, 12.0), triangle.begin(), triangle.end()));
}
