# The test suites (GoogleTest): one binary for the backend, one for the GUI.
include(FetchContent)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

add_subdirectory(backend)
add_subdirectory(gui)

# The benchmark library's own tests, with the tool.
if(QFTBX_BUILD_BENCHMARK)
  add_subdirectory(bench)
endif()

# The installed layout: a project that builds and passes its tests can still
# be packaged into something the program cannot read. This installs into a
# directory of the build tree and checks that the examples end up where the
# executable will look for them.
add_test(NAME Installation.TheExamplesEndUpWhereTheProgramLooks
  COMMAND "${CMAKE_COMMAND}"
    -D "BUILD_DIR=${CMAKE_BINARY_DIR}"
    -D "STAGE_DIR=${CMAKE_BINARY_DIR}/install-layout-test"
    -D "EXE_DIR=${QFTBX_INSTALLED_EXE_DIR}"
    -D "EXAMPLES_RELATIVE=${QFTBX_EXAMPLES_RELATIVE_DIR}"
    -D "SOURCE_EXAMPLES=${CMAKE_SOURCE_DIR}/examples"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/install_layout.cmake"
)
