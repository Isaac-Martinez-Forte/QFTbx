cmake_minimum_required(VERSION 3.17)

project(QFTbx LANGUAGES CXX)

set(EXE_NAME QFTbx)

# Qt auto tools
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Opciones de configuracion
option(USE_CLANG       "Use CLANG"   OFF)
option(USE_OpenMP      "Use OpenMP"  ON)
option(USE_CUDA        "Use CUDA"    OFF)
option(USE_OpenCL      "Use OpenCL"  OFF)
option(USE_Doxygen     "Use Doxygen" OFF)
option(USE_QML         "Use QML"     OFF)
option(USE_NATIVE_ARCH "Enable -march=native" ON)

if(WIN32)
    #set (CMAKE_PREFIX_PATH "C:\\Qt\\6.6.2\\msvc2019_64")
    set (CMAKE_PREFIX_PATH "C:\\Qt\\6.6.2\\mingw_64")
endif()

message("Looking for Qt...")
find_package(Qt6 REQUIRED COMPONENTS Core Widgets PrintSupport LinguistTools)

message("Found Qt ${Qt6_VERSION}")

# ------------------------------------------------------------------------------
# OpenMP
# ------------------------------------------------------------------------------
if(USE_OpenMP)
  message("Use OpenMP")
  find_package(OpenMP)
endif()

# ------------------------------------------------------------------------------
# CUDA
# ------------------------------------------------------------------------------
if(USE_CUDA)
    message("Use CUDA")
    find_package(CUDA)
    if(CUDA_FOUND)
        message("CUDA found")
        list(APPEND CUDA_NVCC_FLAGS "-std=c++20;-O3;-DVERBOSE")
        set(CUDA_PROPAGATE_HOST_FLAGS OFF)
        set(CUDA_SEPARABLE_COMPILATION ON)
        file(GLOB QFTbx_CUDA GPU/CUDA/bnd_cuda.cu GPU/CUDA/e_hull_cuda.cu)
        add_definitions(-DCUDA_AVAILABLE=1)
        CUDA_ADD_LIBRARY(CUDA STATIC ${QFTbx_CUDA})
    else()
        message("CUDA NOT found")
    endif()
endif()

# ------------------------------------------------------------------------------
# OpenCL 
# ------------------------------------------------------------------------------
if(USE_OpenCL)
    find_package(OpenCL)
endif()

