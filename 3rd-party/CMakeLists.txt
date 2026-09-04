# The dependencies, each exposed as a namespaced target.

# pugixml: the .qft reader and writer. Fetched, pinned by tag.
include(FetchContent)
FetchContent_Declare(pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG        v1.14
)
FetchContent_MakeAvailable(pugixml)

# muParserX evaluates the free-form transfer-function expressions;
# QCustomPlot draws the plots of the GUI. Both vendored, built on demand.
add_subdirectory(muparserx EXCLUDE_FROM_ALL)
add_subdirectory(qcustomplot EXCLUDE_FROM_ALL)

# C-XSC: interval arithmetic for loop shaping. Built untouched as an
# external project and imported as cxsc::cxsc.
include(ExternalProject)
include(GNUInstallDirs)

set(CXSC_PREFIX "${CMAKE_BINARY_DIR}/3rd-party/cxsc")
set(CXSC_LIBDIR "${CXSC_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

if(NOT WIN32)
  set(CXSC_LIB "${CXSC_LIBDIR}/libcxsc.so")
else()
  set(CXSC_LIB "${CXSC_LIBDIR}/libcxsc.a")
endif()

ExternalProject_Add(cxsc_ep
  PREFIX "${CXSC_PREFIX}/ep"
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cxsc"
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=${CXSC_PREFIX}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
  BUILD_BYPRODUCTS "${CXSC_LIB}"
)

if(NOT WIN32)
  add_library(cxsc::cxsc SHARED IMPORTED GLOBAL)
else()
  add_library(cxsc::cxsc STATIC IMPORTED GLOBAL)
endif()

file(MAKE_DIRECTORY "${CXSC_PREFIX}/include")
file(MAKE_DIRECTORY "${CXSC_LIBDIR}")

set_target_properties(cxsc::cxsc PROPERTIES
  IMPORTED_LOCATION "${CXSC_LIB}"
  INTERFACE_INCLUDE_DIRECTORIES "${CXSC_PREFIX}/include"
)
add_dependencies(cxsc::cxsc cxsc_ep)
