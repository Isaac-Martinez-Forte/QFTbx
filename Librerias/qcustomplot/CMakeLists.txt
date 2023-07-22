cmake_minimum_required(VERSION 3.5)

project(qcustomplot LANGUAGES CXX)

set(EXE_NAME qcustomplot)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD_REQUIRED ON)


if( WIN32 )
    #set (CMAKE_PREFIX_PATH "C:\\Qt\\5.11.0\\msvc2015\\")
    set (CMAKE_PREFIX_PATH "D:\\Qt\\5.15.2\\mingw81_64\\")
endif()

message("Looking for Qt...")
#Buscamos los paquetes de Qt5
find_package(Qt5 COMPONENTS Core Widgets PrintSupport REQUIRED)

if (${Qt5_FOUND})
    message("Found Qt " ${Qt5_VERSION})
else()
    message(FATAL_ERROR "Couldn't find Qt")
endif()

if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2 -march=native")

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

add_library (qcustomplot STATIC qcustomplot.cpp qcustomplot.h)

target_link_libraries( qcustomplot Qt5::Core Qt5::Widgets Qt5::PrintSupport Qt5::Gui)

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall  -O2 -march=native")

install(TARGETS qcustomplot
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Install headers
########################################################################
file(GLOB qcustomplot.h "${CMAKE_SOURCE_DIR}/*.h")
file(GLOB qcustomplot.cpp "${CMAKE_SOURCE_DIR}/*.cpp")
install(
    FILES qcustomplot.h qcustomplot.cpp
    DESTINATION include
)
