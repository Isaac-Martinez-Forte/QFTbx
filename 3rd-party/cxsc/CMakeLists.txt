# CMakeLists.txt Configuration File, 2012-12-11
cmake_minimum_required(VERSION 3.17)

project(cxsc LANGUAGES C CXX)

include(GNUInstallDirs)

# ------------------------------------------------------------------------------
# Version
set(VER_MAJOR "2")
set(VER_MINOR "5")
set(VER_PATCH "4")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C-XSC and the C++ Toolbox for Verified Computation")
set(CPACK_PACKAGE_VENDOR "Universitaet Wuppertal, Germany")
set(CPACK_PACKAGE_VERSION_MAJOR "${VER_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VER_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VER_PATCH}")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/docu/COPYING")
include(CPack)

# ------------------------------------------------------------------------------
# C++ Standard (CXSC aquí pide C++17)
message("Check C++ version")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (CMAKE_CXX_STANDARD LESS 17)
  message(FATAL_ERROR "Se requiere C++ versión 17 o superior.")
endif()

# ------------------------------------------------------------------------------
# RPATH: evitar hardcode "lib" (Gentoo suele ser lib64)
# No nos saltamos el RPATH del build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)
# RPATH de instalación: <prefix>/<libdir>
if(UNIX AND NOT APPLE)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# ------------------------------------------------------------------------------
# Include dirs (mantenemos comportamiento; pero lo hacemos “target-friendly” luego)
set(CXSC_INTERNAL_INCLUDE_DIRS
  "${CMAKE_CURRENT_BINARY_DIR}"          # donde se genera cxscconf.h
  "${CMAKE_SOURCE_DIR}/CToolbox/Modules"
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/src/fi_lib"
  "${CMAKE_SOURCE_DIR}/src/rts"
  "${CMAKE_SOURCE_DIR}/src/asm"
)
include_directories(${CXSC_INTERNAL_INCLUDE_DIRS})

# ------------------------------------------------------------------------------
# Check for intel compiler
if ("${CMAKE_C_COMPILER}" MATCHES "icc")
  set(USING_INTEL_COMPILER TRUE)
endif()

# ------------------------------------------------------------------------------
# Rounding mode options
if(NOT WIN32)
  if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR
     ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86" OR
     ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ppc64")
    option(ROUNDING_ASM "Use assembler routines for rounding" ON)
    if(NOT ROUNDING_ASM)
      option(ROUNDING_HARD "Use hardware support for IEEE 759 arithmetic" ON)
      if(ROUNDING_HARD)
        option(ROUNDING_QUICK "Use the fast but critical hardware IEEE arithmetic" OFF)
      endif()
    endif()
  endif()
else()
  option(ROUNDING_HARD "Use hardware support for IEEE 759 arithmetic" ON)
  if(ROUNDING_HARD)
    option(ROUNDING_QUICK "Use the fast but critical hardware IEEE arithmetic" OFF)
  endif()
endif()

# Option for shared or static library (either/or)
if(NOT WIN32)
  if(APPLE)
    option(BUILD_SHARED "Build the shared version of the library" OFF)
  else()
    option(BUILD_SHARED "Build the shared version of the library" ON)
  endif()
endif()

# ------------------------------------------------------------------------------
# set the rounding options in cxscconf.h.
# The C96 options are only used in the combination "SunOS" with a GNU compiler.
if(NOT WIN32)
  if(ROUNDING_ASM)
    set(ROUND_ASM 1)
  else()
    if(ROUNDING_HARD)
      if(ROUNDING_QUICK)
        set(ROUND_C99_QUICK 1)
      else()
        set(ROUND_C99_SAVE 1)
      endif()
    endif()
  endif()
endif()

# ------------------------------------------------------------------------------
# System settings for the RTS (big/little endian, 64 Bit)
if(APPLE)
  if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "powerpc")
    set(SUN4_GNU_C 1)
  else()
    set(IBM_LINUX_C 1)
    set(GNU_X86_64 1)
    set(CXSC_x86_64 1)
  endif()
