# The benchmark planner: the plan editor, the queue, the results and the
# window that holds them, over the benchmark library. Only with
# QFTBX_BUILD_BENCHMARK; the main window's Tools menu appears with it.
qftbx_sources_here(qftbx_gui)
target_link_libraries(qftbx_gui PUBLIC qftbx_bench)
target_compile_definitions(qftbx_gui PUBLIC QFTBX_BENCHMARK=1)
