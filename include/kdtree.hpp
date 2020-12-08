#ifndef KDTREE_KDTREE_HPP
#define KDTREE_KDTREE_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <limits>
#include <queue>
#include <string>
#include <utility>

/*
Eventually, because it's really easy to implement, the nearest neighbor functions should accept a
distance function parameter.
*/

namespace {

	using dimension_type = std::size_t;
	using depth_type = std::size_t;
	using distance_type = float;

	dimension_type dimension( dimension_type dimensionality, depth_type depth ) {
		return depth % dimensionality;
	}

	template <class InputIt1, class InputIt2>
	distance_type squared_euclidean_distance( InputIt1 first1, InputIt1 last1, InputIt2 first2 ) {
		/* AWAIT BETTER C++ 2020 SUPPORT
		auto sum_function = []( auto accum, auto element ) { return accum + element; };
		auto product_function = []( auto xi1, auto xi2 ) { return std::pow( xi1 - xi2, 2 ); };
		return std::inner_product( first1, last1, first2, static_cast<distance_type>( 0 ), sum_function, product_function )
		*/
		distance_type dist = 0;
		while( first1 != last1 ) {
			dist += std::pow( *first1 - *first2, 2 );
			++first1;
			++first2;
		}
		return dist;
	}
	
	template <class RandomAccessIterator, class Point>
	void update_minimum_distance( RandomAccessIterator it, Point const & p, distance_type & mindist, RandomAccessIterator & closest ) {
		distance_type dist = squared_euclidean_distance( it->begin(), it->end(), p.begin() );
		if( dist < mindist ) {
			mindist = dist;
			closest = it;
		}
	}

	template <class RandomAccessIterator, class Point, class PriorityQueue>
	void update_priority_queue( RandomAccessIterator it, Point const & p, PriorityQueue & pq, std::size_t k ) {
		distance_type dist = squared_euclidean_distance( it->begin(), it->end(), p.begin() );
		if( pq.size() < k ) {
			pq.emplace( dist, it );
		} else {
			if( dist < pq.top().first ) {
				pq.pop();
				pq.emplace( dist, it );
			}
		}
//		std::cerr << "\npq.size(): " << pq.size() << " - pq.top().first: " << pq.top().first << " - dist: " << dist << '\n';
	}

	template <class Point>
	bool hypercube_contains( Point const & lower, Point const & upper, Point const & needle ) {
		for( std::size_t i = 0; i < Point::dimensionality(); ++i ) {
			if( needle[ i ] < lower[ i ]  || needle[ i ] > upper[ i ] ) {
				return false;
			}
		}
		return true;
	}
	
	template <class RandomAccessIterator>
	void make_kdtree_helper( RandomAccessIterator begin, RandomAccessIterator end, depth_type depth ) {
		dimension_type dim = dimension( begin->dimensionality(), depth );
		std::size_t n = end - begin;
		if( n > 1 ) {
			RandomAccessIterator median = begin + (n / 2);
			auto comp = [ dim ]( auto lhs, auto rhs ) { return *(lhs.begin() + dim) < *(rhs.begin() + dim); };
			std::nth_element( begin, median, end, comp );
			make_kdtree_helper( begin, median, depth + 1 );
			make_kdtree_helper( median + 1, end, depth + 1 );
		}
	}

	template <class RandomAccessIterator>
	void print_kdtree_node_helper( std::ostream & os, RandomAccessIterator median, depth_type depth, std::size_t node_count ) {
		using coordinate_type = decltype( *(median->cbegin()) );
		os << "(";
		std::copy( median->cbegin(), median->cend() - 1, std::ostream_iterator<coordinate_type>( os, "," ) );
		os << *(median->cend() - 1) << ") [d=" << depth << ",n=" << node_count << "]";
	}

