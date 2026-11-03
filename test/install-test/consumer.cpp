// Standalone consumer compiled against an *installed* kdtree tree only.
//
// The rest of the test suite consumes the headers through the library's
// BUILD_INTERFACE, i.e. straight out of the source tree, so it stays green
// even when the FILE_SET HEADERS list omits a header and the installation is
// therefore uncompilable. This translation unit is built by a separate CMake
// project that locates kdtree via find_package(), with no path into the
// source tree at all. It includes every public header.

#include <iostream>
#include <vector>

#include <kdtree/convex_polygon.hpp>
#include <kdtree/kdtree.hpp>
#include <kdtree/point.hpp>
#include <kdtree/point_in_polygon.hpp>

int main() {
  using point_type = kdtree::point<double, 2>;

  std::vector<point_type> points{point_type{0.0, 0.0}, point_type{1.0, 1.0}, point_type{5.0, 5.0},
                                 point_type{9.0, 9.0}};
  kdtree::make_kdtree(points.begin(), points.end());

  auto const nearest = kdtree::nnsearch_kdtree(points.begin(), points.end(), point_type{4.0, 4.0});

  if (nearest == points.end() || (*nearest)[0] != 5.0 || (*nearest)[1] != 5.0) {
    std::cerr << "unexpected nearest neighbour\n";
    return 1;
  }

  std::cout << "installed-tree consumer found the nearest neighbour of " << points.size()
            << " points\n";
  return 0;
}
