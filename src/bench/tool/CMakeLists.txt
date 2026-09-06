# qftbx-bench: the command-line benchmark tool over the library.
add_executable(qftbx-bench main.cpp)
target_link_libraries(qftbx-bench PRIVATE qftbx_bench)
qftbx_warnings(qftbx-bench)
