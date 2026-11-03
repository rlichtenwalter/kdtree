# Standalone project that compiles a consumer against an *installed* kdtree.
#
# Deliberately not add_subdirectory()'d from the main build: it must resolve
# kdtree through find_package() and the installed kdtreeConfig.cmake, never
# through the in-tree target. Configure it against a staged prefix:
#
#   cmake --install build --prefix "$PWD/stage"
#   cmake -S test/install-test -B build/install-test \
#         -DCMAKE_PREFIX_PATH="$PWD/stage"
#   cmake --build build/install-test
#   ./build/install-test/kdtree_install_consumer
#
# The `cmake-min` CI job runs exactly this sequence under the oldest CMake the
# project supports.

cmake_minimum_required(VERSION 3.24)

project(kdtree_install_test LANGUAGES CXX)

find_package(kdtree REQUIRED)

add_executable(kdtree_install_consumer consumer.cpp)
target_link_libraries(kdtree_install_consumer PRIVATE kdtree::kdtree)
target_compile_options(kdtree_install_consumer PRIVATE -Wall -Wextra -Werror -pedantic)