	template <class RandomAccessIterator>
	void print_kdtree_helper( std::ostream & os, RandomAccessIterator begin, RandomAccessIterator end, depth_type depth ) {
		std::size_t n = end - begin;
		if( n > 0 ) {
			RandomAccessIterator median = begin + (n / 2);
			std::fill_n( std::ostream_iterator<std::string>( os ), depth, " | " );
			print_kdtree_node_helper( os, median, depth, n );
			os << "\n";
			print_kdtree_helper( os, begin, median, depth + 1 );
			print_kdtree_helper( os, median + 1, end, depth + 1 );
		}
	}

	template <class RandomAccessIterator, class Point>
	void nnsearch_kdtree_helper( RandomAccessIterator begin, RandomAccessIterator end, Point const & point, depth_type depth, distance_type & mindist, RandomAccessIterator & closest ) {
		dimension_type dim = dimension( Point::dimensionality(), depth );
		std::size_t n = end - begin;
		if( n > 0 ) {
			RandomAccessIterator median = begin + (n / 2);
//			print_kdtree_node_helper( std::cerr, median, depth, n );
			if( n > 1 ) {
				if( point[ dim ] <= (*median)[ dim ] ) {
//					std::cerr << " - heading left\n";
					nnsearch_kdtree_helper( begin, median, point, depth + 1, mindist, closest );
					if( point[ dim ] + mindist >= (*median)[ dim ] ) {
						update_minimum_distance( median, point, mindist, closest );
//						print_kdtree_node_helper( std::cerr, median, depth, n );
//						std::cerr << " - mindist: " << mindist << " - heading right\n";
						nnsearch_kdtree_helper( median + 1, end, point, depth + 1, mindist, closest );
					}
//					print_kdtree_node_helper( std::cerr, median, depth, n );
//					std::cerr << " - mindist: " << mindist << " - heading up\n";
				} else {
//					std::cerr << " - heading right\n";
					nnsearch_kdtree_helper( median + 1, end, point, depth + 1, mindist, closest );
					if( point[ dim ] - mindist <= (*median)[ dim ] ) {
						update_minimum_distance( median, point, mindist, closest );
//						print_kdtree_node_helper( std::cerr, median, depth, n );
//						std::cerr << " - mindist: " << mindist << " - heading left\n";
						nnsearch_kdtree_helper( begin, median, point, depth + 1, mindist, closest );
					}
//					print_kdtree_node_helper( std::cerr, median, depth, n );
//					std::cerr << " - mindist: " << mindist << " - heading up\n";
				}
			} else if( n == 1 ) {
				update_minimum_distance( median, point, mindist, closest );
//				std::cerr << " - mindist: " << mindist << " - heading up\n";
			}
		}
	}

	template <class RandomAccessIterator, class Point, class PriorityQueue>
	void nnsearch_kdtree_helper( RandomAccessIterator begin, RandomAccessIterator end, Point const & point, std::size_t k, depth_type depth, PriorityQueue & pq ) {
		dimension_type dim = dimension( Point::dimensionality(), depth );
		std::size_t n = end - begin;
		if( n > 0 ) {
			RandomAccessIterator median = begin + (n / 2);
//			print_kdtree_node_helper( std::cerr, median, depth, n );
			if( n > 1 ) {
				if( point[ dim ] <= (*median)[ dim ] ) {
//					std::cerr << " - heading left\n";
					nnsearch_kdtree_helper( begin, median, point, k, depth + 1, pq );
					if( point[ dim ] + pq.top().first >= (*median)[ dim ] ) {
						update_priority_queue( median, point, pq, k );
//						print_kdtree_node_helper( std::cerr, median, depth, n );
//						std::cerr << " - pq.top().first: " << pq.top().first << " - heading right\n";
						nnsearch_kdtree_helper( median + 1, end, point, k, depth + 1, pq );
					}
//					print_kdtree_node_helper( std::cerr, median, depth, n );
//					std::cerr << " - pq.top().first: " << pq.top().first << " - heading up\n";
				} else {
//					std::cerr << " - heading right\n";
					nnsearch_kdtree_helper( median + 1, end, point, k, depth + 1, pq );
					if( point[ dim ] - pq.top().first <= (*median)[ dim ] ) {
						update_priority_queue( median, point, pq, k );
//						print_kdtree_node_helper( std::cerr, median, depth, n );
//						std::cerr << " - pq.top().first: " << pq.top().first << " - heading left\n";
						nnsearch_kdtree_helper( begin, median, point, k, depth + 1, pq );
					}
//					print_kdtree_node_helper( std::cerr, median, depth, n );
//					std::cerr << " - pq.top().first: " << pq.top().first << " - heading up\n";
				}
			} else if( n == 1 ) {
				update_priority_queue( median, point, pq, k );
//				std::cerr << " - pq.top().first: " << pq.top().first << " - heading up\n";
			}
		}
	}

