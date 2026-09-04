# Functions shared by the CMakeLists.txt files of the project.

# qftbx_sources_here(<target>)
#
# Hands the target every source that sits in the calling directory: C++
# sources and headers, Qt Designer forms and CUDA sources. The directory is
# the unit of organisation, so a folder's CMakeLists.txt is this one call and
# nobody keeps a file list by hand. CONFIGURE_DEPENDS makes every build
# re-check the folder, so a new file joins the build without reconfiguring.
function(qftbx_sources_here target)
  file(GLOB files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cu"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cuh"
  )
  target_sources(${target} PRIVATE ${files})
endfunction()

# qftbx_warnings(<target>)
#
# The warning set every target of the project is built with.
function(qftbx_warnings target)
  target_compile_options(${target} PRIVATE -Wall -Wextra)
endfunction()

# qftbx_native_arch(<target>)
#
# Tunes the code for the machine that builds it, when USE_NATIVE_ARCH asks
# for it and the compiler understands the flag.
#
# On a machine with AVX-512 the compiler is asked to keep its vectors at
# 256 bits. Left to itself it reaches for the 512-bit registers, and on the
# Xeon this was measured on that made the numeric code several times
# slower rather than faster: the boundary sweep took sixteen times longer,
# and the constraint propagation of algorithm MR twice as long, for the
# same results. Whether it is the clock the wide registers cost or the
# transitions into the libraries built without them, the narrower vectors
# are the faster ones here, and the flag is harmless where AVX-512 is
# absent.
function(qftbx_native_arch target)
  if(USE_NATIVE_ARCH AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target} PRIVATE -march=native)

    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-mprefer-vector-width=256 QFTBX_HAS_PREFER_VECTOR_WIDTH)
    if(QFTBX_HAS_PREFER_VECTOR_WIDTH)
      target_compile_options(${target} PRIVATE -mprefer-vector-width=256)
    endif()
  endif()
endfunction()

# qftbx_interval_arithmetic(<target>)
#
# For every target that compiles the C-XSC headers. Their inline interval
# operators switch the FPU rounding mode around each endpoint; without
# -frounding-math the optimiser assumes round-to-nearest everywhere and
# reorders the arithmetic across the switches, producing intervals with the
# lower end above the upper one (seen live at -O3: the HC4 filter aborted on
# an empty interval raised by a plain subtraction). The whole interval
# arithmetic rests on this flag. It cannot be global: it demotes Qt's
# constexpr floating-point code, so it applies to the interval-computing
# targets only.
function(qftbx_interval_arithmetic target)
  target_compile_options(${target} PRIVATE -frounding-math)
endfunction()

# qftbx_add_desktop_entry(<target> <icon>)
#
# Writes a portable .desktop file next to the executable in the build tree,
# so the application can be launched from a desktop without installing it.
function(qftbx_add_desktop_entry target icon)
  set(desktop "${CMAKE_BINARY_DIR}/${target}.desktop")

  # An intermediate file, because file(GENERATE) is what expands the
  # generator expression holding the executable path.
  file(GENERATE
    OUTPUT "${desktop}.gen.in"
    CONTENT
"[Desktop Entry]\n\
Type=Application\n\
Version=1.0\n\
Name=${target}\n\
Comment=Quantitative Feedback Theory toolbox (robust control)\n\
Exec=$<TARGET_FILE:${target}>\n\
Path=${CMAKE_BINARY_DIR}\n\
Icon=${icon}\n\
Terminal=false\n\
Categories=Science;Engineering;\n\
StartupNotify=true\n"
  )

  add_custom_command(
    TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${desktop}.gen.in" "${desktop}"
    COMMAND chmod +x "${desktop}"
    COMMENT "Writing the desktop entry ${desktop}"
  )
endfunction()
