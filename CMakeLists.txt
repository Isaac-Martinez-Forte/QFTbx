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
option(USE_Doxygen     "Use Doxygen" OFF)
option(USE_NATIVE_ARCH "Enable -march=native" ON)

# Build de desarrollo con sanitizers, p.ej. -DQFTBX_SANITIZERS=address,undefined
# (requiere los runtimes libasan/libubsan instalados). Nota: mientras dure el
# refactor el codigo tiene fugas conocidas; para ejecutar los tests usar
# ASAN_OPTIONS=detect_leaks=0 hasta que cada modulo quede limpio.
set(QFTBX_SANITIZERS "" CACHE STRING "Comma-separated sanitizers for dev builds (e.g. address,undefined)")
if(QFTBX_SANITIZERS)
  add_compile_options(-fsanitize=${QFTBX_SANITIZERS} -fno-omit-frame-pointer)
  add_link_options(-fsanitize=${QFTBX_SANITIZERS})
endif()

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
# CUDA (lenguaje de primera clase de CMake; find_package(CUDA) y
# CUDA_ADD_LIBRARY llevan años obsoletos)
# ------------------------------------------------------------------------------
if(USE_CUDA)
    message("Use CUDA")
    include(CheckLanguage)
    check_language(CUDA)
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        message("CUDA found: ${CMAKE_CUDA_COMPILER}")
        set(CMAKE_CUDA_STANDARD 17)
        set(CMAKE_CUDA_STANDARD_REQUIRED ON)
        # Arquitecturas por defecto razonables; sobreescribir con
        # -DCMAKE_CUDA_ARCHITECTURES=... segun la tarjeta.
        if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
            set(CMAKE_CUDA_ARCHITECTURES 61 75 86)
        endif()
        add_library(qftbx_cuda STATIC
            src/core/gpu/boundary_sheets_cuda.cu
            src/core/gpu/boundary_sheets_cuda.h
            src/core/gpu/template_contour_cuda.cu
            src/core/gpu/template_contour_cuda.h
            src/core/gpu/cuda_utility.cuh
        )
        target_include_directories(qftbx_cuda PUBLIC "${CMAKE_SOURCE_DIR}")
        set_target_properties(qftbx_cuda PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
        add_compile_definitions(CUDA_AVAILABLE=1)
    else()
        message("CUDA NOT found")
    endif()
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
# pugixml (persistencia .qft; MIT, anclada por tag)
# ------------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG        v1.14
)
FetchContent_MakeAvailable(pugixml)

# ------------------------------------------------------------------------------
# Fuentes del proyecto
# ------------------------------------------------------------------------------
# Backend (Modelo, DAO, XmlParser, Consola): compilado como libreria estatica
# para poder enlazarlo tanto desde el ejecutable como desde los tests.
# Nucleo computacional: sin persistencia, sin GUI.
set(QFTbx_CORE_SRC
    src/core/loopshaping/natural_interval_extension.cpp
    src/core/system/transfer_function.cpp
    src/core/system/lti_system.cpp
    src/core/system/polynomial_form.cpp
    src/core/system/zero_pole_gain.cpp
    src/core/system/time_constant_gain.cpp
    src/core/templates/template_engine.cpp
    src/core/math/sequence_vectors.cpp
    src/core/math/parser_warmup.cpp
    src/core/math/expression_cache.cpp
    src/core/frequencies/omega.cpp
    src/core/project_data.cpp
    src/core/text_tokens.cpp
    src/core/boundaries/boundary_data.cpp
    src/core/loopshaping/algorithm_nt.cpp
    src/core/loopshaping/nominal_stability_checker.cpp
    src/core/loopshaping/algorithm_mc1.cpp
    src/core/loopshaping/algorithm_mc_thesis.cpp
    src/core/system/parameter.cpp
    src/core/loopshaping/algorithm_nk.cpp
    src/core/loopshaping/algorithm_mr.cpp
    src/core/boundaries/boundary_engine.cpp
    Consola/consola.cpp
    src/core/system/free_form.cpp
    src/core/loopshaping/loop_shaping.cpp
    src/core/loopshaping/search_node.cpp
    src/core/loopshaping/mc_search_node.cpp
    src/core/loopshaping/boundary_violation_detector.cpp
    src/core/loopshaping/ordered_list.cpp
    src/core/loopshaping/loop_shaping_result.cpp
    src/core/boundaries/boundary_union_1d.cpp
    src/core/boundaries/contour_tracer.cpp
    src/core/loopshaping/expression_tree.cpp
    src/core/loopshaping/box_classification.cpp
)

