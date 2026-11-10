# The GPU suite, built with the GPU engines. What needs a card skips without
# one; the rest checks what the program found on the machine and how it
# answers. The kernels the tests launch are in .cu files of their own.
add_executable(qftbx_gpu_tests)
qftbx_sources_here(qftbx_gpu_tests)
target_link_libraries(qftbx_gpu_tests PRIVATE qftbx_app GTest::gtest)
qftbx_warnings(qftbx_gpu_tests)

gtest_discover_tests(qftbx_gpu_tests DISCOVERY_TIMEOUT 60)
