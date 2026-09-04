# Layered libraries. The link order is the architecture, and the linker
# enforces it: the core knows nothing of files or widgets, the persistence
# knows nothing of the facade or the GUI, and the GUI reaches the core only
# through the facade.
#
#   qftbx_core  <-  qftbx_persistence  <-  qftbx_app  <-  qftbx_gui  <-  QFTbx
#
# Everything is static: the application runs from the build directory, and
# static linking keeps that free of library-path setup.
add_subdirectory(core)
add_subdirectory(persistence)
add_subdirectory(app)
add_subdirectory(gui)

# Does the core compile with no Qt at all? A syntax-only target over the
# same sources with the Qt include paths taken away answers with a yes or a
# no from the compiler. EXCLUDE_FROM_ALL: ask for it explicitly with
#
#     cmake --build build --target qftbx_core_noqt
get_target_property(core_sources qftbx_core SOURCES)
get_target_property(persistence_sources qftbx_persistence SOURCES)
add_library(qftbx_core_noqt OBJECT EXCLUDE_FROM_ALL ${core_sources} ${persistence_sources})
target_include_directories(qftbx_core_noqt PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(qftbx_core_noqt PRIVATE cxsc::cxsc pugixml::pugixml)
target_compile_options(qftbx_core_noqt PRIVATE -fsyntax-only)
set_target_properties(qftbx_core_noqt PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
