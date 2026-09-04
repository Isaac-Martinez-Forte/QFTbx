# The application facade: owns the project data and drives the engines and
# the persistence. It is what the GUI and the tests talk to.
add_library(qftbx_app STATIC)
qftbx_sources_here(qftbx_app)
target_link_libraries(qftbx_app PUBLIC qftbx_core qftbx_persistence)
qftbx_warnings(qftbx_app)
qftbx_native_arch(qftbx_app)
