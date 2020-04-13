#include <array>
#include <cstddef>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "../include/kdtree.hpp"
#include "../include/point.hpp"

using intpoint = kdtree::point<int,2>;
using floatpoint = kdtree::point<float,2>;
using highdpoint = kdtree::point<int,3>;

int main( int argc, char* argv[] ) {
	(void)argc;
	(void)argv;

	std::cout << "Testing T=int:\n\n";

	{
		intpoint x1 = {1, 3};
		intpoint x2 = {2, 7};
		intpoint x3 = {-3, 6};
		intpoint x4 = {-2, -1};
		intpoint x5 = {-7, 4};
		intpoint x6 = {2, 3};
		intpoint x7 = {-5, 2};
		intpoint x8 = {-1, 9};
		intpoint x9 = {6, -3};
		intpoint x10 = {-4, 0};
		intpoint x11 = {0, -1};
		intpoint x12 = {-2, -1 };
		intpoint x13 = {3,3};
		std::vector<intpoint> data = { x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13 };

	//	kdtree::print_kdtree( std::cerr, data.begin(), data.end() );
		kdtree::make_kdtree( data.begin(), data.end() );
		kdtree::print_kdtree( std::cout, data.cbegin(), data.cend() );

	//	std::copy( data.begin(), data.end(), std::ostream_iterator<point>( std::cout, "\n" ) );

		intpoint exact = x7;
		auto exact_match_location = kdtree::search_kdtree( data.cbegin(), data.cend(), exact );
		std::cout << "\nExact match for " << exact << ":\n";
		std::cout << *exact_match_location << "\n";

		intpoint p = {-1,-1};
		auto nn_location = kdtree::nnsearch_kdtree( data.cbegin(), data.cend(), p );
		std::cout << "\nNearest neighbor of " << p << ":\n";
		std::cout << *nn_location << "\n";

		intpoint knn_point = {-1,-1};
		auto knn_locations = kdtree::nnsearch_kdtree( data.cbegin(), data.cend(), knn_point, 4 );
		std::cout << "\nk nearest neighbors of " << knn_point << " with k=4:\n";
		for( auto location : knn_locations ) {
			std::cout << *location << "\n";
		}

		intpoint lower = {-2,-3};
		intpoint upper = {3,3};
		auto range_locations = kdtree::rangequery_kdtree( data.cbegin(), data.cend(), lower, upper );
		std::cout << "\nRange query within [" << lower << "," << upper << "]:\n";
		for( auto location : range_locations ) {
			std::cout << *location << "\n";
		}
	}

	std::cout << "\n\nTesting T=float:\n\n";

	{
		floatpoint x1 = {1.0f, 3.0f};
		floatpoint x2 = {2.0f, 7.0f};
		floatpoint x3 = {-3.0f, 6.0f};
		floatpoint x4 = {-2.0f, -1.0f};
		floatpoint x5 = {-7.0f, 4.0f};
		floatpoint x6 = {2.0f, 3.0f};
		floatpoint x7 = {-5.0f, 2.0f};
		floatpoint x8 = {-1.0f, 9.0f};
		floatpoint x9 = {6.0f, -3.0f};
		floatpoint x10 = {-4.0f, 0.0f};
		floatpoint x11 = {0.0f, -1.0f};
		floatpoint x12 = {-2.0f, -1.0f };
		floatpoint x13 = {3.0f,3.0f};
		std::vector<floatpoint> data = { x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13 };

	//	kdtree::print_kdtree( std::cerr, data.begin(), data.end() );
		kdtree::make_kdtree( data.begin(), data.end() );
		kdtree::print_kdtree( std::cout, data.cbegin(), data.cend() );

	//	std::copy( data.begin(), data.end(), std::ostream_iterator<point>( std::cout, "\n" ) );

		floatpoint exact = x7;
		auto exact_match_location = kdtree::search_kdtree( data.cbegin(), data.cend(), exact );
		std::cout << "\nExact match for " << exact << ":\n";
		std::cout << *exact_match_location << "\n";

		floatpoint p = {-1.0f,-1.0f};
		auto nn_location = kdtree::nnsearch_kdtree( data.cbegin(), data.cend(), p );
		std::cout << "\nNearest neighbor of " << p << ":\n";
		std::cout << *nn_location << "\n";

		floatpoint knn_point = {-1.0f,-1.0f};
		auto knn_locations = kdtree::nnsearch_kdtree( data.cbegin(), data.cend(), knn_point, 4 );
		std::cout << "\nk nearest neighbors of " << knn_point << " with k=4:\n";
		for( auto location : knn_locations ) {
			std::cout << *location << "\n";
		}

		floatpoint lower = {-2.0f,-3.0f};
		floatpoint upper = {3.0f,3.0f};
		auto range_locations = kdtree::rangequery_kdtree( data.cbegin(), data.cend(), lower, upper );
		std::cout << "\nRange query within [" << lower << "," << upper << "]:\n";
		for( auto location : range_locations ) {
			std::cout << *location << "\n";
		}
	}

	std::cout << "\n\nTesting d=3:\n\n";

	{
		highdpoint x1 = {1, 3, 1};
		highdpoint x2 = {2, 7, 1};
		highdpoint x3 = {-3, 6, 1};
		highdpoint x4 = {-2, -1, 1};
		highdpoint x5 = {-7, 4, 1};
		highdpoint x6 = {2, 3, 1};
		highdpoint x7 = {-5, 2, 1};
		highdpoint x8 = {-1, 9, 1};
		highdpoint x9 = {6, -3, 1};
		highdpoint x10 = {-4, 0, 1};
		highdpoint x11 = {0, -1, 1};
		highdpoint x12 = {-2, -1, 1 };
		highdpoint x13 = {3,3, 1};
		std::vector<highdpoint> data = { x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13 };

	//	kdtree::print_kdtree( std::cerr, data.begin(), data.end() );
		kdtree::make_kdtree( data.begin(), data.end() );
		kdtree::print_kdtree( std::cout, data.cbegin(), data.cend() );

	//	std::copy( data.begin(), data.end(), std::ostream_iterator<point>( std::cout, "\n" ) );

		highdpoint exact = x7;
		auto exact_match_location = kdtree::search_kdtree( data.cbegin(), data.cend(), exact );
		std::cout << "\nExact match for " << exact << ":\n";
		std::cout << *exact_match_location << "\n";

		highdpoint p = {-1,-1, 1};
		auto nn_location = kdtree::nnsearch_kdtree( data.cbegin(), data.cend(), p );
		std::cout << "\nNearest neighbor of " << p << ":\n";
		std::cout << *nn_location << "\n";

		highdpoint knn_point = {-1,-1, 1};
		auto knn_locations = kdtree::nnsearch_kdtree( data.cbegin(), data.cend(), knn_point, 4 );
		std::cout << "\nk nearest neighbors of " << knn_point << " with k=4:\n";
		for( auto location : knn_locations ) {
			std::cout << *location << "\n";
		}

		highdpoint lower = {-2,-3, 1};
		highdpoint upper = {3,3, 1};
		auto range_locations = kdtree::rangequery_kdtree( data.cbegin(), data.cend(), lower, upper );
		std::cout << "\nRange query within [" << lower << "," << upper << "]:\n";
		for( auto location : range_locations ) {
			std::cout << *location << "\n";
		}
	}


/*
	std::string line;
	while( std::getline( std::cin, line ) ) {
		point p;
		std::size_t index = 0;
		std::size_t end = 0;
		while( true ) {
			std::size_t begin = line.find_first_of( "-0123456789.", end );
			end = line.find_first_of( " ,)]}", begin );
			if( end == std::string::npos ) {
				break;
			}
			std::string numeric_string = line.substr( begin, end - begin );
			std::stringstream ss( numeric_string );
			coordinate xi;
			ss >> xi;
			p[ index++ ] = xi;
		}
		it = kdtree::nnsearch_kdtree( data.begin(), data.end(), p );
		print( *it );
		std::cout << "\n";
	}
*/
	return 0;
}
