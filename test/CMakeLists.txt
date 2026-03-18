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

# --- Benchmarks ---

add_executable(bench_kdtree bench_kdtree.cpp)
target_link_libraries(bench_kdtree PRIVATE kdtree::kdtree Catch2::Catch2WithMain)
target_compile_options(bench_kdtree PRIVATE -O3)

# --- CLI integration tests ---

set(CLI $<TARGET_FILE:kdtree-cli>)
set(TEST_DATA ${CMAKE_CURRENT_SOURCE_DIR}/data)

# --version and --help
add_test(NAME cli_version COMMAND ${CLI} --version)
set_tests_properties(cli_version PROPERTIES
    PASS_REGULAR_EXPRESSION "kdtree by Ryan N. Lichtenwalter v[0-9]+\\.[0-9]+\\.[0-9]+")

add_test(NAME cli_help COMMAND ${CLI} --help)
set_tests_properties(cli_help PROPERTIES
    PASS_REGULAR_EXPRESSION "Usage:.*\\[OPTION\\]")

# File input - single point
add_test(NAME cli_file_single_point COMMAND ${CLI} ${TEST_DATA}/single_point.txt)
set_tests_properties(cli_file_single_point PROPERTIES
    PASS_REGULAR_EXPRESSION "\\(5,10\\)")

# File input - three points (tree structure)
add_test(NAME cli_file_three_points COMMAND ${CLI} ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_file_three_points PROPERTIES
    PASS_REGULAR_EXPRESSION "\\[d=0,n=3\\]")

# Stdin input
add_test(NAME cli_stdin COMMAND sh -c "echo '(1,2)' | ${CLI}")
set_tests_properties(cli_stdin PROPERTIES
    PASS_REGULAR_EXPRESSION "\\(1,2\\).*\\[d=0,n=1\\]")

# Stdin multiple points
add_test(NAME cli_stdin_multiple
    COMMAND sh -c "printf '(1,3)\\n(2,7)\\n(-3,6)\\n' | ${CLI}")
set_tests_properties(cli_stdin_multiple PROPERTIES
    PASS_REGULAR_EXPRESSION "\\[d=0,n=3\\]")

# Error: nonexistent file
add_test(NAME cli_bad_file COMMAND ${CLI} /nonexistent/file)
set_tests_properties(cli_bad_file PROPERTIES
    WILL_FAIL TRUE)

# Error: bad input format
add_test(NAME cli_bad_format COMMAND ${CLI} ${TEST_DATA}/bad_format.txt)
set_tests_properties(cli_bad_format PROPERTIES
    WILL_FAIL TRUE)

# Error: too many arguments
add_test(NAME cli_too_many_args COMMAND ${CLI} file1 file2)
set_tests_properties(cli_too_many_args PROPERTIES
    WILL_FAIL TRUE)

# Error: invalid option
add_test(NAME cli_invalid_option COMMAND ${CLI} --bogus)
set_tests_properties(cli_invalid_option PROPERTIES
    WILL_FAIL TRUE)

# Error: invalid verbosity
add_test(NAME cli_bad_verbosity COMMAND ${CLI} -v 99)
set_tests_properties(cli_bad_verbosity PROPERTIES
    WILL_FAIL TRUE)

# Verbosity levels work (numeric and keyword)
add_test(NAME cli_verbosity_0 COMMAND sh -c "echo '(1,2)' | ${CLI} -v 0")
add_test(NAME cli_verbosity_1 COMMAND sh -c "echo '(1,2)' | ${CLI} -v 1")
add_test(NAME cli_verbosity_2 COMMAND sh -c "echo '(1,2)' | ${CLI} -v 2")
add_test(NAME cli_verbosity_3 COMMAND sh -c "echo '(1,2)' | ${CLI} -v 3")
add_test(NAME cli_verbosity_quiet COMMAND sh -c "echo '(1,2)' | ${CLI} -v quiet")
add_test(NAME cli_verbosity_warning COMMAND sh -c "echo '(1,2)' | ${CLI} -v warning")
add_test(NAME cli_verbosity_info COMMAND sh -c "echo '(1,2)' | ${CLI} -v info")
add_test(NAME cli_verbosity_debug COMMAND sh -c "echo '(1,2)' | ${CLI} -v debug")

# Empty input produces no tree output
add_test(NAME cli_empty_input COMMAND sh -c "echo -n '' | ${CLI}")

# File with no trailing newline
add_test(NAME cli_no_trailing_newline
    COMMAND sh -c "printf '(1,2)' | ${CLI}")
