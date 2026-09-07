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

# The translations: one .ts per language under translations/ (see the README
# there), each compiled by lrelease at build time into a .qm that goes into
# the resources as :/i18n/qftbx_<code>.qm, where the View menu finds them. `cmake --build build --target update_translations` runs
# lupdate over this library's sources to bring the .ts up to date; nothing
# runs lupdate on its own, so a build never rewrites a file of the tree.
file(GLOB qftbx_translation_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/translations/*.ts")
qt_add_lrelease(qftbx_gui
  TS_FILES ${qftbx_translation_sources}
  QM_FILES_OUTPUT_VARIABLE qftbx_qm_files
)
qt_add_resources(qftbx_gui "translations"
  PREFIX "/i18n"
  BASE "${CMAKE_CURRENT_BINARY_DIR}"
  FILES ${qftbx_qm_files}
)
# The core's messages too: they are English texts marked with QFTBX_TR
# (src/core/common/message.h), which lupdate reads as QT_TRANSLATE_NOOP, and
# the interface translates them at its boundary (error_message.h).
get_target_property(qftbx_core_sources qftbx_core SOURCES)
get_target_property(qftbx_persistence_sources qftbx_persistence SOURCES)
get_target_property(qftbx_app_sources qftbx_app SOURCES)
get_target_property(qftbx_gui_sources qftbx_gui SOURCES)
qt_add_lupdate(qftbx_gui
  TS_FILES ${qftbx_translation_sources}
  SOURCES ${qftbx_gui_sources} ${qftbx_core_sources} ${qftbx_persistence_sources} ${qftbx_app_sources}
  OPTIONS -locations none -no-obsolete -tr-function-alias QT_TRANSLATE_NOOP+=QFTBX_TR
)
