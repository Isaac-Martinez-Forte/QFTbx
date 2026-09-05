# The backend suite talks to the facade, which drags the core and the
# persistence along, and to pugixml for its own smoke test. Qt6::Core here is
# test scaffolding only: QTemporaryDir for the files a couple of tests write.
# The core and the persistence link no Qt at all, and qftbx_core_noqt is what
# proves it.
add_executable(qftbx_tests)
qftbx_sources_here(qftbx_tests)
target_link_libraries(qftbx_tests PRIVATE qftbx_app GTest::gtest Qt6::Core pugixml::pugixml)

target_compile_definitions(qftbx_tests
  PRIVATE QFTBX_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data"
  PRIVATE QFTBX_EXAMPLE_CONFIG="${CMAKE_SOURCE_DIR}/qftbx.conf.example"
)

gtest_discover_tests(qftbx_tests DISCOVERY_TIMEOUT 60)
