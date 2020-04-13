#include <vector>
#include "../include/point.hpp"
#include "../include/point_in_polygon.hpp"

int main( int argc, char* argv[] ) {
	(void)argc;
	(void)argv;

	using point = kdtree::point<double,2>;

	point point1( 1.0, 1.0 );
	point point2( 2.0, 2.0 );
	point point3( -1.0, 0.0 );
	point point4( 1.0, 0.2 );
	point point5( -2.4, -1.3 );
	point point6( -2.4, -1.30001 );
	std::vector<point> polygon1 = { point(0.0,0.0), point(2.0,0.0), point(1.0,2.0) };
	std::vector<point> polygon2 = { point(-2.4,-1.3), point(-1.7,-1.5), point(0.4,-2.2), point(1.9,1.3), point(-3.3,-1.5) };

	std::cerr << "result for point1 in polygon1: " << kdtree::point_in_polygon( point1, polygon1.begin(), polygon1.end() ) << '\n';
	std::cerr << "result for point2 in polygon1: " << kdtree::point_in_polygon( point2, polygon1.begin(), polygon1.end() ) << '\n';
	std::cerr << "result for point3 in polygon1: " << kdtree::point_in_polygon( point3, polygon1.begin(), polygon1.end() ) << '\n';
	std::cerr << "result for point1 in polygon2: " << kdtree::point_in_polygon( point1, polygon2.begin(), polygon2.end() ) << '\n';
	std::cerr << "result for point2 in polygon2: " << kdtree::point_in_polygon( point2, polygon2.begin(), polygon2.end() ) << '\n';
	std::cerr << "result for point3 in polygon2: " << kdtree::point_in_polygon( point3, polygon2.begin(), polygon2.end() ) << '\n';
	std::cerr << "result for point4 in polygon2: " << kdtree::point_in_polygon( point4, polygon2.begin(), polygon2.end() ) << '\n';
	std::cerr << "result for point5 in polygon2: " << kdtree::point_in_polygon( point5, polygon2.begin(), polygon2.end() ) << '\n';
	std::cerr << "result for point6 in polygon2: " << kdtree::point_in_polygon( point6, polygon2.begin(), polygon2.end() ) << '\n';

	return 0;
}