# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Fixed
- Skip the `no-commit-to-branch` pre-commit hook in CI `pre-commit` steps: the hook guards local commits to `main`/`develop` and fired spuriously when CI checked out one of those branches, failing the job despite no real commit

## [2.0.1] - 2026-04-02

### Added
- Branch protection hook (no-commit-to-branch) for main and develop

### Changed
- Update clang-format to v22.1.2 for fleet-wide consistency

## [2.0.0] - 2026-03-18

### Added
- CMake install support with `find_package(kdtree)` for downstream consumers
- pkg-config support for non-CMake consumers
- CTest integration for test executables
- Comprehensive Catch2 v3 unit test suite covering point, kdtree, convex_polygon, and point_in_polygon
- Catch2 benchmark suite for construction and query operations across 2D-5D
- CLI integration tests covering all options, input modes, and error cases
- clang-format configuration (LLVM base style) for consistent code formatting
- clang-tidy configuration for static analysis
- CI lint job for format checking and static analysis
- Gitea Actions CI workflow for build and test on push/PR
- CLI version string sourced from VERSION file via CMake
- Output-iterator overloads for `rangequery_kdtree` and `radiusquery_kdtree` to avoid per-call allocation
- Bucket k-d tree with auto-tuned leaf threshold based on point size and cache line width
- Pluggable distance metric support for kd-tree search via `Metric` template parameter with `squared_euclidean_metric` (default) and `chebyshev_metric` policies
- `LeafThreshold` template parameter on all public API functions (default auto-selects)
- Chebyshev (L-infinity) distance function for max-norm nearest neighbor queries
- `detail::abs_diff` SFINAE helper for unsigned-safe absolute difference computation
- Mixed-type `squared_euclidean_distance` overload for points with different coordinate types
- CLI nearest-neighbor query support (`--query`, `--k`, `--metric`, `--print-tree`)
- Javadoc-style docstrings on all public API functions and classes

### Changed
- **BREAKING**: Move headers to `include/kdtree/` subdirectory (use `#include <kdtree/kdtree.hpp>`)
- **BREAKING**: Derive distance type from the metric policy rather than hardcoding `float`
- **BREAKING**: `radiusquery_kdtree` radius parameter type changed from `double` to `Point::coordinate_type`
- **BREAKING**: `nnsearch_kdtree` template parameters: `Metric` (type, default `squared_euclidean_metric`) is first, `LeafThreshold` (NTTP, default auto) is second
- kNN overload returns early for empty ranges or k=0
- Pin clang-format via pre-commit (`mirrors-clang-format` v20.1.8) and clang-tidy via pip (`clang-tidy` 20.1.0) for version consistency between development and CI
- Replace Makefile with CMake build system
- Minimum C++ standard is C++14 (unchanged, now declared via CMake)
- Move internal helpers from anonymous namespace to `kdtree::detail` namespace
- Replace `std::pow(x, 2)` with `x * x` in squared distance computation
- Replace two-pass radius query (range query + filter) with direct tree traversal
- Replace `std::priority_queue` with direct heap for O(k) kNN result extraction
- Improve point hash function from XOR to boost-style combiner
- Use compile-time constant for dimension cycling via `next_dimension<d>()`
- Pass construction comparator by const reference to avoid point copies
- Use index-based loop in `squared_euclidean_distance` for auto-vectorization
- Avoid point copies in polygon containment loops
- Replace unconstrained variadic constructor on `convex_polygon` with `std::initializer_list`
- Reorder public API declarations so `nnsearch_kdtree` precedes `search_kdtree`
- Move CLI tool from `src/` to `tools/` to follow header-only library conventions
- Rewrite CLI tool: fix verbosity parser, file error handling, input parsing, and help text

### Fixed
- Nearest neighbor pruning compared squared distance as linear distance against splitting plane gap, causing incorrect results for close floating-point queries
- k-nearest neighbor pruning had the same squared-vs-linear distance bug
- k-nearest neighbor result extraction used a dangling pointer after std::move (undefined behavior)
- k-nearest neighbor sentinel iterator occupied a result slot, returning invalid end iterator and off-by-one count
- `point::operator>>` error message reported `[` instead of `(` as expected character
- `convex_polygon` accepted 2-vertex input (minimum check ran after sentinel append)
- `convex_polygon` reverse iterators exposed internal sentinel and skipped first vertex
- Missing include guard on `point_in_polygon.hpp`
- Incorrect include guard prefix `HDTREE_` on `point.hpp` and `convex_polygon.hpp`
- Missing `#include "point.hpp"` in `point_in_polygon.hpp`
- `nnsearch_kdtree` returned undefined iterator for empty ranges
- `search_kdtree` dereferenced potentially invalid iterator from empty-range `nnsearch_kdtree`
- CLI `--version` output incorrectly identified as "Improved mRMR"
- CLI verbosity levels 2 and 3 were unreachable by numeric argument (copy-paste error)
- CLI silently fell back to stdin when specified file could not be opened
- CLI file-read loop pushed a garbage point after the last valid one on EOF
- CLI accepted undocumented `-w` option
- CLI `log_message` timer stack could become unbalanced across verbosity levels
- CLI help text typo "exist" instead of "exit"
- CLI `-t` delimiter option was parsed but never used (removed)
- `radiusquery_kdtree` subtree selection used signed gap comparison that silently wrapped for unsigned coordinate types
- Suppress clang-tidy checks incompatible with C++14 and LLVM 20

## [1.0.0] - 2020-12-07

### Added
- K-d tree container adaptor with idiomatic C++ STL interface
- In-place tree construction via std::nth_element
- Exact search, nearest neighbor queries, range queries, and radius queries
- Templated point type as compositional facade over std::array
- Support for custom coordinate types and dynamically-sized containers
- Memory-efficient design with O(log n) stack overhead
