cmake_minimum_required(VERSION 3.17)

# The build options, all of them here. Each is passed as -D<option>=<value>
# and remembered by the build directory. CMakePresets.json names the usual
# combinations: native, the fastest build for the machine that compiles it and
# the one to use from the sources; reproducible, the same with the results of
# the published packages; and portable, the build of those packages. See
# docs/BUILDING.md.
#
# USE_NATIVE_ARCH tunes the numeric code for the machine that builds it
# (-march=native), the fastest, but the binary may not run on another one.
# USE_FP_CONTRACTION lets the compiler fuse a product and a sum into one
# instruction where the machine has it: a few per cent faster, with last bits
# that differ from a build without it; off, a native build gives exactly the
# results of the packages.
option(USE_NATIVE_ARCH    "Tune the code for the machine that builds it (-march=native)" ON)
option(USE_FP_CONTRACTION "Let the compiler fuse a product and a sum into one instruction" ON)
option(USE_OpenMP         "Use OpenMP" ON)
option(USE_CLANG          "Compile with Clang when available" OFF)
option(QFTBX_USE_SYSTEM_LIBS "Use installed pugixml, QCustomPlot and (if selected) C-XSC" OFF)

# USE_CUDA: AUTO builds the GPU engines when it finds CUDA 11.8 or later and
# leaves them out, saying why, when it does not; ON requires them and OFF
# leaves them out. 11.8 is what the engines use: the memory pools (11.2),
# libcu++'s atomic_ref (11.6) and the code for Ada and Hopper (11.8).
set(USE_CUDA AUTO CACHE STRING "GPU engines: AUTO when CUDA 11.8 or later is found, ON required, OFF never")
set_property(CACHE USE_CUDA PROPERTY STRINGS AUTO ON OFF)

# The tests, off unless asked for. They are the author's, not something a
# build for use needs: they fetch GoogleTest, which a build without network
# access cannot do, and they take longer to compile than the program. They do
# not change the program, which is built from the same sources either way.
option(QFTBX_BUILD_TESTS "Build unit tests" OFF)

# The benchmark tool and its planner in the interface: for measuring the
# algorithms, not for using them, so off unless asked for. See
# docs/BENCHMARKING.md.
option(QFTBX_BUILD_BENCHMARK "Build the benchmark tool and its planner" OFF)

# The interval arithmetic under the loop shaping: kv (vendored, header-only,
# the default) or cxsc (C-XSC, fetched from its repository at configure time
# and built as an external project). Both live behind src/core/math/interval.h
# and must give the same enclosures up to rounding.
set(QFTBX_INTERVAL_BACKEND "kv" CACHE STRING "Interval arithmetic library: kv or cxsc")
set_property(CACHE QFTBX_INTERVAL_BACKEND PROPERTY STRINGS kv cxsc)

# Development builds with sanitizers, e.g. -DQFTBX_SANITIZERS=address,undefined
# (needs the libasan/libubsan runtimes installed).
set(QFTBX_SANITIZERS "" CACHE STRING "Comma-separated sanitizers for dev builds (e.g. address,undefined)")

# Compile with Clang when it is installed and asked for. The compiler has to
# be chosen before project() and cannot change once a build tree exists, so
# this takes effect in a fresh build directory only; an explicit
# CMAKE_CXX_COMPILER on the command line always wins.
if(USE_CLANG AND NOT DEFINED CMAKE_CXX_COMPILER)
  find_program(QFTBX_CLANGXX NAMES clang++)
  find_program(QFTBX_CLANG NAMES clang)
  if(QFTBX_CLANGXX AND QFTBX_CLANG)
    set(CMAKE_CXX_COMPILER "${QFTBX_CLANGXX}" CACHE FILEPATH "C++ compiler")
    set(CMAKE_C_COMPILER "${QFTBX_CLANG}" CACHE FILEPATH "C compiler")
  else()
    message(STATUS "USE_CLANG is on but clang++ was not found: using the default compiler")
  endif()
endif()

