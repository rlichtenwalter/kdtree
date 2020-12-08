#ifndef HDTREE_POINT_HPP
#define HDTREE_POINT_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <cctype>
#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace kdtree {

	template <typename T, std::size_t d> class point {
		private:
			using storage_type = std::array<T,d>;
			using size_type = typename storage_type::size_type;
			storage_type _coordinates;
		public:
			using iterator = typename storage_type::iterator;
			using const_iterator = typename storage_type::const_iterator;
			using reverse_iterator = typename storage_type::reverse_iterator;
			using const_reverse_iterator = typename storage_type::const_reverse_iterator;
			using coordinate_type = T;

			point() = default;
			template <class...T2, typename std::enable_if<sizeof...(T2) == d, int>::type = 0> point( T2... args ) : _coordinates{ std::forward<T2>( args )... } {}
			static constexpr typename storage_type::size_type dimensionality() noexcept { return d; }

			constexpr coordinate_type & operator[]( size_type dimension ) { return _coordinates[ dimension ]; }
			constexpr coordinate_type const & operator[]( size_type dimension ) const { return _coordinates[ dimension ]; }

			bool operator==( point const & other ) const { return std::equal( this->begin(), this->end(), other.begin(), other.end() ); }			
			bool operator<( point const & other ) const { return std::lexicographical_compare( this->begin(), this->end(), other.begin(), other.end() ); }

			point operator+( point const & other ) const { point result; std::transform( this->begin(), this->end(), other.begin(), result.begin(), []( auto xi1, auto xi2 ) { return xi1 + xi2; } ); return result; }
			point operator-( point const & other ) const { point result; std::transform( this->begin(), this->end(), other.begin(), result.begin(), []( auto xi1, auto xi2 ) { return xi1 - xi2; } ); return result; }

			iterator begin() noexcept { return _coordinates.begin(); }
			const_iterator begin() const noexcept { return _coordinates.begin(); }
			const_iterator cbegin() const noexcept { return _coordinates.cbegin(); }
			iterator end() noexcept { return _coordinates.end(); }
			const_iterator end() const noexcept { return _coordinates.end(); }
			const_iterator cend() const noexcept { return _coordinates.cend(); }
			reverse_iterator rbegin() noexcept { return _coordinates.rbegin(); }
			const_reverse_iterator rbegin() const noexcept { return _coordinates.rbegin(); }
			const_reverse_iterator crbegin() const noexcept { return _coordinates.crbegin(); }
			reverse_iterator rend() noexcept { return _coordinates.rend(); }
			const_reverse_iterator rend() const noexcept { return _coordinates.rend(); }
			const_reverse_iterator crend() const noexcept { return _coordinates.crend(); }
	};

	template <class T, class U, std::size_t d>
	T squared_euclidean_distance( point<T,d> const & p1, point<U,d> const & p2 ) {
		/* AWAIT BETTER C++ 2020 SUPPORT
		auto sum_function = []( auto accum, auto element ) { return accum + element; };
		auto product_function = []( auto xi1, auto xi2 ) { return std::pow( xi1 - xi2, 2 ); };
		return std::inner_product( first1, last1, first2, static_cast<distance_type>( 0 ), sum_function, product_function )
		*/
		auto first1 = p1.cbegin();
		auto last1 = p1.cend();
		auto first2 = p2.cbegin();
		T dist = 0;
		while( first1 != last1 ) {
			dist += std::pow( *first1 - *first2, 2 );
			++first1;
			++first2;
		}
		return dist;
	}

	template <typename T, std::size_t d>
	std::ostream & operator<<( std::ostream & os, kdtree::point<T,d> const & p ) {
		os << '(';
		if( d > 0 ) {
			for( auto it = p.cbegin(); it != p.cend() - 1; ++it ) {
				os << *it << ',';
			}
			os << *(p.cend() - 1);
		}
		os << ')';
		return os;
	}

	template <typename T, std::size_t d>
	std::istream & operator>>( std::istream & is, kdtree::point<T,d> & p ) {
		auto generate_error_message = []( char c_expected, char c_given ) {
					std::stringstream error_ss( "invalid format for kdtree:point: expected '", std::ios_base::in | std::ios_base::out | std::ios_base::app );
					error_ss << c_expected << "' but saw '";
					if( std::isprint( c_given ) ) {
						error_ss << c_given << '\'';
					} else if( c_given == '\t' ) {
						error_ss << "\\t";
					} else if( c_given == '\n' ) {
						error_ss << "\\n";
					} else {
						error_ss << "\\" << std::hex << static_cast<unsigned int>( static_cast<unsigned char>( c_given ) );
					}
					error_ss << '\'';
					return error_ss.str();
				};
		char c;
		is >> c;
		if( c != '(' ) {
			throw std::range_error( generate_error_message( '[', c ) );
		}
		if( d > 0 ) {
			for( auto it = p.begin(); it != p.end() - 1; ++it ) {
				try {
					is >> *it;
				} catch( ... ) {
					throw std::range_error( "invalid format for kdtree:point: expected coordinate" );
				}
				is >> c;
				if( c != ',' ) {
					throw std::range_error( generate_error_message( ',', c ) );
				}
			}
			try {
				is >> *(p.end() - 1);
			} catch( ... ) {
				throw std::range_error( "invalid format for kdtree:point: expected coordinate" );
			}
		}
		is >> c;
		if( c != ')' ) {
			throw std::range_error( generate_error_message( ')', c ) );
		}
		return is;
	}
}

namespace std {
	template <typename T, std::size_t d> struct hash< kdtree::point<T,d> > {
		using argument_type = kdtree::point<T,d>;
		using result_type = std::size_t;
		result_type operator()( argument_type const & key ) const noexcept {
			result_type hash = 0;
			for( auto const xi : key ) {
				hash ^= std::hash<T>{}( xi );
			}
			return hash;
		}
	};
}

#endif
