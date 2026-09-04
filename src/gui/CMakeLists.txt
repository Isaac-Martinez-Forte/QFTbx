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
