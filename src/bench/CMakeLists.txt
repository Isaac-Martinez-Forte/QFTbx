# The benchmark library: plans, measured runs, the process runner and the
# summaries. Built only with QFTBX_BUILD_BENCHMARK; Qt Core for the
# processes and the JSON, no widgets.
add_library(qftbx_bench STATIC)
qftbx_sources_here(qftbx_bench)
target_include_directories(qftbx_bench PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(qftbx_bench PUBLIC qftbx_app pugixml::pugixml Qt6::Core)
target_compile_definitions(qftbx_bench PRIVATE QFTBX_GIT_COMMIT="${QFTBX_GIT_COMMIT}")
if(USE_NATIVE_ARCH)
  target_compile_definitions(qftbx_bench PRIVATE QFTBX_NATIVE_ARCH=1)
endif()
qftbx_warnings(qftbx_bench)
qftbx_native_arch(qftbx_bench)
qftbx_interval_arithmetic(qftbx_bench)

add_subdirectory(tool)
