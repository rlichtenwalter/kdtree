#include <sstream>
#include <stdexcept>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <kdtree/convex_polygon.hpp>
#include <kdtree/point.hpp>

// Note: convex_polygon.hpp leaks a 'point' alias from an anonymous namespace.
// Use fully qualified names to avoid ambiguity.
using dpoint = kdtree::point<double, 2>;

// ============================================================
// Construction
// ============================================================

TEST_CASE("convex_polygon valid triangle CCW", "[polygon][construction]") {
  kdtree::convex_polygon<double> poly = {dpoint(0.0, 0.0), dpoint(2.0, 0.0), dpoint(1.0, 2.0)};
  REQUIRE(poly.size() == 3);
}

TEST_CASE("convex_polygon valid triangle CW normalizes to CCW", "[polygon][construction]") {
  kdtree::convex_polygon<double> poly = {dpoint(0.0, 0.0), dpoint(1.0, 2.0), dpoint(2.0, 0.0)};
  REQUIRE(poly.size() == 3);
}

TEST_CASE("convex_polygon valid quadrilateral", "[polygon][construction]") {
  kdtree::convex_polygon<double> poly = {dpoint(0.0, 0.0), dpoint(2.0, 0.0), dpoint(2.0, 2.0),
                                         dpoint(0.0, 2.0)};
  REQUIRE(poly.size() == 4);
}

TEST_CASE("convex_polygon from iterator range", "[polygon][construction]") {
  std::vector<dpoint> pts = {dpoint(0.0, 0.0), dpoint(2.0, 0.0), dpoint(1.0, 2.0)};
  kdtree::convex_polygon<double> poly(pts.begin(), pts.end());
  REQUIRE(poly.size() == 3);
}

TEST_CASE("convex_polygon rejects fewer than 3 points", "[polygon][construction]") {
  std::vector<dpoint> pts = {dpoint(0.0, 0.0), dpoint(1.0, 1.0)};
  REQUIRE_THROWS(kdtree::convex_polygon<double>(pts.begin(), pts.end()));
}

TEST_CASE("convex_polygon rejects collinear points", "[polygon][construction]") {
  std::vector<dpoint> pts = {dpoint(0.0, 0.0), dpoint(1.0, 1.0), dpoint(2.0, 2.0)};
  REQUIRE_THROWS_AS(kdtree::convex_polygon<double>(pts.begin(), pts.end()), std::runtime_error);
}

TEST_CASE("convex_polygon rejects non-convex polygon", "[polygon][construction]") {
  // A non-convex quadrilateral (bowtie / self-intersecting)
  std::vector<dpoint> pts = {dpoint(0.0, 0.0), dpoint(2.0, 2.0), dpoint(2.0, 0.0),
                             dpoint(0.0, 2.0)};
  REQUIRE_THROWS_AS(kdtree::convex_polygon<double>(pts.begin(), pts.end()), std::runtime_error);
}

// ============================================================
// contains
// ============================================================

TEST_CASE("convex_polygon contains interior point", "[polygon][contains]") {
  kdtree::convex_polygon<double> poly = {dpoint(0.0, 0.0), dpoint(4.0, 0.0), dpoint(4.0, 4.0),
                                         dpoint(0.0, 4.0)};

  REQUIRE(poly.contains(dpoint(2.0, 2.0)));
  REQUIRE(poly.contains(dpoint(1.0, 1.0)));
  REQUIRE(poly.contains(dpoint(3.5, 3.5)));
}

TEST_CASE("convex_polygon excludes exterior point", "[polygon][contains]") {
  kdtree::convex_polygon<double> poly = {dpoint(0.0, 0.0), dpoint(4.0, 0.0), dpoint(4.0, 4.0),
                                         dpoint(0.0, 4.0)};

  REQUIRE_FALSE(poly.contains(dpoint(5.0, 5.0)));
  REQUIRE_FALSE(poly.contains(dpoint(-1.0, 2.0)));
  REQUIRE_FALSE(poly.contains(dpoint(2.0, -1.0)));
}

TEST_CASE("convex_polygon triangle containment", "[polygon][contains]") {
  kdtree::convex_polygon<double> tri = {dpoint(0.0, 0.0), dpoint(2.0, 0.0), dpoint(1.0, 2.0)};

  REQUIRE(tri.contains(dpoint(1.0, 0.5)));        // interior
  REQUIRE_FALSE(tri.contains(dpoint(2.0, 2.0)));  // exterior
  REQUIRE_FALSE(tri.contains(dpoint(-1.0, 0.0))); // exterior
}

TEST_CASE("convex_polygon CW triangle has same containment as CCW", "[polygon][contains]") {
  kdtree::convex_polygon<double> ccw = {dpoint(0.0, 0.0), dpoint(4.0, 0.0), dpoint(2.0, 4.0)};
  kdtree::convex_polygon<double> cw = {dpoint(0.0, 0.0), dpoint(2.0, 4.0), dpoint(4.0, 0.0)};

  dpoint inside(2.0, 1.0);
  dpoint outside(5.0, 5.0);

  REQUIRE(ccw.contains(inside));
  REQUIRE(cw.contains(inside));
  REQUIRE_FALSE(ccw.contains(outside));
  REQUIRE_FALSE(cw.contains(outside));
}

// ============================================================
// Iterators
// ============================================================

TEST_CASE("convex_polygon iterators traverse vertices", "[polygon][iterator]") {
  kdtree::convex_polygon<double> poly = {dpoint(0.0, 0.0), dpoint(2.0, 0.0), dpoint(1.0, 2.0)};

  std::size_t count = 0;
  for (auto it = poly.begin(); it != poly.end(); ++it) {
    ++count;
  }
  REQUIRE(count == poly.size());
}

// ============================================================
// Stream I/O
// ============================================================

TEST_CASE("convex_polygon output format", "[polygon][io]") {
  using ipoint = kdtree::point<int, 2>;
  kdtree::convex_polygon<int> poly = {ipoint(0, 0), ipoint(2, 0), ipoint(1, 2)};

  std::ostringstream oss;
  oss << poly;
  std::string output = oss.str();

  REQUIRE(output.front() == '[');
  REQUIRE(output.back() == ']');
}

TEST_CASE("convex_polygon roundtrip through streams", "[polygon][io]") {
  using ipoint = kdtree::point<int, 2>;
  kdtree::convex_polygon<int> original = {ipoint(0, 0), ipoint(3, 0), ipoint(1, 3)};

  std::ostringstream oss;
  oss << original;

  kdtree::convex_polygon<int> parsed;
  std::istringstream iss(oss.str());
  iss >> parsed;

  REQUIRE(parsed.size() == original.size());

  // Verify containment behavior matches
  ipoint inside(1, 1);
  ipoint outside(10, 10);
  REQUIRE(original.contains(inside) == parsed.contains(inside));
  REQUIRE(original.contains(outside) == parsed.contains(outside));
}
