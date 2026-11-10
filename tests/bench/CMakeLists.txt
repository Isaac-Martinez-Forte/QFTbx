# Tests of the benchmark library: plans, cases, structures, summaries, one
# measured run and the phases a plan computes before the search.
add_executable(qftbx_bench_tests)
qftbx_sources_here(qftbx_bench_tests)
target_link_libraries(qftbx_bench_tests PRIVATE qftbx_bench GTest::gtest)
target_compile_definitions(qftbx_bench_tests
  PRIVATE QFTBX_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data"
  PRIVATE QFTBX_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples")
qftbx_warnings(qftbx_bench_tests)
qftbx_interval_arithmetic(qftbx_bench_tests)

include(GoogleTest)
gtest_discover_tests(qftbx_bench_tests DISCOVERY_TIMEOUT 60)
