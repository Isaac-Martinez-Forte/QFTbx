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

# kv: the interval arithmetic of the loop shaping, header-only. KV_NOHWROUND
# selects the rounding back-end that never changes the floating-point mode;
# its error-free product is Dekker's splitting unless KV_USE_TPFMA asks for
# the fused multiply-add, which halves the cost of a product on a machine
# that has the instruction, so it goes with the native-architecture build
# (elsewhere fma() would be a library call and slower than the splitting).
add_library(kv INTERFACE)
add_library(kv::kv ALIAS kv)
target_include_directories(kv SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/kv")
target_compile_definitions(kv INTERFACE KV_NOHWROUND)
if(USE_NATIVE_ARCH)
  target_compile_definitions(kv INTERFACE KV_USE_TPFMA=1)
endif()