# The version of the program, and the only place it is written. A zero major
# says the program does not commit to stability yet: the major changes when a
# published result moves or a project file of an older version stops opening,
# the minor when something new arrives that moves no result, and the patch
# with fixes that move nothing. The version of the .qft format is its own
# number and lives in src/persistence/qft_dialect.h.
project(QFTbx VERSION 0.1.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(QftbxFunctions)
include(GNUInstallDirs)

# Where the shipped examples sit once installed, and how to reach them from
# the executable, which is what the interface resolves at run time. Each
# system has its own habit: a bundle on macOS, everything beside the program
# on Windows, and the data directory of the prefix on the rest.
if(APPLE)
  set(QFTBX_EXAMPLES_INSTALL_DIR "QFTbx.app/Contents/Resources/examples")
  set(QFTBX_EXAMPLES_RELATIVE_DIR "../Resources/examples")
elseif(WIN32)
  set(QFTBX_EXAMPLES_INSTALL_DIR "examples")
  set(QFTBX_EXAMPLES_RELATIVE_DIR "examples")
else()
  set(QFTBX_EXAMPLES_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/qftbx/examples")
  set(QFTBX_EXAMPLES_RELATIVE_DIR "../${CMAKE_INSTALL_DATADIR}/qftbx/examples")
endif()

# Where the executable itself lands, which is where the relative path above
# starts from. The installation test walks the same two values.
# The header that carries the version into the About box, written at build
# time so the commit it names is the one being built.
set(QFTBX_VERSION_HEADER "${CMAKE_BINARY_DIR}/generated/qftbx_version.h")
add_custom_target(qftbx_version_header
  COMMAND "${CMAKE_COMMAND}"
    -D "SOURCE_DIR=${CMAKE_SOURCE_DIR}"
    -D "OUTPUT=${QFTBX_VERSION_HEADER}"
    -D "VERSION=${PROJECT_VERSION}"
    -P "${CMAKE_SOURCE_DIR}/cmake/WriteVersionHeader.cmake"
  BYPRODUCTS "${QFTBX_VERSION_HEADER}"
  COMMENT "Writing the version header"
  VERBATIM
)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")

if(APPLE)
  set(QFTBX_INSTALLED_EXE_DIR "QFTbx.app/Contents/MacOS")
elseif(WIN32)
  set(QFTBX_INSTALLED_EXE_DIR ".")
else()
  set(QFTBX_INSTALLED_EXE_DIR "${CMAKE_INSTALL_BINDIR}")
endif()

# Release unless the build asks for something else. A configure with no build
# type named gives no optimisation flags at all, which for a program whose
# work is a branch and bound is the difference between minutes and hours.
# Multi-configuration generators choose at build time and are left alone.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
               Debug Release RelWithDebInfo MinSizeRel)
  message(STATUS "No build type given: building Release")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# The commit the build is made from, for the benchmark records; "unknown"
# outside a repository.
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(COMMAND git rev-parse --short HEAD
                  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                  OUTPUT_VARIABLE QFTBX_GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE
                  ERROR_QUIET)
endif()
if(NOT QFTBX_GIT_COMMIT)
  set(QFTBX_GIT_COMMIT "unknown")
endif()

if(NOT QFTBX_INTERVAL_BACKEND MATCHES "^(kv|cxsc)$")
  message(FATAL_ERROR "QFTBX_INTERVAL_BACKEND must be kv or cxsc, not '${QFTBX_INTERVAL_BACKEND}'")
endif()

if(QFTBX_SANITIZERS)
  add_compile_options(-fsanitize=${QFTBX_SANITIZERS} -fno-omit-frame-pointer)
  add_link_options(-fsanitize=${QFTBX_SANITIZERS})
endif()

if(NOT USE_FP_CONTRACTION AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(
    "$<$<COMPILE_LANGUAGE:CXX>:-ffp-contract=off>"
    "$<$<COMPILE_LANGUAGE:CUDA>:--fmad=false>"
  )
endif()

# Where Qt lives on Windows when nothing says otherwise; pass
# -DCMAKE_PREFIX_PATH=<Qt dir> to point at another installation.
if(WIN32 AND NOT CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH "C:/Qt/6.6.2/mingw_64" CACHE PATH "Qt installation used on Windows")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets PrintSupport Svg LinguistTools)
message(STATUS "Found Qt ${Qt6_VERSION}")

if(USE_OpenMP)
  find_package(OpenMP)
endif()

# CUDA as a first-class CMake language; the GPU engines live in
# src/core/gpu, and QFTBX_CUDA says whether they are built. nvcc is looked
# for where CMake looks (CUDACXX, PATH, CUDA_PATH) and where Linux installs
# it, and its version is read before the language is enabled, since an
# enabled language cannot be taken back. By default every card the toolkit
# still supports gets its own code, from Pascal before CUDA 13 and from
# Turing since, and the newest architecture also a PTX the driver compiles
# for the cards that come after; -DCMAKE_CUDA_ARCHITECTURES=native builds
# for the machine's card only, which compiles faster.
set(QFTBX_CUDA OFF)
string(TOUPPER "${USE_CUDA}" qftbx_cuda_request)
if(NOT qftbx_cuda_request MATCHES "^(OFF|NO|FALSE|N|0|)$")
  set(qftbx_cuda_missing "")
  if(CMAKE_VERSION VERSION_LESS 3.18)
    set(qftbx_cuda_missing "it needs CMake 3.18 or later")
  else()
    if(NOT DEFINED CMAKE_CUDA_COMPILER AND "$ENV{CUDACXX}" STREQUAL "")
      find_program(QFTBX_NVCC nvcc PATHS /usr/local/cuda/bin /opt/cuda/bin)
      if(QFTBX_NVCC)
        set(CMAKE_CUDA_COMPILER "${QFTBX_NVCC}" CACHE FILEPATH "The CUDA compiler")
      endif()
    endif()
    include(CheckLanguage)
    check_language(CUDA)
    if(NOT CMAKE_CUDA_COMPILER)
      set(qftbx_cuda_missing "no CUDA compiler was found")
    else()
      execute_process(COMMAND "${CMAKE_CUDA_COMPILER}" --version
                      OUTPUT_VARIABLE qftbx_nvcc_banner ERROR_QUIET)
      string(REGEX MATCH "release ([0-9]+\\.[0-9]+)" qftbx_nvcc_release "${qftbx_nvcc_banner}")
      if(NOT CMAKE_MATCH_1 OR CMAKE_MATCH_1 VERSION_LESS 11.8)
        set(qftbx_cuda_missing "they need CUDA 11.8 or later, and ${CMAKE_CUDA_COMPILER} is ${CMAKE_MATCH_1}")
      endif()
    endif()
  endif()

  if(qftbx_cuda_missing STREQUAL "")
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES AND "$ENV{CUDAARCHS}" STREQUAL "")
      set(QFTBX_DEFAULT_CUDA_ARCHITECTURES TRUE)
    endif()
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    if(QFTBX_DEFAULT_CUDA_ARCHITECTURES)
      if(CMAKE_CUDA_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
        set(qftbx_cuda_architectures "75-real;80-real;86-real;89-real;90-real;100-real;120")
      elseif(CMAKE_CUDA_COMPILER_VERSION VERSION_GREATER_EQUAL 12.8)
        set(qftbx_cuda_architectures "61-real;70-real;75-real;80-real;86-real;89-real;90-real;100-real;120")
      else()
        set(qftbx_cuda_architectures "61-real;70-real;75-real;80-real;86-real;89-real;90")
      endif()
      set(CMAKE_CUDA_ARCHITECTURES "${qftbx_cuda_architectures}" CACHE STRING "CUDA architectures" FORCE)
      string(APPEND CMAKE_CUDA_FLAGS " -Wno-deprecated-gpu-targets")
    endif()
    set(QFTBX_CUDA ON)
    message(STATUS "GPU engines: CUDA ${CMAKE_CUDA_COMPILER_VERSION} for ${CMAKE_CUDA_ARCHITECTURES} (${CMAKE_CUDA_COMPILER})")
  elseif(qftbx_cuda_request STREQUAL "AUTO")
    message(STATUS "GPU engines left out: ${qftbx_cuda_missing}")
  else()
    message(FATAL_ERROR "USE_CUDA is ${USE_CUDA}, but the GPU engines cannot be built: ${qftbx_cuda_missing}")
  endif()
endif()

# The `docs` target, present only when doxygen is installed. It runs the
# Doxyfile at the root of the repository as it is, so `doxygen` from the
# root does exactly the same.
find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(DOXYGEN_FOUND)
  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating the API documentation into docs/api/html"
    VERBATIM
  )
  if(NOT TARGET Doxygen::dot)
    message(STATUS "Doxygen found, graphviz not: the docs will have no diagrams")
  endif()