elseif(UNIX)
  if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "alpha")
      set(DEC_ALPHA_C 1)
    elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
      set(IBM_LINUX_C 1)
      set(GNU_X86_64 1)
      set(CXSC_x86_64 1)
    elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ppc64")
      set(CXSC_PPC64 1)
      set(GNU_X86_64 1)
    else()
      set(IBM_LINUX_C 1)
    endif()
  elseif(${CMAKE_SYSTEM_NAME} STREQUAL "AIX")
    set(IBM_RS_6000_C 1)
    set(GNU_X86_64 1)
  elseif(${CMAKE_SYSTEM_NAME} STREQUAL "AIX")
    set(SUN4_GNU_C 1)
  else()
    # default (e.g. cygwin)
    set(IBM_LINUX_C 1)
  endif()
elseif(MSVC)
  set(IBM_LINUX_C 1)
  set(WINDOWS_X86_32 1)
elseif(MINGW)
  set(IBM_LINUX_C 1)
  set(WINDOWS_MINGW 1)
  set(WINDOWS_X86_32 1)
else()
  set(IBM_LINUX_C 1)
endif()

# ------------------------------------------------------------------------------
# Create cxscconf.h file (will be created in the build directory)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cxscconf.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cxscconf.h"
)

# ------------------------------------------------------------------------------
# Add the list of files to compile (excluding example and test programs)
# (NO TOCAR: mantiene el layout original de CXSC)
include("${CMAKE_SOURCE_DIR}/src/CMakeLists.txt")
include("${CMAKE_SOURCE_DIR}/src/fi_lib/CMakeLists.txt")
include("${CMAKE_SOURCE_DIR}/src/rts/CMakeLists.txt")
include("${CMAKE_SOURCE_DIR}/CToolbox/Modules/CMakeLists.txt")

# ------------------------------------------------------------------------------
# set the variables for all header files
file(GLOB cxsc_headers
  "${CMAKE_SOURCE_DIR}/src/*.h"
  "${CMAKE_SOURCE_DIR}/src/*.hpp"
  "${CMAKE_SOURCE_DIR}/src/*.inl"
)
file(GLOB rts_headers     "${CMAKE_SOURCE_DIR}/src/rts/*.h")
file(GLOB fi_lib_headers  "${CMAKE_SOURCE_DIR}/src/fi_lib/*.hpp")
file(GLOB asm_headers     "${CMAKE_SOURCE_DIR}/src/asm/*.h")
file(GLOB ctoolbox_headers
  "${CMAKE_SOURCE_DIR}/CToolbox/Modules/*.hpp"
  "${CMAKE_SOURCE_DIR}/CToolbox/Modules/*.inl"
)

# ------------------------------------------------------------------------------
# Set special options depending on compiler (optimization, error free transformations)
# (Mantenemos tu lógica; solo corrigiendo CMAKE_CC_FLAGS -> CMAKE_C_FLAGS)
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86" AND CMAKE_COMPILER_IS_GNUCXX)
  if(ROUNDING_ASM)
    set(CMAKE_CXX_FLAGS "-O3 -mfpmath=sse -msse2")
    set(CMAKE_C_FLAGS   "-O3 -mfpmath=sse -msse2")
  else()
    set(CMAKE_CXX_FLAGS "-O1 -finline-functions -mfpmath=sse -msse2")
    set(CMAKE_C_FLAGS   "-O1 -finline-functions -mfpmath=sse -msse2")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCXX)
  if(ROUNDING_ASM)
    set(CMAKE_CXX_FLAGS "-O3")
    set(CMAKE_C_FLAGS   "-O3")
  else()
    set(CMAKE_CXX_FLAGS "-O1 -finline-functions")
    set(CMAKE_C_FLAGS   "-O1 -finline-functions")
  endif()
elseif(USING_INTEL_COMPILER)
  set(CMAKE_CXX_FLAGS "-O3 -fp-model source")
  set(CMAKE_C_FLAGS   "-O3 -fp-model source")
else()
  # Default compiler options
  set(CMAKE_CXX_FLAGS "-O1")
  set(CMAKE_C_FLAGS   "-O1")
endif()

