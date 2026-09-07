# The planner's smoke tests join the GUI suite when the benchmark is built.
qftbx_sources_here(qftbx_gui_tests)
target_compile_definitions(qftbx_gui_tests PRIVATE QFTBX_BENCH_TOOL="$<TARGET_FILE:qftbx-bench>")
add_dependencies(qftbx_gui_tests qftbx-bench)
