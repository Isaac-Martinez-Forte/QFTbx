# Writes the header that carries the version into the program. Run at BUILD
# time, not at configure time, so the commit it names is the one being built
# and not the one the build tree was set up at. The file is only replaced when
# its contents change, so a build that adds nothing recompiles nothing.

if(NOT DEFINED SOURCE_DIR OR NOT DEFINED OUTPUT OR NOT DEFINED VERSION)
  message(FATAL_ERROR "WriteVersionHeader.cmake: an argument was not given")
endif()

set(commit "")
find_program(git_program NAMES git)
if(git_program)
  execute_process(
    COMMAND "${git_program}" -C "${SOURCE_DIR}" rev-parse --short HEAD
    OUTPUT_VARIABLE commit
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  execute_process(
    COMMAND "${git_program}" -C "${SOURCE_DIR}" describe --exact-match --tags HEAD
    RESULT_VARIABLE on_a_tag
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(on_a_tag EQUAL 0)
    set(commit "")
  endif()
endif()

set(contents "#ifndef QFTBX_VERSION_H\n#define QFTBX_VERSION_H\n\n")
string(APPEND contents "#define QFTBX_VERSION \"${VERSION}\"\n")
string(APPEND contents "#define QFTBX_COMMIT \"${commit}\"\n\n")
string(APPEND contents "#endif\n")

file(WRITE "${OUTPUT}.candidate" "${contents}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${OUTPUT}.candidate" "${OUTPUT}")
file(REMOVE "${OUTPUT}.candidate")