# ------------------------------------------------------------------------------
# Doxygen 
# ------------------------------------------------------------------------------
if (USE_Doxygen)
    message("Use Doxygen")
    find_package(Doxygen)
    option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

    if(BUILD_DOCUMENTATION)
        if(NOT DOXYGEN_FOUND)
            message(FATAL_ERROR "Doxygen is needed to build the documentation.")
        endif()
        set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
        set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile)
        configure_file(${doxyfile_in} ${doxyfile})
        add_custom_target(doc1
            COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
        add_custom_target(doc
            COMMAND make -f ${CMAKE_CURRENT_BINARY_DIR}/doc/latex/Makefile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/latex
            COMMENT "Generating Latex documentation with TexLive "
            VERBATIM
        )
        add_dependencies(doc doc1)
    endif()
endif()

set(TS_FILES QFTbx_es_ES.ts)

# ------------------------------------------------------------------------------
# Fuentes del proyecto
# ------------------------------------------------------------------------------
set(QFTbx_SRC main.cpp
    Modelo/LoopShaping/NaturalIntervalExtension/natural_interval_extension.cpp
    Modelo/EstructuraSistema/funciontransferencia.cpp
    DAO/adaptadorplantadao.cpp
    Modelo/EstructuraSistema/sistema.cpp
    Modelo/EstructuraSistema/polosceros.cpp
    Modelo/EstructuraSistema/cpolinomios.cpp
    Modelo/EstructuraSistema/kganancia.cpp
    Modelo/EstructuraSistema/knganancia.cpp
    GUI/diagramabode.cpp
    GUI/windowsgeneral.cpp
    GUI/introducirplanta.cpp
    GUI/intomega.cpp
    GUI/intincertidumbre.cpp
    Modelo/controlador.cpp
    Modelo/EstructurasDatos/parlineedit.cpp
    DAO/adaptadoromegadao.cpp
    Modelo/Objetos/incertidumbre.cpp
    Modelo/Templates/templates.cpp
    DAO/adaptadortemplatedao.cpp
    Modelo/Herramientas/tools.cpp
    Modelo/Objetos/omega.cpp
    DAO/adaptadorbounddao.cpp
    GUI/viewbound.cpp
    XmlParser/xmlparsersave.cpp
    Modelo/EstructurasDatos/datosplanta.cpp
    XmlParser/xmlparserload.cpp
    DAO/fdao.cpp
    GUI/intdatosboundaries.cpp
    Modelo/EstructurasDatos/datosbound.cpp
    GUI/intespecificaciones.cpp
    DAO/adaptadorespecificacionesdao.cpp
    GUI/introducirecontr.cpp
    DAO/adaptadorcontroladordao.cpp
    Modelo/LoopShaping/algorithm_sachin.cpp
    Modelo/LoopShaping/algorithm_primer_articulo.cpp
    Modelo/LoopShaping/algorithm_segundo_articulo.cpp
    Modelo/EstructurasDatos/var.cpp
    Modelo/LoopShaping/algorithm_nandkishor.cpp
    Modelo/LoopShaping/algorithm_rambabu.cpp
    GUI/viewboundreun.cpp
    Modelo/Boundaries/boundaries.cpp
    Modelo/Boundaries/contour.cpp
    Consola/consola.cpp
    Modelo/EstructuraSistema/formatolibre.cpp
    GUI/intloopshaping.cpp
    Modelo/LoopShaping/loopshaping.cpp
    GUI/viewloopshaping.cpp
    Modelo/LoopShaping/EstructuraDatos/datosfeasible.cpp
    Modelo/LoopShaping/EstructuraDatos/tripleta.cpp
    Modelo/LoopShaping/EstructuraDatos/tripleta2.cpp
    Modelo/LoopShaping/DeteccionViolacionBoundaries/deteccionviolacionboundaries.cpp
    Modelo/LoopShaping/EstructuraDatos/listaordenada.cpp
    DAO/adaptadorloopshapingdao.cpp
    Modelo/EstructurasDatos/datosloopshaping.cpp
    GUI/introducirtemplates.cpp
    GUI/viewtemplates.cpp
    GUI/verboundaries.cpp
    Modelo/Boundaries/algoritmointerseccionlineal1D.cpp
    Modelo/Boundaries/algoritmointerseccionproyectivo2D.cpp
    Modelo/Boundaries/contour2.cpp
    Modelo/LoopShaping/EstructuraDatos/arbol_exp.cpp
    Modelo/LoopShaping/EstructuraDatos/data_box.cpp
)

set(QFTbx_OpenCL GPU/OpenCL/e_hull_opencl.cl)

set(QFTbx_HED
    Modelo/EstructuraSistema/funciontransferencia.h
    Modelo/LoopShaping/NaturalIntervalExtension/natural_interval_extension.h
    DAO/adaptadorplantadao.h
    Modelo/EstructuraSistema/sistema.h
    Modelo/EstructuraSistema/polosceros.h
    Modelo/EstructuraSistema/cpolinomios.h
    Modelo/EstructuraSistema/kganancia.h
    Modelo/EstructuraSistema/knganancia.h
    GUI/diagramabode.h
    GUI/windowsgeneral.h
    GUI/introducirplanta.h
    DAO/plantadao.h
    GUI/intomega.h
    GUI/intincertidumbre.h
    Modelo/controlador.h
    Modelo/EstructurasDatos/parlineedit.h
    DAO/adaptadoromegadao.h
    DAO/omegadao.h
    Modelo/Objetos/incertidumbre.h
    Modelo/Templates/templates.h
    DAO/templatedao.h
    DAO/adaptadortemplatedao.h
    Modelo/Herramientas/tools.h
    Modelo/Objetos/omega.h
    DAO/bounddao.h
    DAO/adaptadorbounddao.h
    GUI/viewbound.h
    Modelo/EstructurasDatos/datosplanta.h
    DAO/dao.h
    DAO/fdao.h
    GUI/intdatosboundaries.h
    Modelo/EstructurasDatos/datosbound.h
    XmlParser/parserload.h
    XmlParser/parsersave.h
    GUI/intespecificaciones.h
    DAO/especificacionesdao.h
    DAO/adaptadorespecificacionesdao.h
    GUI/introducirecontr.h
    DAO/adaptadorcontroladordao.h
    DAO/controladordao.h
    Modelo/LoopShaping/algorithm_sachin.h
    Modelo/LoopShaping/algorithm_primer_articulo.h
    Modelo/LoopShaping/algorithm_segundo_articulo.h
    Modelo/EstructurasDatos/var.h
    Modelo/LoopShaping/algorithm_nandkishor.h
    Modelo/LoopShaping/algorithm_rambabu.h
    Modelo/LoopShaping/EstructuraDatos/arbol_exp.h
    GUI/viewboundreun.h
    Modelo/Boundaries/boundaries.h
    Modelo/Boundaries/contour.h
    Consola/consola.h
    Modelo/EstructuraSistema/formatolibre.h
    GUI/intloopshaping.h
    Modelo/LoopShaping/loopshaping.h
    GUI/viewloopshaping.h
    Modelo/LoopShaping/EstructuraDatos/avl.h
    Modelo/LoopShaping/EstructuraDatos/n.h
    Modelo/LoopShaping/EstructuraDatos/datosfeasible.h
    Modelo/LoopShaping/EstructuraDatos/tripleta.h
    Modelo/LoopShaping/EstructuraDatos/tripleta2.h
    Modelo/LoopShaping/EstructuraDatos/etapas.h
    Modelo/LoopShaping/EstructuraDatos/nodo.h
    Modelo/LoopShaping/DeteccionViolacionBoundaries/deteccionviolacionboundaries.h
    Modelo/LoopShaping/EstructuraDatos/listaordenada.h
    DAO/loopshapingdao.h
    DAO/adaptadorloopshapingdao.h
    Modelo/EstructurasDatos/datosloopshaping.h
    GUI/introducirtemplates.h
    GUI/viewtemplates.h
    GUI/verboundaries.h
    Modelo/Boundaries/algoritmointerseccionlineal1D.h
    Modelo/Boundaries/algoritmointerseccionproyectivo2D.h
    Modelo/Boundaries/contour2.h
    Modelo/LoopShaping/funcionescomunes.h
    Modelo/LoopShaping/EstructuraDatos/data_box.h
)

# Recursos
qt6_add_resources(QFTbx_RES_SRC Resources/Images/imagenesPlanta.qrc)
qt6_add_resources(QFTbx_ICON_RES Resources/Icon/icons.qrc)


# ------------------------------------------------------------------------------
# Ejecutable
# ------------------------------------------------------------------------------
add_executable(${EXE_NAME}
    ${QFTbx_SRC}
    ${QFTbx_HED}
    ${QFTbx_QML}
    ${CUDA_OBJECTS}
    ${QFTbx_OpenCL}
    ${TS_FILES}
    ${QFTbx_ICON_RES}
    ${QFTbx_RES_SRC}
)

target_include_directories(${EXE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}")

target_link_libraries(${EXE_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::PrintSupport)

if(WIN32)
  target_sources(${EXE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Resources/Icon/qftbx.rc)
endif()

if(UNIX AND NOT APPLE)
  # Ruta absoluta del icono en el source tree
  set(QFTBX_ICON "${CMAKE_SOURCE_DIR}/Resources/Icon/qftbx_256.png")

  # Generar .desktop en el build dir (portable, sin install)
  # Usamos generator expressions para capturar la ruta real del ejecutable
  set(QFTBX_DESKTOP_IN  "${CMAKE_SOURCE_DIR}/Resources/Icon/QFTbx.desktop.in")
  set(QFTBX_DESKTOP_OUT "${CMAKE_BINARY_DIR}/QFTbx.desktop")

  # 1) Generamos un archivo intermedio con placeholders que incluyen generator expressions
  file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/QFTbx.desktop.gen.in"
    CONTENT
"[Desktop Entry]\n\
Type=Application\n\
Version=1.0\n\
Name=QFTbx\n\
Comment=Quantitative Feedback Theory toolbox (robust control)\n\
Exec=$<TARGET_FILE:${EXE_NAME}>\n\
Path=${CMAKE_BINARY_DIR}\n\
Icon=${QFTBX_ICON}\n\
Terminal=false\n\
Categories=Science;Engineering;\n\
StartupNotify=true\n"
  )

  # 2) Copiamos el intermedio al .desktop final (para tener un nombre estable)
  add_custom_command(
    TARGET ${EXE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/QFTbx.desktop.gen.in"
            "${QFTBX_DESKTOP_OUT}"
    COMMENT "Generating portable desktop file: ${QFTBX_DESKTOP_OUT}"
  )

  # 3) (Opcional) hacerlo ejecutable automáticamente
  add_custom_command(
    TARGET ${EXE_NAME} POST_BUILD
    COMMAND chmod +x "${QFTBX_DESKTOP_OUT}"
    COMMENT "Marking QFTbx.desktop as executable"
  )
endif()

# Warnings/arch 
target_compile_options(${EXE_NAME} PRIVATE -Wall)
if(USE_NATIVE_ARCH AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
  target_compile_options(${EXE_NAME} PRIVATE -march=native)
endif()

# OpenMP link
if(USE_OpenMP AND OpenMP_CXX_FOUND)
  target_link_libraries(${EXE_NAME} PRIVATE OpenMP::OpenMP_CXX)
  target_compile_definitions(${EXE_NAME} PRIVATE OpenMP_AVAILABLE=1)
else()
  if(USE_OpenMP)
    message("OpenMP NOT FOUND")
  endif()
endif()

# CUDA link
if(CUDA_FOUND)
    add_dependencies(${EXE_NAME} CUDA)
    target_link_libraries(${EXE_NAME} PRIVATE CUDA)
endif()

# OpenCL link
if(USE_OpenCL AND OpenCL_FOUND)
  if(TARGET OpenCL::OpenCL)
    target_link_libraries(${EXE_NAME} PRIVATE OpenCL::OpenCL)
  else()
    target_link_libraries(${EXE_NAME} PRIVATE ${OpenCL_LIBRARY})
    target_include_directories(${EXE_NAME} PRIVATE ${OpenCL_INCLUDE_DIRS})
  endif()
  target_compile_definitions(${EXE_NAME} PRIVATE OpenCL_AVAILABLE=1)
endif()

set_property(TARGET ${EXE_NAME} APPEND PROPERTY
    INTERFACE_AUTOUIC_OPTIONS
)

qt6_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

# ------------------------------------------------------------------------------
# LIBS internas: muparserx y qcustomplot como subdirectorios
# ------------------------------------------------------------------------------
add_subdirectory(3rd-party/muparserx EXCLUDE_FROM_ALL)
target_link_libraries(${EXE_NAME} PRIVATE muparserx::muparserx)

add_subdirectory(3rd-party/qcustomplot EXCLUDE_FROM_ALL)
target_link_libraries(${EXE_NAME} PRIVATE qcustomplot::qcustomplot)

# ------------------------------------------------------------------------------
# CXSC aislada (NO tocar cxsc). ExternalProject + imported target.
# ------------------------------------------------------------------------------
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
  PREFIX "${CMAKE_BINARY_DIR}/3rd-party/cxsc/ep"
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/3rd-party/cxsc"
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
target_link_libraries(${EXE_NAME} PRIVATE cxsc::cxsc)

# RPATH en Linux para encontrar libcxsc.so al ejecutar desde build
if(UNIX AND NOT APPLE AND NOT WIN32)
  set_target_properties(${EXE_NAME} PROPERTIES
    BUILD_RPATH "${CXSC_LIBDIR}"
  )
endif()