# The Qt Widgets HMI: dialogs and viewers without main(), so the smoke suite
# can instantiate them headless. One folder per design step, plus the
# application shell and the pieces the dialogs share.
add_library(qftbx_gui STATIC)
target_include_directories(qftbx_gui PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(qftbx_gui PUBLIC qftbx_app Qt6::Widgets Qt6::PrintSupport qcustomplot::qcustomplot)
qftbx_warnings(qftbx_gui)

qt6_add_resources(resources
  "${CMAKE_SOURCE_DIR}/Resources/Images/imagenesPlanta.qrc"
  "${CMAKE_SOURCE_DIR}/Resources/Icon/icons.qrc"
)
target_sources(qftbx_gui PRIVATE ${resources})

# The Spanish translation: the .ts under translations/ is the source, lrelease
# compiles it into a .qm at build time and the .qm goes into the resources as
# :/i18n/qftbx_es.qm. `cmake --build build --target update_translations` runs
# lupdate over this library's sources to bring the .ts up to date; nothing
# runs lupdate on its own, so a build never rewrites a file of the tree.
qt_add_lrelease(qftbx_gui
  TS_FILES translations/qftbx_es.ts
  QM_FILES_OUTPUT_VARIABLE qftbx_qm_files
)
qt_add_resources(qftbx_gui "translations"
  PREFIX "/i18n"
  BASE "${CMAKE_CURRENT_BINARY_DIR}"
  FILES ${qftbx_qm_files}
)
qt_add_lupdate(qftbx_gui
  TS_FILES translations/qftbx_es.ts
  OPTIONS -locations none -no-obsolete
)

add_subdirectory(application)
add_subdirectory(common)
add_subdirectory(plant)
add_subdirectory(frequencies)
add_subdirectory(specifications)
add_subdirectory(templates)
add_subdirectory(boundaries)
add_subdirectory(loopshaping)
if(QFTBX_BUILD_BENCHMARK)
  add_subdirectory(bench)
endif()
