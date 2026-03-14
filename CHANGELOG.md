# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- CMake install support with `find_package(kdtree)` for downstream consumers
- pkg-config support for non-CMake consumers
- CTest integration for test executables
- Comprehensive Catch2 v3 unit test suite covering point, kdtree, convex_polygon, and point_in_polygon
- clang-format configuration (LLVM base style) for consistent code formatting
- clang-tidy configuration for static analysis
- CI lint job for format checking and static analysis
- Gitea Actions CI workflow for build and test on push/PR
- CLI version string sourced from VERSION file via CMake

### Changed
- Replace Makefile with CMake build system
- Minimum C++ standard is C++14 (unchanged, now declared via CMake)
- Update .gitignore for CMake build directory
- Update README with CMake build, test, and installation instructions
- Move headers to `include/kdtree/` subdirectory for namespaced installation
- Move internal helpers from anonymous namespace to `kdtree::detail` namespace
- Derive distance type from point coordinate type instead of hardcoding `float`
- Replace `std::pow(x, 2)` with `x * x` in squared distance computation
- Replace two-pass radius query (range query + filter) with direct tree traversal
- Improve point hash function from XOR to boost-style combiner
- Reorder public API declarations so `nnsearch_kdtree` precedes `search_kdtree`
- Move CLI tool from `src/` to `tools/` to follow header-only library conventions
- Rewrite CLI tool: fix verbosity parser, file error handling, input parsing, and help text

### Fixed
- CLI `--version` output incorrectly identified as "Improved mRMR"
- Nearest neighbor pruning compared squared distance as linear distance against splitting plane gap, causing incorrect results for close floating-point queries
- k-nearest neighbor pruning had the same squared-vs-linear distance bug
- k-nearest neighbor result extraction used a dangling pointer after std::move (undefined behavior)
- k-nearest neighbor sentinel iterator occupied a result slot, returning invalid end iterator and off-by-one count
- Missing include guard on `point_in_polygon.hpp`
- Incorrect include guard prefix `HDTREE_` on `point.hpp` and `convex_polygon.hpp`
- Missing `#include "point.hpp"` in `point_in_polygon.hpp`
- `nnsearch_kdtree` returned undefined iterator for empty ranges
- `search_kdtree` dereferenced potentially invalid iterator from empty-range `nnsearch_kdtree`
- CLI verbosity levels 2 and 3 were unreachable by numeric argument (copy-paste error)
- CLI silently fell back to stdin when specified file could not be opened
- CLI file-read loop pushed a garbage point after the last valid one on EOF
- CLI accepted undocumented `-w` option
- CLI `log_message` timer stack could become unbalanced across verbosity levels
- CLI help text typo "exist" instead of "exit"
- CLI `-t` delimiter option was parsed but never used (removed)

## [1.0.0] - 2020-12-07

### Added
- K-d tree container adaptor with idiomatic C++ STL interface
- In-place tree construction via std::nth_element
- Exact search, nearest neighbor queries, range queries, and radius queries
- Templated point type as compositional facade over std::array
- Support for custom coordinate types and dynamically-sized containers
- Memory-efficient design with O(log n) stack overhead
