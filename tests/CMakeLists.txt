# ------------------------------------------------------------------------------
# Unit tests (GoogleTest)
# ------------------------------------------------------------------------------
include(FetchContent)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

add_executable(qftbx_tests
  main.cpp
  backend/boundaries_golden_test.cpp
  backend/e_hull_test.cpp
  backend/templates_golden_test.cpp
  backend/omega_test.cpp
  backend/plant_fixture_test.cpp
  backend/lti_system_test.cpp
  backend/parameter_test.cpp
  backend/specification_test.cpp
  backend/project_reader_load_test.cpp
  backend/pugixml_smoke_test.cpp
  backend/project_roundtrip_test.cpp
  backend/controller_pipeline_test.cpp
  backend/loopshaping_structures_test.cpp
  backend/interval_extension_test.cpp
  backend/loopshaping_golden_test.cpp
  backend/thesis_benchmark_test.cpp
  backend/nominal_stability_test.cpp
  backend/literature_validation_test.cpp
)

target_link_libraries(qftbx_tests PRIVATE qftbx_backend GTest::gtest)

target_compile_definitions(qftbx_tests
  PRIVATE QFTBX_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

# libcxsc.so se construye via ExternalProject; el RPATH del build tree no la
# recoge automaticamente.
if(UNIX AND NOT APPLE)
  set_target_properties(qftbx_tests PROPERTIES BUILD_RPATH "${CXSC_LIBDIR}")
endif()

include(GoogleTest)
gtest_discover_tests(qftbx_tests DISCOVERY_TIMEOUT 60)
