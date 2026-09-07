# The dependencies, each exposed as a namespaced target.

# pugixml: the .qft reader and writer. Fetched, pinned by tag.
include(FetchContent)
FetchContent_Declare(pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG        v1.14
)
FetchContent_MakeAvailable(pugixml)

# QCustomPlot draws the plots of the GUI. Vendored, built on demand.
add_subdirectory(qcustomplot EXCLUDE_FROM_ALL)

# The interval arithmetic, as the single target qftbx_interval: the chosen
# library plus the macro that selects the backend section of interval.h.
add_library(qftbx_interval INTERFACE)

if(QFTBX_INTERVAL_BACKEND STREQUAL "kv")
  # kv: header-only, vendored. KV_NOHWROUND selects the rounding back-end
  # that never changes the floating-point mode; its error-free product is
  # Dekker's splitting unless KV_USE_TPFMA asks for the fused multiply-add,
  # which halves the cost of a product on a machine that has the
  # instruction, so it goes with the native-architecture build (elsewhere
  # fma() would be a library call and slower than the splitting).
  add_library(kv INTERFACE)
  add_library(kv::kv ALIAS kv)
  target_include_directories(kv SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/kv")
  target_compile_definitions(kv INTERFACE KV_NOHWROUND)
  if(USE_NATIVE_ARCH)
    target_compile_definitions(kv INTERFACE KV_USE_TPFMA=1)
  endif()
  target_link_libraries(qftbx_interval INTERFACE kv::kv)
  target_compile_definitions(qftbx_interval INTERFACE QFTBX_INTERVAL_KV=1)
else()
  # C-XSC: fetched from the fork that builds with a current compiler, pinned
  # by tag, and built untouched as an external project with its own CMake.
  # Point FETCHCONTENT_SOURCE_DIR_CXSC at a checkout to build without
  # network access. The library switches the rounding mode around each
  # operation, so every target that compiles interval code needs
  # -frounding-math: see qftbx_interval_arithmetic().
  include(ExternalProject)
  include(GNUInstallDirs)

  FetchContent_Declare(cxsc
    GIT_REPOSITORY https://github.com/Isaac-Martinez-Forte/cxsc-cpp17.git
    GIT_TAG        v2.5.4-qftbx1
  )
  FetchContent_Populate(cxsc)

  set(CXSC_PREFIX "${CMAKE_BINARY_DIR}/3rd-party/cxsc")
  set(CXSC_LIBDIR "${CXSC_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  if(NOT WIN32)
    set(CXSC_LIB "${CXSC_LIBDIR}/libcxsc.so")
  else()
    set(CXSC_LIB "${CXSC_LIBDIR}/libcxsc.a")
  endif()

  ExternalProject_Add(cxsc_ep
    PREFIX "${CXSC_PREFIX}/ep"
    SOURCE_DIR "${cxsc_SOURCE_DIR}"
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

  target_link_libraries(qftbx_interval INTERFACE cxsc::cxsc)
  target_compile_definitions(qftbx_interval INTERFACE QFTBX_INTERVAL_CXSC=1)
endif()
