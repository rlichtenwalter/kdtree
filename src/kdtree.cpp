#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <getopt.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <stack>
#include <string>
#include <utility>
#include <vector>
#include "../include/kdtree.hpp"
#include "../include/point.hpp"

std::string VERSION_STRING = "0.1 (beta)";

enum verbosity_level : char {
	QUIET = 0,
	WARNING = 1,
	INFO = 2,
	DEBUG = 3
};

enum message_type : char {
	STANDARD = 0,
	START = 1,
	FINISH = 2
};

char DELIMITER = '\t';
verbosity_level VERBOSITY = WARNING;

void short_usage( char const * program ) {
	std::cerr << "Usage: " << program << " [OPTION]... [FILE]                                     \n";
	std::cerr << "Try '" << program << " --help' for more information.                            \n";
}

void usage( char const * program ) {
	std::cerr << "Usage: " << program << " [OPTION]... [FILE]                                     \n";
	std::cerr << "Currently, generate k-d tree from input file of points in format (x1,x2,...xn). \n";
	std::cerr << "Accepts either standard input or reads from a file. Named pipes and process     \n";
	std::cerr << "substitution may also be used as the file argument.                             \n";
	std::cerr << "                                                                                \n";
	std::cerr << "  -t, --delimiter=CHAR      use CHAR for field separator                        \n";
	std::cerr << "                            defaults to TAB if not provided                     \n";
	std::cerr << "  -v, --verbosity=VALUE     one of {0,1,2,3,quiet,warning,info,debug};          \n";
	std::cerr << "                            defaults to 1=warning if not provided               \n";
	std::cerr << "  -h, --help                display this help and exit                          \n";
	std::cerr << "  -V, --version             output version information and exist                \n";
}

void log_message( char const * message, verbosity_level verbosity, message_type mtype ) {
	using time_type = std::chrono::time_point<std::chrono::high_resolution_clock>;
	static std::stack<time_type,std::list<time_type>> time_stack;
	if( VERBOSITY >= verbosity ) {
		if( mtype == STANDARD && time_stack.size() > 0 ) {
			std::cerr << '\n';
		}
		if( mtype == STANDARD || mtype == START ) {
			std::time_t time = std::time( nullptr );
			std::cerr << std::string( time_stack.size(), '\t' ) << std::put_time( std::localtime( &time ), "%Y-%m-%d %H:%M:%S" ) << " - " << message;
		}
		if( mtype == STANDARD ) {
			std::cerr << '\n';
		} else if( mtype == START ) {
			time_stack.emplace( std::chrono::high_resolution_clock::now() );
		} else if( mtype == FINISH ) {
			if( time_stack.size() > 0 ) {
				auto start_time = time_stack.top();
				std::chrono::duration<double> time_span = std::chrono::duration_cast< std::chrono::duration<double> >( std::chrono::high_resolution_clock::now() - start_time );
				time_stack.pop();
				std::cerr << std::string( time_stack.size(), '\t' ) << "DONE (" << time_span.count() << " seconds)\n";
			} else {
				throw std::logic_error( "attempted to log 'FINISH' message without first logging corresponding 'START' message" );
			}
		}
	}
}

int main( int argc, char* argv[] ) {
	// disable I/O sychronization for better I/O performance
	std::ios_base::sync_with_stdio( false );

	std::ifstream ifs;

	int c;
	int option_index = 0;
	while( true ) {
		static struct option long_options[] = {
				{ "delimiter", required_argument, 0, 't' },
				{ "verbosity", required_argument, 0, 'v' },
				{ "help", no_argument, 0, 'h' },
				{ "version", no_argument, 0, 'V' }
				};
		c = getopt_long( argc, argv, "t:v:whV", long_options, &option_index );
		if( c == -1 ) {
			break;
		}
		switch( c ) {
			case 't':
				if( strcmp( optarg, "\\t" ) == 0 ) {
					DELIMITER = '\t';
				} else if( strlen( optarg ) != 1 ) {
					std::cerr << argv[0] << ":  -t, --delimiter=CHAR  must be a single character\n";
					return 1;
				} else {
					DELIMITER = optarg[0];
				}
				break;
			case 'v':
				if( strcmp( optarg, "0" ) == 0 || strcmp( optarg, "quiet" ) == 0 ) {
					VERBOSITY = QUIET;
				} else if( strcmp( optarg, "1" ) == 0 || strcmp( optarg, "warning" ) == 0 ) {
					VERBOSITY = WARNING;
				} else if( strcmp( optarg, "1" ) == 0 || strcmp( optarg, "info" ) == 0 ) {
					VERBOSITY = INFO;
				} else if( strcmp( optarg, "2" ) == 0 || strcmp( optarg, "debug" ) == 0 ) {
					VERBOSITY = DEBUG;
				} else {
					std::cerr << argv[0] << ": " << "  -v, --verbosity=[VALUE]  one of {0,1,2,3,quiet,warning,info,debug}; defaults to 1=warning\n";
					short_usage( argv[0] );
					return 1;
				}
				break;
			case 'h':
				usage( argv[0] );
				return 0;
			case 'V':
				std::cout << "Improved mRMR by Ryan N. Lichtenwalter v" << VERSION_STRING << "\n";
				return 0;
			default:
				short_usage( argv[0] );
				return 1;
		}
	}
	if( optind < argc ) {
		if( optind == argc - 1 ) {
			ifs = std::ifstream( argv[optind] );
			log_message( (std::string( "FILE = " ) + std::string( argv[optind] )).c_str(), DEBUG, STANDARD );
		} else {
			std::cerr << argv[0] << ": " << "too many arguments\n";
			short_usage( argv[0] );
			return 1;
		}
	}

	using point = kdtree::point<double,2>;

	// read data
	std::vector< point > points;
	log_message( "Reading points...", INFO, START );
	point p;
	if( ifs.is_open() ) {
		log_message( "Reading from file...", DEBUG, STANDARD );
		while( ifs.good() ) {
			ifs >> p;
			points.push_back( p );
		}
	} else {
		log_message( "Reading from standard input...", DEBUG, STANDARD );
		while( std::cin.good() ) {
			std::cin >> p;
			points.push_back( p );
			std::cin >> std::ws;
		}
	}
	log_message( "DONE", INFO, FINISH );

	// generate k-d tree data structure
	log_message( "Constructing k-d tree...", INFO, START );
	kdtree::make_kdtree( points.begin(), points.end() );
	log_message( "DONE", INFO, FINISH );


	kdtree::print_kdtree( std::cout, points.begin(), points.end() );

	return 0;
}
