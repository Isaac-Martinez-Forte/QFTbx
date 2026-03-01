cmake_minimum_required(VERSION 3.17)
project(muparserx LANGUAGES CXX)

# Mantengo la extracción de versión como hacía tu CMake original
set(MUPARSERX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/parser)

file(READ "${MUPARSERX_SOURCE_DIR}/mpDefines.h" mpDefines_h)
string(REGEX MATCH "\\#define MUP_PARSER_VERSION _T\\(\"([0-9]+\\.[0-9]+\\.[0-9]+) \\("
       MUPARSERX_VERSION_MATCHES "${mpDefines_h}")

if(NOT MUPARSERX_VERSION_MATCHES)
  message(FATAL_ERROR "Failed to extract version number from mpDefines.h")
endif()

set(MUPARSERX_VERSION ${CMAKE_MATCH_1})

file(GLOB MUPARSERX_SOURCES "${MUPARSERX_SOURCE_DIR}/*.cpp")

add_library(muparserx ${MUPARSERX_SOURCES})
add_library(muparserx::muparserx ALIAS muparserx)

set_target_properties(muparserx PROPERTIES
  VERSION ${MUPARSERX_VERSION}
  SOVERSION ${MUPARSERX_VERSION}
  POSITION_INDEPENDENT_CODE TRUE
)

target_compile_features(muparserx PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(muparserx PRIVATE /MP)
  target_compile_options(muparserx PRIVATE /std:c++20)
endif()

if(WIN32)
  target_compile_definitions(muparserx PRIVATE _MSC_VER=1)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(muparserx PRIVATE -pedantic -Wall -Wextra)
endif()

target_include_directories(muparserx
  PUBLIC
    "${MUPARSERX_SOURCE_DIR}"
)

# link con libm si existe (igual que tu original, pero por target)
find_library(M_LIBRARY NAMES m PATHS /usr/lib /usr/lib64)
if(M_LIBRARY)
  target_link_libraries(muparserx PUBLIC ${M_LIBRARY})
endif()

# Opción wide string (igual que el original)
option(USE_WIDE_STRING "use UNICODE characters" OFF)
if(USE_WIDE_STRING)
  target_compile_definitions(muparserx PUBLIC MUP_USE_WIDE_STRING)
endif()

# Ejemplos (igual que el original, pero no se construyen por defecto)
option(BUILD_EXAMPLES "enable building example applications" OFF)
if(BUILD_EXAMPLES)
  add_executable(muparserx_example sample/example.cpp sample/timer.cpp)
  target_link_libraries(muparserx_example PRIVATE muparserx::muparserx)
endif()

message(STATUS "Building muparserx version: ${MUPARSERX_VERSION}")