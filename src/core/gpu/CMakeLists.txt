# The optional CUDA kernels for templates and boundaries. Added only when
# USE_CUDA is on and a CUDA compiler was found; CUDA_AVAILABLE then reaches
# every consumer of the core.
add_library(qftbx_cuda STATIC)
qftbx_sources_here(qftbx_cuda)
target_include_directories(qftbx_cuda PUBLIC "${CMAKE_SOURCE_DIR}")
target_compile_definitions(qftbx_cuda PUBLIC CUDA_AVAILABLE=1)
set_target_properties(qftbx_cuda PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
