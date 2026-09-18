# The command-line solver: the seven phases without the interface. It links
# the facade the windows link and no Qt at all, which is what makes it the
# proof that the algorithms are separable from the interface.
add_executable(qftbx-solve)
qftbx_sources_here(qftbx-solve)
target_link_libraries(qftbx-solve PRIVATE qftbx_app)
qftbx_warnings(qftbx-solve)
qftbx_native_arch(qftbx-solve)
qftbx_interval_arithmetic(qftbx-solve)

# No widgets, no signals, no moc: the claim above is only worth something if
# the build enforces it.
set_target_properties(qftbx-solve PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
