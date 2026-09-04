# The computational core: plants, frequencies, specifications, templates,
# boundaries and the loop-shaping algorithms. No Qt, no files, no widgets.
# Each folder below adds its sources to this one library.
add_library(qftbx_core STATIC)
target_include_directories(qftbx_core PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(qftbx_core PUBLIC muparserx::muparserx cxsc::cxsc)
qftbx_warnings(qftbx_core)
qftbx_native_arch(qftbx_core)
qftbx_interval_arithmetic(qftbx_core)

if(USE_OpenMP AND OpenMP_CXX_FOUND)
  # PUBLIC: OpenMP_AVAILABLE changes declarations in shared headers, so it
  # must reach the consumers.
  target_link_libraries(qftbx_core PUBLIC OpenMP::OpenMP_CXX)
  target_compile_definitions(qftbx_core PUBLIC OpenMP_AVAILABLE=1)
elseif(USE_OpenMP)
  message(STATUS "OpenMP not found")
endif()

add_subdirectory(boundaries)
add_subdirectory(common)
add_subdirectory(frequencies)
add_subdirectory(loopshaping)
add_subdirectory(math)
add_subdirectory(pipeline)
add_subdirectory(project)
add_subdirectory(specifications)
add_subdirectory(system)
add_subdirectory(templates)

if(USE_CUDA AND CMAKE_CUDA_COMPILER)
  add_subdirectory(gpu)
  target_link_libraries(qftbx_core PUBLIC qftbx_cuda)
endif()