else()
  message(STATUS "Doxygen not found: the `docs` target is unavailable")
endif()

add_subdirectory(3rd-party)
add_subdirectory(src)

# The application: main() over the GUI library.
add_executable(QFTbx
  main.cpp
)
target_link_libraries(QFTbx PRIVATE qftbx_gui)
qftbx_warnings(QFTbx)
qftbx_native_arch(QFTbx)

if(WIN32)
  target_sources(QFTbx PRIVATE Resources/Icon/qftbx.rc)
  set_target_properties(QFTbx PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(APPLE)
  set(QFTBX_MACOS_ICON "${CMAKE_SOURCE_DIR}/Resources/Icon/qftbx.icns")
  target_sources(QFTbx PRIVATE "${QFTBX_MACOS_ICON}")
  set_source_files_properties("${QFTBX_MACOS_ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  set_target_properties(QFTbx PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Resources/macos/Info.plist.in"
    MACOSX_BUNDLE_BUNDLE_NAME "QFTbx"
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.isaac-martinez-forte.qftbx"
    MACOSX_BUNDLE_ICON_FILE "qftbx.icns"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
  )
endif()

if(UNIX AND NOT APPLE)
  qftbx_add_desktop_entry(QFTbx "${CMAKE_SOURCE_DIR}/Resources/Icon/qftbx_256.png")
endif()

# Installation. The examples travel with the program and are never written
# to: the interface lists them, loads them and saves a copy wherever the
# user asks instead.
if(WIN32)
  install(TARGETS QFTbx RUNTIME DESTINATION .)
elseif(APPLE)
  install(TARGETS QFTbx BUNDLE DESTINATION .)
else()
  install(TARGETS QFTbx RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

install(DIRECTORY "${CMAKE_SOURCE_DIR}/examples/"
  DESTINATION "${QFTBX_EXAMPLES_INSTALL_DIR}"
  FILES_MATCHING
    PATTERN "*.qft"
    PATTERN "README.md"
)

if(UNIX AND NOT APPLE)
  install(FILES "${CMAKE_SOURCE_DIR}/Resources/qftbx.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
  )
  install(FILES "${CMAKE_SOURCE_DIR}/Resources/qftbx-mime.xml"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages"
    RENAME qftbx.xml
  )
  foreach(size 16 32 64 128 256 512)
    install(FILES "${CMAKE_SOURCE_DIR}/Resources/Icon/qftbx_${size}.png"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps"
      RENAME qftbx.png
    )
  endforeach()
endif()

if(QFTBX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
