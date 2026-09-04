# Project persistence (.qft): the only place that knows the file format.
# pugixml stays private so a layering slip in the core does not compile.
add_library(qftbx_persistence STATIC)
qftbx_sources_here(qftbx_persistence)
target_link_libraries(qftbx_persistence PUBLIC qftbx_core)
target_link_libraries(qftbx_persistence PRIVATE pugixml::pugixml)
qftbx_warnings(qftbx_persistence)
qftbx_native_arch(qftbx_persistence)
