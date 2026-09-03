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
  backend/boundary_bucket_bounds_test.cpp
  backend/mc_thesis_strategies_test.cpp
  backend/mr_article_validation_test.cpp
  backend/e_hull_test.cpp
  backend/templates_golden_test.cpp
  backend/templates_determinism_test.cpp
  backend/omega_test.cpp
  backend/plant_fixture_test.cpp
  backend/lti_system_test.cpp
  backend/evaluate_precision_test.cpp
  backend/expression_cache_test.cpp
  backend/parameter_test.cpp
  backend/specification_test.cpp
  backend/project_reader_load_test.cpp
  backend/project_reader_malformed_test.cpp
  backend/pugixml_smoke_test.cpp
  backend/project_roundtrip_test.cpp
  backend/controller_pipeline_test.cpp
  backend/project_staleness_test.cpp
  backend/computation_failure_test.cpp
  backend/loopshaping_structures_test.cpp
  backend/interval_extension_test.cpp
  backend/loopshaping_golden_test.cpp
  backend/thesis_benchmark_test.cpp
  backend/nominal_stability_test.cpp
  backend/literature_validation_test.cpp
  backend/quick_solution_test.cpp
  backend/cxsc_rigor_test.cpp
  backend/rounding_mode_test.cpp
  backend/cxsc_printing_test.cpp
)

#The suite talks to the facade (which drags core and persistence) and, for
#the pugixml smoke test, to pugixml itself: persistence keeps it private so
#a layering slip in the core does not compile.
# Qt6::Core here is TEST SCAFFOLDING only - QTemporaryDir for the files a
# couple of tests write. The core and the persistence link no Qt at all
# since phase 9.8, and qftbx_core_noqt is what proves it: the same sources
# compiled with the Qt include paths taken away.
target_link_libraries(qftbx_tests PRIVATE qftbx_app GTest::gtest Qt6::Core)
target_link_libraries(qftbx_tests PRIVATE pugixml::pugixml)

#See the -frounding-math rationale in the top-level CMakeLists: the tests
#compile the C-XSC inline interval operators too.
target_compile_options(qftbx_tests PRIVATE -frounding-math)

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

# ------------------------------------------------------------------------------
# GUI smoke suite: drives the dialogs headless (offscreen platform plugin).
# Separate binary because it needs a QApplication and the GUI library, which
# the backend suite must stay free of.
# ------------------------------------------------------------------------------
add_executable(qftbx_gui_tests
  gui/main.cpp
  gui/gui_smoke_test.cpp
)

target_link_libraries(qftbx_gui_tests PRIVATE qftbx_gui GTest::gtest)

#No -frounding-math here, unlike the backend suite: this binary runs no
#interval arithmetic of its own, and the flag breaks Qt's constexpr float
#code (qrgbafloat.h), which is exactly why the GUI target does not carry it
#either.

target_compile_definitions(qftbx_gui_tests
  PRIVATE QFTBX_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

if(UNIX AND NOT APPLE)
  set_target_properties(qftbx_gui_tests PROPERTIES BUILD_RPATH "${CXSC_LIBDIR}")
endif()

gtest_discover_tests(qftbx_gui_tests
  DISCOVERY_TIMEOUT 60
  PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
