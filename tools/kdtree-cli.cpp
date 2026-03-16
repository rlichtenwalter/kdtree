#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <kdtree/kdtree.hpp>
#include <kdtree/point.hpp>
#include <stack>
#include <string>
#include <vector>

#ifndef KDTREE_VERSION
#define KDTREE_VERSION "unknown"
#endif

enum verbosity_level : char { QUIET = 0, WARNING = 1, INFO = 2, DEBUG = 3 };

enum message_type : char { STANDARD = 0, START = 1, FINISH = 2 };

static verbosity_level VERBOSITY = WARNING;

static void short_usage(char const *program) {
  std::cerr << "Usage: " << program << " [OPTION]... [FILE]\n";
  std::cerr << "Try '" << program << " --help' for more information.\n";
}

static void usage(char const *program) {
  std::cerr << "Usage: " << program << " [OPTION]... [FILE]\n";
  std::cerr << "Build a k-d tree from 2D points in format (x1,x2).\n";
  std::cerr << "Accepts either standard input or reads from a file. Named pipes and process\n";
  std::cerr << "substitution may also be used as the file argument.\n";
  std::cerr << "\n";
  std::cerr << "  -v, --verbosity=VALUE     one of {0,1,2,3,quiet,warning,info,debug};\n";
  std::cerr << "                            defaults to 1=warning if not provided\n";
  std::cerr << "  -h, --help                display this help and exit\n";
  std::cerr << "  -V, --version             output version information and exit\n";
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

int main(int argc, char *argv[]) {
  try {
    std::ios_base::sync_with_stdio(false);

    std::ifstream ifs;

    int c;
    int option_index = 0;
    while (true) {
      static struct option long_options[] = {{"verbosity", required_argument, nullptr, 'v'},
                                             {"help", no_argument, nullptr, 'h'},
                                             {"version", no_argument, nullptr, 'V'},
                                             {nullptr, 0, nullptr, 0}};
      c = getopt_long(argc, argv, "v:hV", long_options, &option_index);
      if (c == -1) {
        break;
      }
      switch (c) {
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

    using point = kdtree::point<double, 2>;

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

    // generate k-d tree data structure
    log_message("Constructing k-d tree...", INFO, START);
    kdtree::make_kdtree(points.begin(), points.end());
    log_message("DONE", INFO, FINISH);

    kdtree::print_kdtree(std::cout, points.begin(), points.end());

    return 0;
  } catch (std::exception const &e) {
    std::cerr << argv[0] << ": fatal error: " << e.what() << "\n";
    return 1;
  }
}
