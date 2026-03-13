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
- Bump minimum C++ standard from C++14 to C++17
- Update .gitignore for CMake build directory
- Update README with CMake build, test, and installation instructions

### Fixed
- CLI `--version` output incorrectly identified as "Improved mRMR"
- Nearest neighbor pruning compared squared distance as linear distance against splitting plane gap, causing incorrect results for close floating-point queries
- k-nearest neighbor pruning had the same squared-vs-linear distance bug
- k-nearest neighbor result extraction used a dangling pointer after std::move (undefined behavior)
- k-nearest neighbor sentinel iterator occupied a result slot, returning invalid end iterator and off-by-one count

## [1.0.0] - 2020-12-07

### Added
- K-d tree container adaptor with idiomatic C++ STL interface
- In-place tree construction via std::nth_element
- Exact search, nearest neighbor queries, range queries, and radius queries
- Templated point type as compositional facade over std::array
- Support for custom coordinate types and dynamically-sized containers
- Memory-efficient design with O(log n) stack overhead
