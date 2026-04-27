# kdtree

There are many k-d tree implementations available, including many good implementations for C++. This particular implementation offers two primary advantages over many others.

First, it presents a container adaptor interface that is idiomatic of C++ STL and will be familiar to users of, for instance, the std::heap adaptor. It can operate over any data storage mechanism that provides iterators satisfying the RandomAccessIterator concept. It requires a mutable container and makes heavy use of std::nth_element to perform the bulk of the k-d tree construction effort in place. Likewise, via template parameters, it can operator on any underlying type that provides iterators satisfying the RandomAccessIterator concept to represent the k-dimensional space. For simplicity of usage, a point type is provided that is a compositional facade over std::array, thus offering contiguous storage requiring no additional dynamic allocations. For high-dimensional use cases, a container with dynamically allocated storage, such as std::vector, may allow for faster tree construction through less expensive point swaps.

Second, and relatedly, it is written to be extremely memory efficient and to enjoy efficiency gains from locality of reference and superior cache utilization. The underlying coordinate type is a template of the provided point type and allows for the selection of the most memory-efficient appropriate type. With respect to the minimal storage necessary to represent the points themselves, overhead during tree construction and search algorithm execution is limited to incidental automatic storage of primitive types, and the O(log(n)) stack depth necessary for the recursions, typically no more than a few KB of overhead for even extremely large data sets. Several potential algorithmic optimizations remain to be applied, but performance is nonetheless favorable compared to several tested implementations.

## Usage

```cpp
#include <kdtree/kdtree.hpp>
#include <kdtree/point.hpp>

std::vector<kdtree::point<double, 2>> points = { ... };

// Build the tree in-place
kdtree::make_kdtree(points.begin(), points.end());

// Nearest neighbor
auto it = kdtree::nnsearch_kdtree(points.cbegin(), points.cend(), query);

// k nearest neighbors
auto knn = kdtree::nnsearch_kdtree(points.cbegin(), points.cend(), query, 10);

// Range query
auto range = kdtree::rangequery_kdtree(points.cbegin(), points.cend(), lower, upper);

// Radius query
auto radius = kdtree::radiusquery_kdtree(points.cbegin(), points.cend(), center, 50.0);
```

All functions accept an optional `LeafThreshold` template parameter that controls
the bucket size for leaf nodes. The default auto-selects a value based on the
point type and cache line size. If you override it, the same value must be used
for construction and all subsequent queries:

```cpp
kdtree::make_kdtree<16>(points.begin(), points.end());
auto it = kdtree::nnsearch_kdtree<16>(points.cbegin(), points.cend(), query);
```

## Building

Requires CMake ≥ 3.24.

```bash
cmake -B build
cmake --build build
```

To force a fresh configure (drop the cached CMake state and reconfigure
from scratch — useful after changing the toolchain or build options):

```bash
cmake -B build --fresh
```

To build in debug mode:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

To build a sanitized Debug configuration (AddressSanitizer +
UndefinedBehaviorSanitizer on every target):

```bash
cmake -B build-san -DCMAKE_BUILD_TYPE=Debug -DKDTREE_SANITIZE=ON
cmake --build build-san
ASAN_OPTIONS=halt_on_error=1:detect_leaks=1:abort_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:abort_on_error=1:print_stacktrace=1 \
ctest --test-dir build-san --output-on-failure
```

The `sanitize` CI job runs this combination on every PR.
`-fno-sanitize-recover=all` makes every sanitizer diagnostic a hard
error; Release builds are never affected.

## Testing

```bash
ctest --test-dir build --output-on-failure
```

## Benchmarks

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run all benchmarks
./build/test/bench_kdtree "[!benchmark]"

# Run a specific category
./build/test/bench_kdtree "[!benchmark][construction]"
./build/test/bench_kdtree "[!benchmark][nn]"
./build/test/bench_kdtree "[!benchmark][knn]"
./build/test/bench_kdtree "[!benchmark][range]"
./build/test/bench_kdtree "[!benchmark][radius]"

# Increase sample count for more stable results
./build/test/bench_kdtree "[!benchmark]" --benchmark-samples 50
```

## Installation

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

After installation, downstream projects can use:

```cmake
find_package(kdtree REQUIRED)
target_link_libraries(your_target PRIVATE kdtree::kdtree)
```

Then include headers with the `kdtree/` prefix:

```cpp
#include <kdtree/kdtree.hpp>
#include <kdtree/point.hpp>
```

Or via pkg-config:

```bash
pkg-config --cflags kdtree
```

## License

BSD 3-Clause. See [LICENSE](LICENSE).
