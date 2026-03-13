add_executable(kdtree_test kdtree.cpp)
target_link_libraries(kdtree_test PRIVATE kdtree::kdtree)
add_test(NAME kdtree_test COMMAND kdtree_test)

add_executable(point_test point.cpp)
target_link_libraries(point_test PRIVATE kdtree::kdtree)
add_test(NAME point_test COMMAND point_test)

add_executable(convex_polygon_test convex_polygon.cpp)
target_link_libraries(convex_polygon_test PRIVATE kdtree::kdtree)
add_test(NAME convex_polygon_test COMMAND convex_polygon_test)

add_executable(point_in_polygon_test point_in_polygon.cpp)
target_link_libraries(point_in_polygon_test PRIVATE kdtree::kdtree)
add_test(NAME point_in_polygon_test COMMAND point_in_polygon_test)
