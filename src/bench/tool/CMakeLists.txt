# qftbx-bench: the command-line benchmark tool over the library.
add_executable(qftbx-bench main.cpp)
# Next to the application, where the planner looks for it.
set_target_properties(qftbx-bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
target_link_libraries(qftbx-bench PRIVATE qftbx_bench)
qftbx_warnings(qftbx-bench)