	template <class RandomAccessIterator, class Point>
	void rangequery_kdtree_helper( RandomAccessIterator begin, RandomAccessIterator end, Point const & min, Point const & max, depth_type depth, std::vector<RandomAccessIterator> & locations ) {
		dimension_type dim = dimension( Point::dimensionality(), depth );
		std::size_t n = end - begin;
		if( n > 0 ) {
			RandomAccessIterator median = begin + (n / 2);
			bool left_oob = min[ dim ] > (*median)[ dim ];
			bool right_oob = max[ dim ] < (*median)[ dim ];
			if( !left_oob ) {
				rangequery_kdtree_helper( begin, median, min, max, depth + 1, locations );
			}
			if( !right_oob ) {
				rangequery_kdtree_helper( median + 1, end, min, max, depth + 1, locations );
			}
			if( !left_oob && !right_oob ) {
				if( hypercube_contains( min, max, *median ) ) {
					locations.push_back( median );
				}
			}
		}
	}
	
}

namespace kdtree {

	template <class RandomAccessIterator>
	void make_kdtree( RandomAccessIterator begin, RandomAccessIterator end ) {
		using iterator_tag = typename std::iterator_traits<RandomAccessIterator>::iterator_category;
		using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
		static_assert( std::is_convertible< iterator_tag, std::random_access_iterator_tag >::value, "kdtree::make_kdtree( RandomAccessIterator begin, RandomAccessIterator end ) only accepts random access iterators or raw pointers to an array.\n" );
		make_kdtree_helper( begin, end, 0 );
	}

	template <class RandomAccessIterator>
	void print_kdtree( std::ostream & os, RandomAccessIterator begin, RandomAccessIterator end ) {
		print_kdtree_helper( os, begin, end, 0 );
	}

	template <class RandomAccessIterator, class Point>
	RandomAccessIterator search_kdtree( RandomAccessIterator begin, RandomAccessIterator end, Point const & point ) {
		using iterator_tag = typename std::iterator_traits<RandomAccessIterator>::iterator_category;
		using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
		static_assert( std::is_convertible< iterator_tag, std::random_access_iterator_tag >::value, "kdtree::search_kdtree( RandomAccessIterator begin, RandomAccessIterator end, Point const & point ) only accepts random access iterators or raw pointers to an array.\n" );
		static_assert( std::is_convertible< Point, value_type >::value, "kdtree::search_kdtree( RandomAccessIterator begin, RandomAccessIterator end, Point point ) only accepts Point types that are convertible to the value_type of the passed RandomAccessIterators.\n" );
//		using point_iterator_tag = typename std::iterator_traits<Point>::iterator_category;
//		static_assert( std::is_convertible< point_iterator_tag, std::random_access_iterator_tag >::value, "kdtree::search_kdtree( RandomAccessIterator begin, RandomAccessIterator end, Point const & point ) only accepts Point types that offer random access iterators or raw pointers to an array.\n" );
		RandomAccessIterator it = nnsearch_kdtree( begin, end, point );
		return point == *it ? it : end;
	}

