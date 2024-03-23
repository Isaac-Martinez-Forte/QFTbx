cmake_minimum_required(VERSION 3.17)

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
find_package(Qt6 COMPONENTS Core Widgets PrintSupport REQUIRED)

if (${Qt6_FOUND})
    message("Found Qt " ${Qt6_VERSION})
else()
    message(FATAL_ERROR "Couldn't find Qt")
endif()

if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++20" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++20")
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2 -march=native")

# Añadir la carpeta de includes de Qt6
include_directories(${Qt6Widgets_INCLUDE_DIRS})

add_library (qcustomplot STATIC qcustomplot.cpp qcustomplot.h)

target_link_libraries( qcustomplot Qt6::Core Qt6::Widgets Qt6::PrintSupport Qt6::Gui)

#Opciones del compilador
message("Check C++ version")
# Establecer la versión mínima de C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Verificar si la versión de C++ cumplida es la requerida
if (CMAKE_CXX_STANDARD LESS 20)
    message(FATAL_ERROR "Se requiere C++ versión 20 o superior.")
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
