# The GUI smoke suite drives the dialogs headless (offscreen platform
# plugin). A separate binary because it needs a QApplication and the GUI
# library, which the backend suite must stay free of.
add_executable(qftbx_gui_tests)
qftbx_sources_here(qftbx_gui_tests)
target_link_libraries(qftbx_gui_tests PRIVATE qftbx_gui GTest::gtest)

target_compile_definitions(qftbx_gui_tests
  PRIVATE QFTBX_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data"
)

gtest_discover_tests(qftbx_gui_tests
  DISCOVERY_TIMEOUT 60
  PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
