########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.12)
project(muparserx CXX)

########################################################################
# Extract version
########################################################################
set(MUPARSERX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/parser)
file(READ "${MUPARSERX_SOURCE_DIR}/mpDefines.h" mpDefines_h)
string(REGEX MATCH "\\#define MUP_PARSER_VERSION _T\\(\"([0-9]+\\.[0-9]+\\.[0-9]+) \\(" MUPARSERX_VERSION_MATCHES "${mpDefines_h}")
if(NOT MUPARSERX_VERSION_MATCHES)
    message(FATAL_ERROR "Failed to extract version number from mpDefines.h")
endif(NOT MUPARSERX_VERSION_MATCHES)
set(MUPARSERX_VERSION ${CMAKE_MATCH_1})

########################################################################
# Compiler specific flags
########################################################################
if(CMAKE_COMPILER_IS_GNUCXX)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -march=native")

endif(CMAKE_COMPILER_IS_GNUCXX)

#enable c++11 extensions for OSX
if (APPLE)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wc++11-extensions")
endif(APPLE)

if(MSVC)
    add_compile_options(/MP) #multi-core build
endif(MSVC)

########################################################################
# Build library
# Defaults to static, set BUILD_SHARED_LIBS=ON for shared
########################################################################
file(GLOB MUPARSERX_SOURCES "${MUPARSERX_SOURCE_DIR}/*.cpp")
include_directories(${MUPARSERX_SOURCE_DIR})
add_library(muparserx ${MUPARSERX_SOURCES})
set_target_properties(muparserx PROPERTIES VERSION ${MUPARSERX_VERSION})
set_property(TARGET muparserx PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(muparserx PROPERTIES SOVERSION ${MUPARSERX_VERSION})
set_target_properties(muparserx PROPERTIES VERSION ${MUPARSERX_VERSION})

#Opciones del compilador
message("Check C++ version")
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
if(COMPILER_SUPPORTS_CXX17)
        message("Use c++17")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

        set_target_properties(${EXE_NAME} PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
        )
else ()
    CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
    if(COMPILER_SUPPORTS_CXX14)
            message("Use c++14")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

            set_target_properties(${EXE_NAME} PROPERTIES
                CXX_STANDARD 14
                CXX_STANDARD_REQUIRED ON
                CXX_EXTENSIONS OFF
            )
    else ()
        CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
        if(COMPILER_SUPPORTS_CXX11)
                message("Use c++11")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

                set_target_properties(${EXE_NAME} PROPERTIES
                    CXX_STANDARD 11
                    CXX_STANDARD_REQUIRED ON
                    CXX_EXTENSIONS OFF
                )
        endif()
    endif()
endif()


#link with lib math when found
find_library(
    M_LIBRARY NAMES m
    PATHS /usr/lib /usr/lib64
)
if(M_LIBRARY)
    target_link_libraries(muparserx ${M_LIBRARY})
endif(M_LIBRARY)

install(TARGETS muparserx
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Install headers
########################################################################
file(GLOB MUPARSERX_HEADERS "${MUPARSERX_SOURCE_DIR}/*.h")
install(
    FILES ${MUPARSERX_HEADERS}
    DESTINATION include/
)

########################################################################
# Example application
########################################################################
option(BUILD_EXAMPLES "enable building example applications" ON)
if(BUILD_EXAMPLES)
    add_executable(example sample/example.cpp sample/timer.cpp)
    target_link_libraries(example muparserx)
    
    install(TARGETS example
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)
endif(BUILD_EXAMPLES)

########################################################################
# Print summary
########################################################################
message(STATUS "Building muparserx version: ${MUPARSERX_VERSION}")
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