# ------------------------------------------------------------------------------
# Add static or shared library to build
if(NOT WIN32)
  if(BUILD_SHARED)
    add_library(cxsc SHARED ${cxsc_src} ${rts_src} ${fi_lib_src} ${ctoolbox_src})
  else()
    add_library(cxsc STATIC ${cxsc_src} ${rts_src} ${fi_lib_src} ${ctoolbox_src})
  endif()
else()
  # On Windows only a static version of the library can be built.
  add_library(cxsc STATIC ${cxsc_src} ${rts_src} ${fi_lib_src} ${ctoolbox_src})
endif()

# So-version
if(NOT WIN32 AND BUILD_SHARED)
  set_target_properties(cxsc PROPERTIES
    VERSION   "${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}"
    SOVERSION "${VER_MAJOR}"
  )
endif()

# Includes (más robusto para consumidores)
target_include_directories(cxsc
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/rts>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/fi_lib>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/asm>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CToolbox/Modules>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ------------------------------------------------------------------------------
# Installation
install(
  FILES
    ${cxsc_headers}
    ${rts_headers}
    ${asm_headers}
    ${fi_lib_headers}
    ${ctoolbox_headers}
    "${CMAKE_CURRENT_BINARY_DIR}/cxscconf.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(FILES changelog README DESTINATION "${CMAKE_INSTALL_DOCDIR}")

install(TARGETS cxsc
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

# ------------------------------------------------------------------------------
# Source groups (IDE)
source_group("Source Files"           FILES ${cxsc_src})
source_group("Source Files\\rts"      FILES ${rts_src})
source_group("Source Files\\fi_lib"   FILES ${fi_lib_src})
source_group("Source Files\\CToolbox" FILES ${ctoolbox_src})
source_group("Header Files"           FILES ${cxsc_headers})
source_group("Header Files\\rts"      FILES ${rts_headers})
source_group("Header Files\\asm"      FILES ${asm_headers})
source_group("Header Files\\fi_lib"   FILES ${fi_lib_headers})
source_group("Header Files\\CToolbox" FILES ${ctoolbox_headers})

# ------------------------------------------------------------------------------
# CTests for Testing CXSC Library of correctness
set(CTOOLBOX_PROGRAMS_DIR "${CMAKE_CURRENT_BINARY_DIR}/CToolbox/test/test_programms")
set(CTOOLBOX_INPUT_DIR    "${CMAKE_SOURCE_DIR}/CToolbox/test/test_input")
set(CTOOLBOX_VLG_EXP_DIR  "${CMAKE_SOURCE_DIR}/CToolbox/test/test_vgl_exp")

enable_testing()

add_test(NAME cpz_test_1
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/cpz_test
          ${CTOOLBOX_INPUT_DIR}/cpz_ex.in1
          ${CTOOLBOX_VLG_EXP_DIR}/cpz_ex.vgl1
          ${CTOOLBOX_VLG_EXP_DIR}/cpz_ex.exp1
)
add_test(NAME cpz_test_2
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/cpz_test
          ${CTOOLBOX_INPUT_DIR}/cpz_ex.in2
          ${CTOOLBOX_VLG_EXP_DIR}/cpz_ex.vgl2
          ${CTOOLBOX_VLG_EXP_DIR}/cpz_ex.exp2
)
add_test(NAME gop1_test
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/gop1_test
          ${CTOOLBOX_INPUT_DIR}/gop1_ex.in
          ${CTOOLBOX_VLG_EXP_DIR}/gop1_ex.vgl
          ${CTOOLBOX_VLG_EXP_DIR}/gop1_ex.exp
)
add_test(NAME gop_test
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/gop_test
          ${CTOOLBOX_INPUT_DIR}/gop_ex.in
          ${CTOOLBOX_VLG_EXP_DIR}/gop_ex.vgl
          ${CTOOLBOX_VLG_EXP_DIR}/gop_ex.exp
)
add_test(NAME hess_test
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/hess_test
          ${CTOOLBOX_INPUT_DIR}/hess_ex.in
          ${CTOOLBOX_VLG_EXP_DIR}/hess_ex.vgl
          ${CTOOLBOX_VLG_EXP_DIR}/hess_ex.exp
)
add_test(NAME jac_test
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/jac_test
          ${CTOOLBOX_INPUT_DIR}/jac_ex.in
          ${CTOOLBOX_VLG_EXP_DIR}/jac_ex.vgl
          ${CTOOLBOX_VLG_EXP_DIR}/jac_ex.exp
)
add_test(NAME lop_test_1
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/lop_test
          ${CTOOLBOX_INPUT_DIR}/lop_ex.in1
          ${CTOOLBOX_VLG_EXP_DIR}/lop_ex.vgl1
          ${CTOOLBOX_VLG_EXP_DIR}/lop_ex.exp1
)
add_test(NAME lop_test_2
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/lop_test
          ${CTOOLBOX_INPUT_DIR}/lop_ex.in2
          ${CTOOLBOX_VLG_EXP_DIR}/lop_ex.vgl2
          ${CTOOLBOX_VLG_EXP_DIR}/lop_ex.exp2
)
add_test(NAME lss_test_1
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/lss_test
          ${CTOOLBOX_INPUT_DIR}/lss_ex.in1
          ${CTOOLBOX_VLG_EXP_DIR}/lss_ex.vgl1
          ${CTOOLBOX_VLG_EXP_DIR}/lss_ex.exp1
)
add_test(NAME lss_test_2
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/lss_test
          ${CTOOLBOX_INPUT_DIR}/lss_ex.in2
          ${CTOOLBOX_VLG_EXP_DIR}/lss_ex.vgl2
          ${CTOOLBOX_VLG_EXP_DIR}/lss_ex.exp2
)
add_test(NAME nlfz_test
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/nlfz_test
          ${CTOOLBOX_INPUT_DIR}/nlfz_ex.in
          ${CTOOLBOX_VLG_EXP_DIR}/nlfz_ex.vgl
          ${CTOOLBOX_VLG_EXP_DIR}/nlfz_ex.exp
)
add_test(NAME nlss_test_1
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/nlss_test
          ${CTOOLBOX_INPUT_DIR}/nlss_ex.in1
          ${CTOOLBOX_VLG_EXP_DIR}/nlss_ex.vgl1
          ${CTOOLBOX_VLG_EXP_DIR}/nlss_ex.exp1
)
add_test(NAME nlss_test_2
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/nlss_test
          ${CTOOLBOX_INPUT_DIR}/nlss_ex.in2
          ${CTOOLBOX_VLG_EXP_DIR}/nlss_ex.vgl2
          ${CTOOLBOX_VLG_EXP_DIR}/nlss_ex.exp2
)
add_test(NAME rpe_test_1
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/rpe_test
          ${CTOOLBOX_INPUT_DIR}/rpe_ex.in1
          ${CTOOLBOX_VLG_EXP_DIR}/rpe_ex.vgl1
          ${CTOOLBOX_VLG_EXP_DIR}/rpe_ex.exp1
)
add_test(NAME rpe_test_2
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/rpe_test
          ${CTOOLBOX_INPUT_DIR}/rpe_ex.in2
          ${CTOOLBOX_VLG_EXP_DIR}/rpe_ex.vgl2
          ${CTOOLBOX_VLG_EXP_DIR}/rpe_ex.exp2
)
add_test(NAME xev_test_1
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/xev_test1
          ${CTOOLBOX_INPUT_DIR}/xev_ex1.in
          ${CTOOLBOX_VLG_EXP_DIR}/xev_ex1.vgl
          ${CTOOLBOX_VLG_EXP_DIR}/xev_ex1.exp
)
add_test(NAME xev_test_2
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/xev_test2
          ${CTOOLBOX_INPUT_DIR}/xev_ex2.in
          ${CTOOLBOX_VLG_EXP_DIR}/xev_ex2.vgl
          ${CTOOLBOX_VLG_EXP_DIR}/xev_ex2.exp
)
add_test(NAME xev_test_3
  COMMAND ${CTOOLBOX_PROGRAMS_DIR}/xev_test3
          ${CTOOLBOX_INPUT_DIR}/xev_ex3.in
          ${CTOOLBOX_VLG_EXP_DIR}/xev_ex3.vgl
          ${CTOOLBOX_VLG_EXP_DIR}/xev_ex3.exp
)