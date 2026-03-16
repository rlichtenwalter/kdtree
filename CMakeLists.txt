cmake_minimum_required(VERSION 3.21)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_STRING)
string(STRIP "${PROJECT_VERSION_STRING}" PROJECT_VERSION_STRING)

project(kdtree
    VERSION ${PROJECT_VERSION_STRING}
    LANGUAGES CXX
    DESCRIPTION "Header-only C++ k-d tree library with STL container adaptor interface"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

# --- Library (header-only) ---
add_library(kdtree INTERFACE)
add_library(kdtree::kdtree ALIAS kdtree)

target_compile_features(kdtree INTERFACE cxx_std_14)

target_include_directories(kdtree INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources(kdtree INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/kdtree/kdtree.hpp
        include/kdtree/point.hpp
        include/kdtree/convex_polygon.hpp
        include/kdtree/point_in_polygon.hpp
)

# --- CLI tool ---
add_executable(kdtree-cli tools/kdtree-cli.cpp)
target_link_libraries(kdtree-cli PRIVATE kdtree::kdtree)
target_compile_definitions(kdtree-cli PRIVATE
    KDTREE_VERSION="${PROJECT_VERSION}"
)
set_target_properties(kdtree-cli PROPERTIES OUTPUT_NAME kdtree)

# --- Tests ---
option(KDTREE_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})

if(KDTREE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# --- Install ---
install(TARGETS kdtree
    EXPORT kdtreeTargets
    FILE_SET HEADERS
)

install(TARGETS kdtree-cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT kdtreeTargets
    FILE kdtreeTargets.cmake
    NAMESPACE kdtree::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kdtree
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/kdtreeConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/kdtreeConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kdtree
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/kdtreeConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/kdtreeConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/kdtreeConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kdtree
)

configure_file(
    cmake/kdtree.pc.in
    "${CMAKE_CURRENT_BINARY_DIR}/kdtree.pc"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/kdtree.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
