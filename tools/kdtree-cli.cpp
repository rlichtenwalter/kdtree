#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <kdtree/kdtree.hpp>
#include <kdtree/point.hpp>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#ifndef KDTREE_VERSION
#define KDTREE_VERSION "unknown"
#endif

enum verbosity_level : char { QUIET = 0, WARNING = 1, INFO = 2, DEBUG = 3 };

enum message_type : char { STANDARD = 0, START = 1, FINISH = 2 };

enum metric_type : char { SQUARED_EUCLIDEAN = 0, CHEBYSHEV = 1 };

static verbosity_level VERBOSITY = WARNING;

static void short_usage(char const *program) {
  std::cerr << "Usage: " << program << " [OPTION]... [FILE]\n";
  std::cerr << "Try '" << program << " --help' for more information.\n";
}

static void usage(char const *program) {
  std::cout << "Usage: " << program << " [OPTION]... [FILE]\n";
  std::cout << "Build a k-d tree from 2D points in format (x1,x2) and optionally query it.\n";
  std::cout << "Accepts standard input or a file argument.\n";
  std::cout << "\n";
  std::cout << "  -q, --query POINT         find nearest neighbor(s) of POINT; may be repeated\n";
  std::cout << "      --k N                 number of nearest neighbors per query (default: 1)\n";
  std::cout << "      --metric NAME         squared-euclidean (default) or chebyshev\n";
  std::cout << "      --print-tree          print tree structure even when --query is given\n";
  std::cout << "  -v, --verbosity VALUE     one of {0,1,2,3,quiet,warning,info,debug};\n";
  std::cout << "                            defaults to 1=warning if not provided\n";
  std::cout << "  -h, --help                display this help and exit\n";
  std::cout << "  -V, --version             output version information and exit\n";
  std::cout << "\n";
  std::cout << "Query output: one result per line as \"(x,y) distance\", sorted nearest first.\n";
  std::cout << "Multiple --query flags produce result blocks separated by blank lines.\n";
}

static void log_message(char const *message, verbosity_level verbosity, message_type mtype) {
  using time_type = std::chrono::time_point<std::chrono::high_resolution_clock>;
  static std::stack<time_type> time_stack;

  if (mtype == START) {
    if (VERBOSITY >= verbosity) {
      std::time_t time = std::time(nullptr);
      std::cerr << std::string(time_stack.size(), '\t')
                << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << " - " << message;
    }
    time_stack.emplace(std::chrono::high_resolution_clock::now());
  } else if (mtype == FINISH) {
    if (time_stack.empty()) {
      throw std::logic_error("attempted to log 'FINISH' message without first logging "
                             "corresponding 'START' message");
    }
    auto start_time = time_stack.top();
    time_stack.pop();
    if (VERBOSITY >= verbosity) {
      std::chrono::duration<double> time_span =
          std::chrono::duration_cast<std::chrono::duration<double>>(
              std::chrono::high_resolution_clock::now() - start_time);
      std::cerr << std::string(time_stack.size(), '\t') << "DONE (" << time_span.count()
                << " seconds)\n";
    }
  } else {
    if (VERBOSITY >= verbosity) {
      if (!time_stack.empty()) {
        std::cerr << '\n';
      }
      std::time_t time = std::time(nullptr);
      std::cerr << std::string(time_stack.size(), '\t')
                << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << " - " << message
                << '\n';
    }
  }
}

using point = kdtree::point<double, 2>;

template <typename Metric>
static void run_query(std::vector<point> &points, point const &query, std::size_t k,
                      std::ostream &out) {
  if (k == 1) {
    auto it = kdtree::nnsearch_kdtree<Metric>(points.cbegin(), points.cend(), query);
    if (it != points.cend()) {
      auto dist = Metric::distance(*it, query);
      // For squared Euclidean, print true Euclidean distance
      double display_dist = std::is_same<Metric, kdtree::squared_euclidean_metric>::value
                                ? std::sqrt(static_cast<double>(dist))
                                : static_cast<double>(dist);
      out << *it << " " << display_dist << "\n";
    }
  } else {
    auto results = kdtree::nnsearch_kdtree<Metric>(points.cbegin(), points.cend(), query, k);
    // Sort by distance ascending
    std::vector<std::pair<double, point>> sorted;
    sorted.reserve(results.size());
    for (auto const &it : results) {
      auto dist = Metric::distance(*it, query);
      double display_dist = std::is_same<Metric, kdtree::squared_euclidean_metric>::value
                                ? std::sqrt(static_cast<double>(dist))
                                : static_cast<double>(dist);
      sorted.emplace_back(display_dist, *it);
    }
    std::sort(sorted.begin(), sorted.end());
    for (auto const &entry : sorted) {
      out << entry.second << " " << entry.first << "\n";
    }
  }
}

