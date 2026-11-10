# The application facade: owns the project data and drives the engines and
# the persistence. It is what the GUI and the tests talk to.
add_library(qftbx_app STATIC)
qftbx_sources_here(qftbx_app)
target_link_libraries(qftbx_app PUBLIC qftbx_core qftbx_persistence)
qftbx_warnings(qftbx_app)
qftbx_native_arch(qftbx_app)

# A build with CUDA links the GPU engines and installs their provider at
# start (gpu_support.cpp).
if(QFTBX_CUDA)
  target_link_libraries(qftbx_app PUBLIC qftbx_cuda)
  target_compile_definitions(qftbx_app PRIVATE QFTBX_WITH_CUDA=1)
endif()
