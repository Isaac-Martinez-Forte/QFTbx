# The GPU engines, built when the configuration finds CUDA (QFTBX_CUDA, see
# USE_CUDA). A library of its own that depends on the core and not the other
# way round: the core declares the engines and the provider
# (src/core/pipeline/gpu_provider.h), this library implements them, and the
# application installs the provider at start. The .cu files take from the
# core only plain C++17 headers (the cancellation token, the expression
# program), so nvcc compiles them as C++17; the .cpp files are the glue
# with the core.
add_library(qftbx_cuda STATIC)
target_include_directories(qftbx_cuda PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(qftbx_cuda PUBLIC qftbx_core)
qftbx_warnings(qftbx_cuda)
qftbx_native_arch(qftbx_cuda)

add_subdirectory(common)
add_subdirectory(templates)
