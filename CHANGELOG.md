# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- `check-json` pre-commit hook (commit stage), validates `CMakePresets.json`
  and any future JSON files at commit time. Closes a small gap flagged by
  `/standards-check` (`precommit.check_json` warning).
- Sibling-alignment cleanup matching the conventions in `vcp` and the
  in-progress mRMR alignment:
  - **C++ standard bumped to C++20** (`cxx_std_14` → `cxx_std_20` in
    `CMakeLists.txt`). C++14 was already a valid subset of C++20, so no
    source changes were forced; the bump aligns kdtree's minimum
    standard with `vcp` and clears the way for opportunistic adoption
    of C++20 idioms at the next refactor.
  - Per-build-type compile flags applied to `kdtree-cli`, matching the
    `vcp` pattern: `$<$<CONFIG:Release>:-O3 -fomit-frame-pointer
    -DNDEBUG>` and `$<$<CONFIG:Debug>:-Og -g -fno-omit-frame-pointer>`.
    Previously `kdtree-cli` received only warning + sanitize flags and
    relied entirely on CMake's default `Release`/`Debug` flags, which
    omitted `-fomit-frame-pointer` on Release.
  - Link-time optimization on `kdtree-cli` Release builds via
    `INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE`. CMake handles the
    gcc-vs-clang toolchain difference (`gcc-ar`/`gcc-ranlib` vs
    `llvm-ar`/`llvm-ranlib`) automatically. Clang's bitcode-only LTO
    output requires `lld` for linking, so `add_link_options(-fuse-ld=lld)`
    is set when the compiler is Clang and the CI `build-and-test`
    install step adds `lld` alongside `clang`.
- `KDTREE_SANITIZE` CMake option that enables AddressSanitizer + UndefinedBehaviorSanitizer on every built target (CLI tool, tests, benchmark) in Debug builds. Includes `-fno-sanitize-recover=all` so every sanitizer diagnostic is a hard error. OFF by default; Release builds are never affected.
- New CI `sanitize` job that builds Debug with `KDTREE_SANITIZE=ON` and runs the full ctest suite on every PR.
- `CMakePresets.json` at the repository root with three named configurations
  (`release`, `debug`, `sanitize`) covering the meaningful build contexts
  the project ships. Each preset has its own `binaryDir` under `build/<name>`,
  so switching between configs no longer triggers a full rebuild — each tree
  keeps its own warm cache. Build presets and test presets mirror configure
  presets one-for-one; the `sanitize` test preset carries the
  `ASAN_OPTIONS` / `UBSAN_OPTIONS` halt-on-error contract that was
  previously duplicated inline in CI yaml. Preset file at version `3`
  (CMake 3.21+, well within the 3.24 floor); `cmakeMinimumRequired`
  declares 3.24 explicitly so older toolchains refuse to load it.
  IDEs that support presets (VSCode CMake Tools, CLion, KDevelop, Qt
  Creator) read the file directly. Schema string intentionally omitted:
  CMake errors on `$schema` below preset version 8, and version 8
  requires CMake 3.30 — outside our floor.

### Changed
- CI `build-and-test` job extended with a Clang matrix entry; both GCC and Clang now build
  the library, CLI, tests, and benchmark, and run the full ctest suite at Release and Debug.
  The library is header-only and implicitly promised Clang compatibility; the matrix makes
  that promise enforceable on every PR. Matrix is `{compiler: gcc, clang} × {build_type: Release, Debug}`
  with `fail-fast: false`.
- CLI, test, and benchmark targets now compile with shared warning flags
  (`-Wall -Wextra -Werror -pedantic -Wno-unused-local-typedefs`) via a new
  `KDTREE_WARNING_FLAGS` CMake variable. Previously CLI and tests received only
  `${KDTREE_SANITIZE_FLAGS}`, so warnings the production code should reject could slide
  through silently. Adding a flag to `KDTREE_WARNING_FLAGS` now lands in every consumer build at once.
- `KDTREE_WARNING_FLAGS` expanded with `-Wconversion -Wsign-conversion
  -Wshadow -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough`
  plus GCC-only `-Wlogical-op` and `-Wduplicated-cond`. Mechanical fallout
  fixed: six iterator-arithmetic conversion sites in `make_kdtree_helper`,
  `print_kdtree_helper`, the two `nnsearch_kdtree_helper` overloads,
  `rangequery_kdtree_helper`, and `radiusquery_kdtree_helper` now declare
  a local `using diff_t = ...iterator_traits...::difference_type` and
  cast at the `size_t`<->`difference_type` boundary instead of relying on
  implicit narrowing. The `std::hash<point>` specialization renamed its
  local accumulator from `hash` to `seed` to avoid shadowing the
  enclosing template specialization.
- Catch2's INTERFACE_INCLUDE_DIRECTORIES are now reassigned to
  INTERFACE_SYSTEM_INCLUDE_DIRECTORIES post-`FetchContent_MakeAvailable`,
  so warnings from Catch2's own headers (notably Clang's
  `-Wdouble-promotion` firing inside `catch_matchers_impl.hpp`'s
  float-vs-double comparison helpers) no longer break our `-Werror`
  builds. CMake 3.25 added a `SYSTEM` keyword to `FetchContent_Declare`
  that would do this declaratively; we still target 3.24 as the floor
  so the property reassignment is done manually.
- **BREAKING**: CMake minimum requirement raised from 3.21 to 3.24. CMake 3.24 introduced `cmake -B build --fresh`, a one-command cache clobber + reconfigure that eliminates the ad-hoc `rm -rf build/CMakeCache.txt` pattern. All current target distros ship CMake >= 3.24 in their default repositories (Rocky Linux 9 AppStream = 3.26.5, Rocky Linux 10 AppStream = 3.30.5, Ubuntu 24.04 LTS = 3.28.x), so the bump imposes no new constraint on contributors. Sibling C++ libraries (`vcp`, `mRMR`) receive the same bump in coordinated PRs.
- `.gitea/workflows/ci.yml` now invokes presets instead of inline
  `-DCMAKE_BUILD_TYPE=...` / `-DKDTREE_SANITIZE=ON` flags. The
  `build-and-test` matrix's `build_type: [Release, Debug]` becomes
  `preset: [release, debug]`, the `lint` job uses `cmake --preset=release`
  and `clang-tidy -p build/release`, and the `sanitize` job uses
  `cmake --preset=sanitize` with `ctest --preset=sanitize`. Sanitizer
  runtime options now live on the test preset, not the workflow yaml.
- `.gitignore` simplified: the `build-*/` glob is removed in favor of
  the existing `build/` rule, since presets place all per-config trees
  under `build/<name>/`.

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