set(QFTbx_CORE_HED
    src/core/system/transfer_function.h
    src/core/loopshaping/natural_interval_extension.h
    src/core/system/lti_system.h
    src/core/system/polynomial_form.h
    src/core/system/zero_pole_gain.h
    src/core/system/time_constant_gain.h
    src/core/templates/template_engine.h
    src/core/templates/parameter_grids.h
    src/core/math/sequence_vectors.h
    src/core/math/parser_warmup.h
    src/core/math/expression_cache.h
    src/core/specifications/specification_record.h
    src/core/project_data.h
    src/core/range.h
    src/core/text_tokens.h
    src/core/loopshaping/loop_shaping_types.h
    src/core/math/sequences.h
    src/core/specifications/specification.h
    src/core/exception.h
    src/core/frequencies/omega.h
    src/core/boundaries/boundary_data.h
    src/core/loopshaping/algorithm_nt.h
    src/core/loopshaping/nominal_stability_checker.h
    src/core/loopshaping/quick_solution.h
    src/core/loopshaping/algorithm_mc1.h
    src/core/loopshaping/algorithm_mc_thesis.h
    src/core/system/parameter.h
    src/core/loopshaping/algorithm_nk.h
    src/core/loopshaping/algorithm_mr.h
    src/core/loopshaping/expression_tree.h
    src/core/boundaries/boundary_engine.h
    Consola/consola.h
    src/core/system/free_form.h
    src/core/loopshaping/loop_shaping.h
    src/core/loopshaping/list_node.h
    src/core/loopshaping/search_node.h
    src/core/loopshaping/mc_search_node.h
    src/core/loopshaping/stages.h
    src/core/loopshaping/boundary_violation_detector.h
    src/core/loopshaping/ordered_list.h
    src/core/loopshaping/loop_shaping_result.h
    src/core/boundaries/boundary_union_1d.h
    src/core/boundaries/contour_tracer.h
    src/core/loopshaping/common_functions.h
    src/core/loopshaping/box_classification.h
)

# Persistencia de proyectos (.qft): depende del nucleo.
set(QFTbx_PERSISTENCE_SRC
    src/persistence/project_reader.cpp
    src/persistence/project_writer.cpp
)

set(QFTbx_PERSISTENCE_HED
    src/persistence/project_reader.h
    src/persistence/project_writer.h
    src/persistence/qft_dialect.h
)

# GUI (HMI Qt): los dialogos y visores. main.cpp va aparte, en el ejecutable.
set(QFTbx_GUI_SRC
    GUI/parlineedit.cpp
    GUI/parlineedit.h
    GUI/error_message.cpp
    GUI/bode_viewer.cpp
    GUI/main_window.cpp
    GUI/plant_dialog.cpp
    GUI/frequencies_dialog.cpp
    GUI/uncertainty_dialog.cpp
    GUI/boundary_viewer.cpp
    GUI/boundary_grid_dialog.cpp
    GUI/specifications_dialog.cpp
    GUI/controller_dialog.cpp
    GUI/boundary_union_viewer.cpp
    GUI/loop_shaping_dialog.cpp
    GUI/loop_shaping_viewer.cpp
    GUI/templates_dialog.cpp
    GUI/template_viewer.cpp
    GUI/loop_boundaries_viewer.cpp
)

# Fachada de la aplicacion: coordina el nucleo y la persistencia.
set(QFTbx_APP_SRC
    src/core/project_controller.cpp
)

set(QFTbx_APP_HED
    src/core/project_controller.h
)


set(QFTbx_GUI_HED
    GUI/bode_viewer.h
    GUI/error_message.h
    GUI/plot_palette.h
    GUI/main_window.h
    GUI/plant_dialog.h
    GUI/frequencies_dialog.h
    GUI/uncertainty_dialog.h
    GUI/boundary_viewer.h
    GUI/boundary_grid_dialog.h
    GUI/specifications_dialog.h
    GUI/controller_dialog.h
    GUI/boundary_union_viewer.h
    GUI/loop_shaping_dialog.h
    GUI/loop_shaping_viewer.h
    GUI/templates_dialog.h
    GUI/template_viewer.h
    GUI/loop_boundaries_viewer.h
)

# Recursos
qt6_add_resources(QFTbx_RES_SRC Resources/Images/imagenesPlanta.qrc)
qt6_add_resources(QFTbx_ICON_RES Resources/Icon/icons.qrc)


# ------------------------------------------------------------------------------
# Layered libraries. The link order IS the architecture, and the linker
# enforces it: core knows nothing of files or widgets, persistence knows
# nothing of the facade or the GUI, and the GUI reaches the core only
# through the facade.
#
#   qftbx_core  <-  qftbx_persistence  <-  qftbx_app  <-  qftbx_gui  <- QFTbx
#
# Everything is static: the application is meant to be run from the build
# directory, and static linking keeps that free of library-path setup.
# ------------------------------------------------------------------------------

# The computational core: plants, frequencies, specifications, templates,
# boundaries and the loop-shaping algorithms. GUI-free since the debug
# viewers left with the legacy algorithms: it links Qt Core (containers) and
# Qt Gui (a few headers take QPointF) but neither Widgets nor PrintSupport.
add_library(qftbx_core STATIC
    ${QFTbx_CORE_SRC}
    ${QFTbx_CORE_HED}
)