set_tests_properties(cli_no_trailing_newline PROPERTIES
    PASS_REGULAR_EXPRESSION "\\(1,2\\)")

# --- Query tests ---

# 1-NN query: nearest to (0,0) among (1,3),(2,7),(-3,6) is (1,3)
add_test(NAME cli_query_1nn
    COMMAND ${CLI} --query "(0,0)" ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_query_1nn PROPERTIES
    PASS_REGULAR_EXPRESSION "^\\(1,3\\) ")

# 1-NN query with -q short form
add_test(NAME cli_query_short_flag
    COMMAND ${CLI} -q "(0,0)" ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_query_short_flag PROPERTIES
    PASS_REGULAR_EXPRESSION "^\\(1,3\\) ")

# kNN query: k=2 nearest to (0,0)
add_test(NAME cli_query_knn
    COMMAND ${CLI} --query "(0,0)" --k 2 ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_query_knn PROPERTIES
    PASS_REGULAR_EXPRESSION "\\(1,3\\).*\\(-3,6\\)")

# Query suppresses tree output by default
add_test(NAME cli_query_no_tree
    COMMAND sh -c "${CLI} --query '(0,0)' ${TEST_DATA}/three_points.txt | grep -c '\\[d='")
set_tests_properties(cli_query_no_tree PROPERTIES
    WILL_FAIL TRUE)

# --print-tree restores tree output alongside query
add_test(NAME cli_query_print_tree
    COMMAND ${CLI} --query "(0,0)" --print-tree ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_query_print_tree PROPERTIES
    PASS_REGULAR_EXPRESSION "\\[d=0,n=3\\]")

# Multiple queries produce blank-line-separated blocks
add_test(NAME cli_query_multiple
    COMMAND sh -c "${CLI} --query '(0,0)' --query '(2,7)' ${TEST_DATA}/three_points.txt | grep -c '^$'")
set_tests_properties(cli_query_multiple PROPERTIES
    PASS_REGULAR_EXPRESSION "^1")

# Chebyshev metric
add_test(NAME cli_query_chebyshev
    COMMAND ${CLI} --query "(0,0)" --metric chebyshev ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_query_chebyshev PROPERTIES
    PASS_REGULAR_EXPRESSION "^\\(1,3\\) 3")

# Stdin with query
add_test(NAME cli_query_stdin
    COMMAND sh -c "printf '(1,3)\\n(2,7)\\n(-3,6)\\n' | ${CLI} --query '(0,0)'")
set_tests_properties(cli_query_stdin PROPERTIES
    PASS_REGULAR_EXPRESSION "^\\(1,3\\) ")

# Error: --query with no input
add_test(NAME cli_query_no_input
    COMMAND sh -c "echo -n '' | ${CLI} --query '(0,0)'")
set_tests_properties(cli_query_no_input PROPERTIES
    WILL_FAIL TRUE)

# Error: --k 0
add_test(NAME cli_query_k_zero
    COMMAND ${CLI} --k 0 --query "(0,0)" ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_query_k_zero PROPERTIES
    WILL_FAIL TRUE)

# Error: --k negative
add_test(NAME cli_query_k_negative
    COMMAND ${CLI} --k -1 --query "(0,0)" ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_query_k_negative PROPERTIES
    WILL_FAIL TRUE)

# Error: invalid metric name
add_test(NAME cli_query_bad_metric
    COMMAND ${CLI} --metric manhattan --query "(0,0)" ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_query_bad_metric PROPERTIES
    WILL_FAIL TRUE)

# Error: malformed query point (missing parens)
add_test(NAME cli_query_bad_format
    COMMAND ${CLI} --query "1,2" ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_query_bad_format PROPERTIES
    WILL_FAIL TRUE)

# Error: query point with trailing garbage
add_test(NAME cli_query_trailing_garbage
    COMMAND ${CLI} --query "(1,2)xyz" ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_query_trailing_garbage PROPERTIES
    WILL_FAIL TRUE)

# Warning: --metric without --query (tool succeeds but warns)
add_test(NAME cli_metric_without_query
    COMMAND ${CLI} --metric chebyshev ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_metric_without_query PROPERTIES
    PASS_REGULAR_EXPRESSION "\\[d=0,n=3\\]")

# k > n returns all points (no crash)
add_test(NAME cli_query_k_exceeds_n
    COMMAND ${CLI} --query "(0,0)" --k 100 ${TEST_DATA}/three_points.txt)
set_tests_properties(cli_query_k_exceeds_n PROPERTIES
    PASS_REGULAR_EXPRESSION "\\(1,3\\)")
