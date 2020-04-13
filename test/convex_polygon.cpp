#include <vector>
#include "../include/point.hpp"
#include "../include/convex_polygon.hpp"

int main( int argc, char* argv[] ) {
	(void)argc;
	(void)argv;

	using point = kdtree::convex_polygon<double>::point;

	point point1( 1.0, 1.0 );
	point point2( 2.0, 2.0 );
	point point3( -1.0, 0.0 );
	point point4( 1.0, 0.2 );
	point point5( -2.4, -1.3 );
	point point6( -2.4, -1.30001 );
	kdtree::convex_polygon<double> polygon1 = { point(0.0,0.0), point(2.0,0.0), point(1.0,2.0) };
	kdtree::convex_polygon<double> polygon2 = { point(-2.4,-1.3), point(-1.7,-1.5), point(0.4,-1.2), point(1.9,1.3), point(-3.3,-1.2) };
	
	std::cerr << "result for point1 in polygon1: " << polygon1.contains( point1 ) << '\n';
	std::cerr << "result for point2 in polygon1: " << polygon1.contains( point2 ) << '\n';
	std::cerr << "result for point3 in polygon1: " << polygon1.contains( point3 ) << '\n';
	std::cerr << "result for point1 in polygon2: " << polygon2.contains( point1 ) << '\n';
	std::cerr << "result for point2 in polygon2: " << polygon2.contains( point2 ) << '\n';
	std::cerr << "result for point3 in polygon2: " << polygon2.contains( point3 ) << '\n';
	std::cerr << "result for point4 in polygon2: " << polygon2.contains( point4 ) << '\n';
	std::cerr << "result for point5 in polygon2: " << polygon2.contains( point5 ) << '\n';
	std::cerr << "result for point6 in polygon2: " << polygon2.contains( point6 ) << '\n';

	return 0;
}