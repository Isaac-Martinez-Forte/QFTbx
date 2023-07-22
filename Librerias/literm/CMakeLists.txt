cmake_minimum_required(VERSION 3.5)

project(literm LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()
if(NOT DEFINED ${DEFAULT_FONT})
    if (WIN32)
        add_definitions(-DDEFAULT_FONT="monospace")
    elseif (APPLE)
        add_definitions(-DDEFAULT_FONT="Monaco")
    elseif (UNIX)
        add_definitions(-DDEFAULT_FONT="monospace")
        add_definitions(-DDETECT_FONT_USING_FC_MATCH)
    endif()
endif()

add_definitions(-DDEFAULT_FONTFAMILY=${DEFAULT_FONT})
add_definitions(-DDEFAULT_DRAG_MODE="select")
add_definitions(-DDESKTOP_BUILD)

find_package(Qt5 COMPONENTS Core Quick REQUIRED)

set (HEADERS ptyiface.h 
    terminal.h 
    textrender.h 
    version.h 
    utilities.h 
    keyloader.h )
    
set (SOURCES main.cpp 
    terminal.cpp 
    textrender.cpp 
    ptyiface.cpp 
    utilities.cpp 
    keyloader.cpp)


add_executable(literm
    ${HEADERS}
    ${SOURCES}
    qml.qrc
    resources.qrc
)

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

target_compile_definitions(literm
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(literm
  PRIVATE Qt5::Core Qt5::Quick -lutil)

  
########################################################################
# Install
########################################################################
install(TARGETS literm
    RUNTIME DESTINATION bin              # .dll file
)
