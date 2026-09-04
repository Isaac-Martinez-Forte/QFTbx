cmake_minimum_required(VERSION 3.17)

project(QFTbx LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(QftbxFunctions)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(USE_OpenMP        "Use OpenMP"           ON)
option(USE_CUDA          "Use CUDA"             OFF)
option(USE_NATIVE_ARCH   "Enable -march=native" ON)
option(QFTBX_BUILD_TESTS "Build unit tests"     ON)

# Development builds with sanitizers, e.g. -DQFTBX_SANITIZERS=address,undefined
# (needs the libasan/libubsan runtimes installed).
set(QFTBX_SANITIZERS "" CACHE STRING "Comma-separated sanitizers for dev builds (e.g. address,undefined)")
if(QFTBX_SANITIZERS)
  add_compile_options(-fsanitize=${QFTBX_SANITIZERS} -fno-omit-frame-pointer)
  add_link_options(-fsanitize=${QFTBX_SANITIZERS})
endif()

if(WIN32)
  set(CMAKE_PREFIX_PATH "C:\\Qt\\6.6.2\\mingw_64")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets PrintSupport LinguistTools)
message(STATUS "Found Qt ${Qt6_VERSION}")

if(USE_OpenMP)
  find_package(OpenMP)
endif()

# CUDA as a first-class CMake language; the kernels live in src/core/gpu.
if(USE_CUDA)
  include(CheckLanguage)
  check_language(CUDA)
  if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    message(STATUS "CUDA found: ${CMAKE_CUDA_COMPILER}")
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    # Sensible defaults; override with -DCMAKE_CUDA_ARCHITECTURES=... for
    # the card at hand.
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
      set(CMAKE_CUDA_ARCHITECTURES 61 75 86)
    endif()
  else()
    message(STATUS "CUDA not found")
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
  QFTbx_es_ES.ts
)
target_link_libraries(QFTbx PRIVATE qftbx_gui)
qftbx_warnings(QFTbx)
qftbx_native_arch(QFTbx)

if(WIN32)
  target_sources(QFTbx PRIVATE Resources/Icon/qftbx.rc)
endif()

if(UNIX AND NOT APPLE)
  qftbx_add_desktop_entry(QFTbx "${CMAKE_SOURCE_DIR}/Resources/Icon/qftbx_256.png")
endif()

qt6_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} QFTbx_es_ES.ts)

if(QFTBX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
