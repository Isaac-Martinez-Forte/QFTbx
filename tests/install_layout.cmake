# Installs the project into a directory of its own and checks that what comes
# out is the layout the program expects: the executable where the packaging
# puts it, the examples where the executable looks for them, and nothing from
# a dependency that has no business in a package of the application.
#
# Driven by ctest; every path it needs is passed in with -D, because a script
# knows nothing of the project that generated it.

if(NOT DEFINED BUILD_DIR OR NOT DEFINED STAGE_DIR OR NOT DEFINED EXE_DIR
   OR NOT DEFINED EXAMPLES_RELATIVE OR NOT DEFINED SOURCE_EXAMPLES)
  message(FATAL_ERROR "install_layout.cmake: a path was not given")
endif()

file(REMOVE_RECURSE "${STAGE_DIR}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${STAGE_DIR}"
  RESULT_VARIABLE installed
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output
)
if(NOT installed EQUAL 0)
  message(FATAL_ERROR "the install failed:\n${output}")
endif()

set(executable "${STAGE_DIR}/${EXE_DIR}/QFTbx")
if(WIN32)
  set(executable "${executable}.exe")
endif()
if(NOT EXISTS "${executable}")
  message(FATAL_ERROR "no executable at ${executable}")
endif()

get_filename_component(examples "${STAGE_DIR}/${EXE_DIR}/${EXAMPLES_RELATIVE}" ABSOLUTE)
if(NOT IS_DIRECTORY "${examples}")
  message(FATAL_ERROR
    "the executable resolves its examples to ${examples}, which is not a directory")
endif()

file(GLOB shipped "${examples}/*.qft")
file(GLOB expected "${SOURCE_EXAMPLES}/*.qft")
list(LENGTH shipped shippedCount)
list(LENGTH expected expectedCount)
if(NOT shippedCount EQUAL expectedCount)
  message(FATAL_ERROR
    "${expectedCount} examples in the tree and ${shippedCount} installed")
endif()
if(shippedCount EQUAL 0)
  message(FATAL_ERROR "no example was installed")
endif()

foreach(example IN LISTS expected)
  get_filename_component(name "${example}" NAME)
  if(NOT EXISTS "${examples}/${name}")
    message(FATAL_ERROR "${name} was not installed")
  endif()
endforeach()

if(NOT EXISTS "${examples}/README.md")
  message(FATAL_ERROR "the guide to the examples was not installed")
endif()

# A dependency built along the way must not reach the prefix: pugixml ships a
# static library, its headers and a CMake package, and none of them is part of
# the application.
foreach(stray "include" "lib/cmake" "lib/pkgconfig")
  if(EXISTS "${STAGE_DIR}/${stray}")
    message(FATAL_ERROR "${stray} was installed and belongs to a dependency")
  endif()
endforeach()

message(STATUS "installed layout verified: ${shippedCount} examples under ${examples}")
