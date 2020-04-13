#include <sstream>
#include "../include/point.hpp"


int main( int argc, char* argv[] ) {
	(void)argc;
	(void)argv;

	using coordinate = float;
	using point = kdtree::point<coordinate,2>;

	point p = {1.0f,2.0f};
	std::cout << p << '\n';

	std::stringstream ss( "(4.2,-3.7)" );
	ss >> p;
	std::cout << p << '\n';

	return 0;
}