	template <class RandomAccessIterator, class Point>
	RandomAccessIterator nnsearch_kdtree( RandomAccessIterator begin, RandomAccessIterator end, Point const & point ) {
		using iterator_tag = typename std::iterator_traits<RandomAccessIterator>::iterator_category;
		using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
		static_assert( std::is_convertible< iterator_tag, std::random_access_iterator_tag >::value, "kdtree::nnsearch_kdtree( RandomAccessIterator begin, RandomAccessIterator end, Point const & point ) only accepts random access iterators or raw pointers to an array.\n" );
		static_assert( std::is_convertible< Point, value_type >::value, "kdtree::nnsearch_kdtree( RandomAccessIterator begin, RandomAccessIterator end, Point point ) only accepts Point types that are convertible to the value_type of the passed RandomAccessIterators.\n" );
		distance_type distance = std::numeric_limits<distance_type>::max();
		RandomAccessIterator location = end;
		nnsearch_kdtree_helper( begin, end, point, 0, distance, location );
		return location;
	}

	template <class RandomAccessIterator, class Point>
	std::vector<RandomAccessIterator> nnsearch_kdtree( RandomAccessIterator begin, RandomAccessIterator end, Point const & point, std::size_t k ) {
		using iterator_tag = typename std::iterator_traits<RandomAccessIterator>::iterator_category;
		using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
		static_assert( std::is_convertible< iterator_tag, std::random_access_iterator_tag >::value, "kdtree::nnsearch_kdtree( RandomAccessIterator begin, RandomAccessIterator end, Point const & point, std::size_t k ) only accepts random access iterators or raw pointers to an array.\n" );
		static_assert( std::is_convertible< Point, value_type >::value, "kdtree::nnsearch_kdtree( RandomAccessIterator begin, RandomAccessIterator end, Point point, std::size_t k ) only accepts Point types that are convertible to the value_type of the passed RandomAccessIterators.\n" );
		using pq_data_package = typename std::pair<distance_type,RandomAccessIterator>;
		auto pq_compare = []( pq_data_package const & lhs, pq_data_package const & rhs ) { return lhs.first < rhs.first; };
		using vector = std::vector<pq_data_package>;
		using pq_type = std::priority_queue< pq_data_package, vector, decltype(pq_compare) >;
		vector pq_storage;
		pq_storage.reserve( k );
		pq_data_package const * cheap_access = &pq_storage[0];
		pq_type pq( pq_compare, std::move( pq_storage ) );
		pq.emplace( std::numeric_limits<distance_type>::max(), end );
		nnsearch_kdtree_helper( begin, end, point, k, 0, pq );
//		std::cerr << "FINAL pq.size(): " << pq.size() << " - FINAL pq.top().first: " << pq.top().first << "\n";
//		std::cerr << "pq_storage.size(): " << pq_storage.size() << "\n";
		std::vector<RandomAccessIterator> result;
		result.reserve( pq.size() );
		std::transform( cheap_access, cheap_access + pq.size(), std::back_inserter( result ), []( auto const & val ) { return val.second; } );
		return result;
	}

	template <class RandomAccessIterator, class Point>
	std::vector<RandomAccessIterator> rangequery_kdtree( RandomAccessIterator begin, RandomAccessIterator end, Point const & min, Point const & max ) {
		std::vector<RandomAccessIterator> locations;
		rangequery_kdtree_helper( begin, end, min, max, 0, locations );
		return locations;
	}

	template <class RandomAccessIterator, class Point>
	std::vector<RandomAccessIterator> radiusquery_kdtree( RandomAccessIterator begin, RandomAccessIterator end, Point const & point, typename Point::coordinate_type radius ) {
		std::vector<RandomAccessIterator> locations;
		if( radius > 0 ) {
			Point min( point[0] - radius, point[1] - radius );
			Point max( point[0] + radius, point[1] + radius );
			std::vector<RandomAccessIterator> candidates = rangequery_kdtree( begin, end, min, max );
			auto squared_radius = radius*radius;
			std::copy_if( candidates.cbegin(), candidates.cend(), locations.begin(), [&point](auto const & p) { return squared_euclidean_distance( point, p ) < squared_radius; } );
		}
		return locations;
	}
		
}

#endif
