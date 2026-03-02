cmake_minimum_required(VERSION 3.17)
project(qcustomplot LANGUAGES CXX)

# Si el proyecto padre ya encontró Qt6, no lo busques otra vez.
if(NOT TARGET Qt6::Core)
  message("Looking for Qt...")
  find_package(Qt6 REQUIRED COMPONENTS Core Widgets PrintSupport)
  message("Found Qt ${Qt6_VERSION}")
endif()

add_library(qcustomplot STATIC qcustomplot.cpp qcustomplot.h)
add_library(qcustomplot::qcustomplot ALIAS qcustomplot)

target_compile_features(qcustomplot PUBLIC cxx_std_20)

# Nada de -O2/-march=native global aquí: que el proyecto padre decida eso.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(qcustomplot PRIVATE -Wall)
endif()

target_link_libraries(qcustomplot
  PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
)

# Para poder #include "qcustomplot.h"
target_include_directories(qcustomplot
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)