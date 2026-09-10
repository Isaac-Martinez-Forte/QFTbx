# The loop-shaping module: the entry point and its result at this level,
# what every algorithm shares in common/, and one folder per algorithm.
qftbx_sources_here(qftbx_core)

add_subdirectory(common)
add_subdirectory(nt)
add_subdirectory(nk)
add_subdirectory(mr)
add_subdirectory(mc1)
add_subdirectory(mc_thesis)
add_subdirectory(mc2)
