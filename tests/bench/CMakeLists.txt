# Tests of the benchmark library: plans, cases, structures, summaries and
# one measured run.
add_executable(qftbx_bench_tests)
qftbx_sources_here(qftbx_bench_tests)
target_link_libraries(qftbx_bench_tests PRIVATE qftbx_bench GTest::gtest)
target_compile_definitions(qftbx_bench_tests
  PRIVATE QFTBX_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data")
qftbx_warnings(qftbx_bench_tests)
qftbx_interval_arithmetic(qftbx_bench_tests)

include(GoogleTest)
gtest_discover_tests(qftbx_bench_tests DISCOVERY_TIMEOUT 60)