int main(int argc, char *argv[]) {
  try {
    std::ios_base::sync_with_stdio(false);

    std::ifstream ifs;
    std::vector<std::string> query_strings;
    std::size_t k = 1;
    metric_type metric = SQUARED_EUCLIDEAN;
    bool print_tree = false;

    // Long-only options use codes above the ASCII range
    enum { OPT_K = 256, OPT_METRIC, OPT_PRINT_TREE };

    int c;
    int option_index = 0;
    while (true) {
      static struct option long_options[] = {{"query", required_argument, nullptr, 'q'},
                                             {"k", required_argument, nullptr, OPT_K},
                                             {"metric", required_argument, nullptr, OPT_METRIC},
                                             {"print-tree", no_argument, nullptr, OPT_PRINT_TREE},
                                             {"verbosity", required_argument, nullptr, 'v'},
                                             {"help", no_argument, nullptr, 'h'},
                                             {"version", no_argument, nullptr, 'V'},
                                             {nullptr, 0, nullptr, 0}};
      c = getopt_long(argc, argv, "q:v:hV", long_options, &option_index);
      if (c == -1) {
        break;
      }
      switch (c) {
      case 'q':
        query_strings.emplace_back(optarg);
        break;
      case OPT_K: {
        char *endptr = nullptr;
        errno = 0;
        long val = std::strtol(optarg, &endptr, 10);
        if (endptr == optarg || *endptr != '\0' || val <= 0 || errno == ERANGE) {
          std::cerr << argv[0] << ": --k must be a positive integer\n";
          return 1;
        }
        k = static_cast<std::size_t>(val);
        break;
      }
      case OPT_METRIC:
        if (strcmp(optarg, "squared-euclidean") == 0) {
          metric = SQUARED_EUCLIDEAN;
        } else if (strcmp(optarg, "chebyshev") == 0) {
          metric = CHEBYSHEV;
        } else {
          std::cerr << argv[0] << ": --metric must be squared-euclidean or chebyshev\n";
          return 1;
        }
        break;
      case OPT_PRINT_TREE:
        print_tree = true;
        break;
      case 'v':
        if (strcmp(optarg, "0") == 0 || strcmp(optarg, "quiet") == 0) {
          VERBOSITY = QUIET;
        } else if (strcmp(optarg, "1") == 0 || strcmp(optarg, "warning") == 0) {
          VERBOSITY = WARNING;
        } else if (strcmp(optarg, "2") == 0 || strcmp(optarg, "info") == 0) {
          VERBOSITY = INFO;
        } else if (strcmp(optarg, "3") == 0 || strcmp(optarg, "debug") == 0) {
          VERBOSITY = DEBUG;
        } else {
          std::cerr << argv[0]
                    << ": -v, --verbosity=VALUE  one of {0,1,2,3,quiet,warning,info,debug}; "
                       "defaults to 1=warning\n";
          short_usage(argv[0]);
          return 1;
        }
        break;
      case 'h':
        usage(argv[0]);
        return 0;
      case 'V':
        std::cout << "kdtree by Ryan N. Lichtenwalter v" << KDTREE_VERSION << "\n";
        return 0;
      default:
        short_usage(argv[0]);
        return 1;
      }
    }
    if (optind < argc) {
      if (optind == argc - 1) {
        ifs = std::ifstream(argv[optind]);
        if (!ifs.is_open()) {
          std::cerr << argv[0] << ": " << argv[optind] << ": " << std::strerror(errno) << "\n";
          return 1;
        }
        log_message((std::string("FILE = ") + std::string(argv[optind])).c_str(), DEBUG, STANDARD);
      } else {
        std::cerr << argv[0] << ": too many arguments\n";
        short_usage(argv[0]);
        return 1;
      }
    }

    // Warn if --metric given without --query
    if (query_strings.empty() && metric != SQUARED_EUCLIDEAN) {
      log_message("--metric has no effect without --query", WARNING, STANDARD);
    }

    // Parse query points
    std::vector<point> queries;
    for (auto const &qs : query_strings) {
      std::istringstream iss(qs);
      point qp;
      try {
        iss >> qp;
      } catch (std::range_error const &e) {
        std::cerr << argv[0] << ": invalid query point '" << qs << "': " << e.what() << "\n";
        return 1;
      }
      if (iss.fail()) {
        std::cerr << argv[0] << ": invalid query point '" << qs << "'\n";
        return 1;
      }
      iss >> std::ws;
      if (!iss.eof()) {
        std::cerr << argv[0] << ": invalid query point '" << qs << "': trailing characters\n";
        return 1;
      }
      queries.push_back(qp);
    }

    std::vector<point> points;
    log_message("Reading points...", INFO, START);
    point p;
    auto &input =
        ifs.is_open() ? static_cast<std::istream &>(ifs) : static_cast<std::istream &>(std::cin);
    if (ifs.is_open()) {
      log_message("Reading from file...", DEBUG, STANDARD);
    } else {
      log_message("Reading from standard input...", DEBUG, STANDARD);
    }
    while (input >> std::ws && !input.eof()) {
      try {
        input >> p;
        points.push_back(p);
      } catch (std::range_error const &e) {
        std::cerr << argv[0] << ": parse error: " << e.what() << "\n";
        return 1;
      }
    }
    log_message("DONE", INFO, FINISH);

    if (!queries.empty() && points.empty()) {
      std::cerr << argv[0] << ": --query requires at least one input point\n";
      return 1;
    }

    // Build k-d tree
    log_message("Constructing k-d tree...", INFO, START);
    kdtree::make_kdtree(points.begin(), points.end());
    log_message("DONE", INFO, FINISH);

    // Print tree unless queries suppress it
    if (queries.empty() || print_tree) {
      kdtree::print_kdtree(std::cout, points.begin(), points.end());
    }

    // Execute queries
    if (!queries.empty()) {
      if (k > points.size()) {
        std::string msg = "--k=" + std::to_string(k) + " exceeds dataset size " +
                          std::to_string(points.size()) + "; returning all points";
        log_message(msg.c_str(), WARNING, STANDARD);
      }

      log_message("Running queries...", INFO, START);
      for (std::size_t i = 0; i < queries.size(); ++i) {
        if (i > 0) {
          std::cout << "\n";
        }
        if (metric == CHEBYSHEV) {
          run_query<kdtree::chebyshev_metric>(points, queries[i], k, std::cout);
        } else {
          run_query<kdtree::squared_euclidean_metric>(points, queries[i], k, std::cout);
        }
      }
      log_message("DONE", INFO, FINISH);
    }

    return 0;
  } catch (std::exception const &e) {
    std::cerr << argv[0] << ": fatal error: " << e.what() << "\n";
    return 1;
  }
}