target_include_directories(qftbx_core PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(qftbx_core PUBLIC Qt6::Core Qt6::Gui)

# Project persistence (.qft): the only place that knows the file format.
add_library(qftbx_persistence STATIC
    ${QFTbx_PERSISTENCE_SRC}
    ${QFTbx_PERSISTENCE_HED}
)

target_link_libraries(qftbx_persistence PUBLIC qftbx_core)
target_link_libraries(qftbx_persistence PRIVATE pugixml::pugixml)

# The application facade: owns the project data and drives the engines and
# the persistence. It is what the GUI and the tests talk to.
add_library(qftbx_app STATIC
    ${QFTbx_APP_SRC}
    ${QFTbx_APP_HED}
)

target_link_libraries(qftbx_app PUBLIC qftbx_core qftbx_persistence)

# Transitional alias: consumers still ask for "qftbx_backend".
add_library(qftbx_backend INTERFACE)
target_link_libraries(qftbx_backend INTERFACE qftbx_app)

# ------------------------------------------------------------------------------
# Libreria de la GUI: los dialogos y visores, sin main(), para que la suite
# pueda instanciarlos (el humo de GUI corre con QT_QPA_PLATFORM=offscreen).
# ------------------------------------------------------------------------------
add_library(qftbx_gui STATIC
    ${QFTbx_GUI_SRC}
    ${QFTbx_GUI_HED}
    ${QFTbx_ICON_RES}
    ${QFTbx_RES_SRC}
)

target_include_directories(qftbx_gui PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(qftbx_gui PUBLIC qftbx_app Qt6::Widgets Qt6::PrintSupport)
target_compile_options(qftbx_gui PRIVATE -Wall -Wextra)

# ------------------------------------------------------------------------------
# Ejecutable
# ------------------------------------------------------------------------------
add_executable(${EXE_NAME}
    main.cpp
    ${TS_FILES}
)

target_link_libraries(${EXE_NAME} PRIVATE qftbx_gui)

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
target_compile_options(${EXE_NAME} PRIVATE -Wall -Wextra)
target_compile_options(qftbx_core PRIVATE -Wall -Wextra)
target_compile_options(qftbx_persistence PRIVATE -Wall -Wextra)
target_compile_options(qftbx_app PRIVATE -Wall -Wextra)
if(USE_NATIVE_ARCH AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
  target_compile_options(${EXE_NAME} PRIVATE -march=native)
  target_compile_options(qftbx_core PRIVATE -march=native)
  target_compile_options(qftbx_persistence PRIVATE -march=native)
  target_compile_options(qftbx_app PRIVATE -march=native)
endif()

# Every translation unit that includes the C-XSC headers compiles its
# INLINE directed-rounding interval operators (interval.inl: subdown/subup
# switch the FPU rounding mode around each endpoint). Without this flag the
# optimiser assumes round-to-nearest everywhere and reorders/fuses the
# arithmetic across the mode switches, producing intervals with the lower
# end above the upper one (observed live at -O3 -march=native: the HC4
# filter aborted on ERROR_INTERVAL_EMPTY_INTERVAL raised by a plain
# interval subtraction). The whole interval-arithmetic correctness of the
# toolbox rests on this flag. It cannot be global: it demotes Qt's
# constexpr float arithmetic, so it applies to the interval-computing
# targets only.
target_compile_options(qftbx_core PRIVATE -frounding-math)

# OpenMP link
if(USE_OpenMP AND OpenMP_CXX_FOUND)
  # PUBLIC: la definicion OpenMP_AVAILABLE cambia declaraciones en cabeceras
  # compartidas (p.ej. templates.h), asi que debe propagarse a los consumidores.
  target_link_libraries(qftbx_core PUBLIC OpenMP::OpenMP_CXX)
  target_compile_definitions(qftbx_core PUBLIC OpenMP_AVAILABLE=1)
else()
  if(USE_OpenMP)
    message("OpenMP NOT FOUND")
  endif()
endif()

# CUDA link
if(TARGET qftbx_cuda)
    target_link_libraries(qftbx_core PUBLIC qftbx_cuda)
endif()

set_property(TARGET ${EXE_NAME} APPEND PROPERTY
    INTERFACE_AUTOUIC_OPTIONS
)

qt6_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

# ------------------------------------------------------------------------------
# LIBS internas: muparserx y qcustomplot como subdirectorios
# ------------------------------------------------------------------------------
add_subdirectory(3rd-party/muparserx EXCLUDE_FROM_ALL)
target_link_libraries(qftbx_core PUBLIC muparserx::muparserx)

add_subdirectory(3rd-party/qcustomplot EXCLUDE_FROM_ALL)
#Only the GUI plots: the backend no longer touches qcustomplot.
target_link_libraries(qftbx_gui PUBLIC qcustomplot::qcustomplot)

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
target_link_libraries(qftbx_core PUBLIC cxsc::cxsc)

# RPATH en Linux para encontrar libcxsc.so al ejecutar desde build
if(UNIX AND NOT APPLE AND NOT WIN32)
  set_target_properties(${EXE_NAME} PROPERTIES
    BUILD_RPATH "${CXSC_LIBDIR}"
  )
endif()

# ------------------------------------------------------------------------------
# Tests (GoogleTest)
# ------------------------------------------------------------------------------
option(QFTBX_BUILD_TESTS "Build unit tests" ON)

if(QFTBX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()