include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.13.0
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

add_executable(test_point test_point.cpp)
target_link_libraries(test_point PRIVATE kdtree::kdtree Catch2::Catch2WithMain)
catch_discover_tests(test_point)

add_executable(test_kdtree test_kdtree.cpp)
target_link_libraries(test_kdtree PRIVATE kdtree::kdtree Catch2::Catch2WithMain)
catch_discover_tests(test_kdtree)

add_executable(test_convex_polygon test_convex_polygon.cpp)
target_link_libraries(test_convex_polygon PRIVATE kdtree::kdtree Catch2::Catch2WithMain)
catch_discover_tests(test_convex_polygon)

add_executable(test_point_in_polygon test_point_in_polygon.cpp)
target_link_libraries(test_point_in_polygon PRIVATE kdtree::kdtree Catch2::Catch2WithMain)
catch_discover_tests(test_point_in_polygon)
