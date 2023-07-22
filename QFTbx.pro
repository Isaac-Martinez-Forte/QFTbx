#-------------------------------------------------
#
# Project created by QtCreator 2012-12-13T14:40:19
#
#-------------------------------------------------

QT += core gui printsupport widgets

include(config.pri) #Configuración de la compilación.

greaterThan(QT_MAJOR_VERSION, 5):

QMAKE_CXXFLAGS += -std=c++11 -std=gnu++11

#DEFINES += RECSUP

#CONFIG += debug

contains (DEFINES, OpenMP_AVAILABLE) {
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS += -fopenmp
    message (Se compila con soporte OpenMP.)
} else {
    message (Se compila sin soporte OpenMP.)
}

TARGET = QFTbx
TEMPLATE = app

SOURCES += main.cpp\
    Modelo/LoopShaping/NaturalIntervalExtension/natural_interval_extension.cpp\
    Modelo/EstructuraSistema/funciontransferencia.cpp\
    DAO/adaptadorplantadao.cpp\
    Modelo/EstructuraSistema/sistema.cpp\
    Modelo/EstructuraSistema/polosceros.cpp\
    Modelo/EstructuraSistema/cpolinomios.cpp\
    Modelo/EstructuraSistema/kganancia.cpp\
    Modelo/EstructuraSistema/knganancia.cpp\
    #Modelo/EstructuraSistema/sistemasimplificada.cpp\
    #Modelo/EstructuraSistema/funciontransferenciasimplificada.cpp\
    #Modelo/EstructuraSistema/poloscerossimplificada.cpp\
    #Modelo/EstructuraSistema/kgananciasimplificada.cpp\
    GUI/diagramabode.cpp\
    GUI/windowsgeneral.cpp\
    GUI/introducirplanta.cpp\
    GUI/intomega.cpp\
    GUI/intincertidumbre.cpp\
    Modelo/controlador.cpp\
    Modelo/EstructurasDatos/parlineedit.cpp\
    DAO/adaptadoromegadao.cpp\
    Modelo/Objetos/incertidumbre.cpp\
    Modelo/Templates/templates.cpp\
    DAO/adaptadortemplatedao.cpp\
    Modelo/Herramientas/tools.cpp\
    Modelo/Objetos/omega.cpp\
    DAO/adaptadorbounddao.cpp\
    GUI/viewbound.cpp\
    XmlParser/xmlparsersave.cpp\
    Modelo/EstructurasDatos/datosplanta.cpp\
    XmlParser/xmlparserload.cpp\
    DAO/fdao.cpp\
    GUI/intdatosboundaries.cpp\
    Modelo/EstructurasDatos/datosbound.cpp\
    GUI/intespecificaciones.cpp\
    DAO/adaptadorespecificacionesdao.cpp\
    GUI/introducirecontr.cpp\
    DAO/adaptadorcontroladordao.cpp\
    Modelo/LoopShaping/algorithm_sachin.cpp\
    Modelo/LoopShaping/algorithm_isaac.cpp\
    #Modelo/LoopShaping/algorithm_isaac_hilo.cpp\
    Modelo/EstructurasDatos/var.cpp\
    Modelo/LoopShaping/algorithm_nandkishor.cpp\
    #Modelo/LoopShaping/algorithm_rambabu.cpp\
    GUI/viewboundreun.cpp\
    Modelo/Boundaries/boundaries.cpp\
    Modelo/Boundaries/contour.cpp\
    Consola/consola.cpp\
    Modelo/EstructuraSistema/formatolibre.cpp\
    GUI/intloopshaping.cpp\
    Modelo/LoopShaping/loopshaping.cpp\
    GUI/viewloopshaping.cpp\
    Modelo/LoopShaping/DeteccionViolacionBoundaries/deteccionviolacionboundaries.cpp\
    #Modelo/LoopShaping/TestDeteccion.cpp\
    Modelo/LoopShaping/EstructuraDatos/listaordenada.cpp\
    DAO/adaptadorloopshapingdao.cpp\
    Modelo/EstructurasDatos/datosloopshaping.cpp\
    GUI/introducirtemplates.cpp\
    GUI/viewtemplates.cpp\
    GUI/verboundaries.cpp\
    Modelo/Boundaries/algoritmointerseccionlineal1D.cpp\
    Modelo/Boundaries/algoritmointerseccionproyectivo2D.cpp\
    Modelo/Boundaries/contour2.cpp \
    Librerias/qcustomplot/qcustomplot.cpp



HEADERS  += \
    Librerias/qcustomplot/qcustomplot.h \
    Librerias/interval/interval.h \
    Librerias/interval/operadores.h \
    Librerias/interval/cinterval.h \
    Modelo/EstructuraSistema/funciontransferencia.h\
    Modelo/LoopShaping/NaturalIntervalExtension/natural_interval_extension.h\
    DAO/adaptadorplantadao.h\
    Modelo/EstructuraSistema/sistema.h\
    Modelo/EstructuraSistema/polosceros.h\
    Modelo/EstructuraSistema/cpolinomios.h\
    Modelo/EstructuraSistema/kganancia.h\
    Modelo/EstructuraSistema/knganancia.h\
    #Modelo/EstructuraSistema/sistemasimplificada.h\
    #Modelo/EstructuraSistema/funciontransferenciasimplificada.h\
    #Modelo/EstructuraSistema/poloscerossimplificada.h\
    #Modelo/EstructuraSistema/kgananciasimplificada.h\
    GUI/diagramabode.h\
    GUI/windowsgeneral.h\
    GUI/introducirplanta.h\
    DAO/plantadao.h\
    GUI/intomega.h\
    GUI/intincertidumbre.h\
    Modelo/controlador.h\
    Modelo/EstructurasDatos/parlineedit.h\
    DAO/adaptadoromegadao.h\
    DAO/omegadao.h\
    Modelo/Objetos/incertidumbre.h\
    Modelo/Templates/templates.h\
    DAO/templatedao.h\
    DAO/adaptadortemplatedao.h\
    Modelo/Herramientas/tools.h\
    Modelo/Objetos/omega.h\
    DAO/bounddao.h\
    DAO/adaptadorbounddao.h\
    GUI/viewbound.h\
    Modelo/EstructurasDatos/datosplanta.h\
    DAO/dao.h\
    DAO/fdao.h\
    GUI/intdatosboundaries.h\
    Modelo/EstructurasDatos/datosbound.h\
    XmlParser/parserload.h\
    XmlParser/parsersave.h\
    GUI/intespecificaciones.h\
    DAO/especificacionesdao.h\
    DAO/adaptadorespecificacionesdao.h\
    GUI/introducirecontr.h\
    DAO/adaptadorcontroladordao.h\
    DAO/controladordao.h\
    Modelo/LoopShaping/algorithm_sachin.h\
    Modelo/LoopShaping/algorithm_isaac.h\
    #Modelo/LoopShaping/algorithm_isaac_hilo.h\
    Modelo/EstructurasDatos/var.h\
    Modelo/LoopShaping/algorithm_nandkishor.h\
    #Modelo/LoopShaping/algorithm_rambabu.h\
    #Modelo/LoopShaping/arbol_exp.h\
    GUI/viewboundreun.h\
    Modelo/Boundaries/boundaries.h\
    Modelo/Boundaries/contour.h\
    Consola/consola.h\
    Modelo/EstructuraSistema/formatolibre.h\
    GUI/intloopshaping.h\
    Modelo/LoopShaping/loopshaping.h\
    GUI/viewloopshaping.h\
    Modelo/LoopShaping/EstructuraDatos/avl.h\
    Modelo/LoopShaping/EstructuraDatos/tripleta.h\
    Modelo/LoopShaping/EstructuraDatos/nodo.h\
    Modelo/LoopShaping/DeteccionViolacionBoundaries/deteccionviolacionboundaries.h\
    #Modelo/LoopShaping/TestDeteccion.h\
    Modelo/LoopShaping/EstructuraDatos/listaordenada.h\
    DAO/loopshapingdao.h\
    DAO/adaptadorloopshapingdao.h\
    Modelo/EstructurasDatos/datosloopshaping.h\
    GUI/introducirtemplates.h\
    GUI/viewtemplates.h\
    GUI/verboundaries.h\
    Modelo/Boundaries/algoritmointerseccionlineal1D.h\
    Modelo/Boundaries/algoritmointerseccionproyectivo2D.h\
    Modelo/Boundaries/contour2.h \
    Modelo/LoopShaping/funcionescomunes.h



FORMS    += \ 
    GUI/diagramabode.ui \        
    GUI/intespecificaciones.ui \  
    GUI/intloopshaping.ui \  
    GUI/introducirecontr.ui \  
    GUI/introducirrango.ui \      
    GUI/verboundaries.ui \  
    GUI/viewbound.ui \        
    GUI/viewtemplates.ui \
    GUI/intdatosboundaries.ui \  
    GUI/intincertidumbre.ui \     
    GUI/intomega.ui \        
    GUI/introducirplanta.ui \  
    GUI/introducirtemplates.ui \  
    GUI/viewboundreun.ui \  
    GUI/viewloopshaping.ui \  
    GUI/windowsgeneral.ui

RESOURCES += \
    Imagenes/imagenesPlanta.qrc

contains (DEFINES, CUDA_AVAILABLE) {
    message (Se compila con soporte CUDA.)

    # Cuda sources
    CUDA_SOURCES += \
        GPU/CUDA/bnd_cuda.cu \
        GPU/CUDA/e_hull_cuda.cu

    # project build directories
    DESTDIR     = $$system(pwd)
    OBJECTS_DIR = $$DESTDIR/Obj
    # C++ flags
    QMAKE_CFLAGS_RELEASE =-O3
    QMAKE_CXXFLAGS_RELEASE =-O3

    # Path to header and libs files
    INCLUDEPATH  += $$CUDA_DIR/include \
        /usr/include/qt
    QMAKE_LIBDIR += $$CUDA_DIR/lib64   
    LIBS += -L $$CUDA_DIR/lib -lcudart -lcuda
    CUDA_ARCH     = sm_21
    NVCCFLAGS     = --compiler-options -fno-strict-aliasing -use_fast_math --ptxas-options=-v

    CUDA_INC = $$join(INCLUDEPATH,' -I','-I',' ')

    cuda.commands = $$CUDA_DIR/bin/nvcc -c -arch=$$CUDA_ARCH -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}

    cuda.dependency_type = TYPE_C
    cuda.depend_command = $$CUDA_DIR/bin/nvcc -M ${QMAKE_FILE_NAME}

    cuda.input = CUDA_SOURCES
    cuda.output = ${OBJECTS_DIR}${QMAKE_FILE_BASE}_cuda.o
    QMAKE_EXTRA_COMPILERS += cuda

} else {
    message (Se compila sin soporte para CUDA.)
}


# Inclusión de librerías




win32:CONFIG(release, debug|release): LIBS += -L$$PWD/Librerias/muparserx/build/ -lmuparserx
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/Librerias/muparserx/build/ -lmuparserxd

INCLUDEPATH += $$PWD/Librerias/muparserx/parser
DEPENDPATH += $$PWD/Librerias/muparserx/parser

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Librerias/muparserx/build/libmuparserx.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Librerias/muparserx/build/libmuparserxd.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Librerias/muparserx/build/muparserx.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Librerias/muparserx/build/muparserxd.lib

unix:!macx: LIBS += -L$$PWD/Librerias/qcustomplot/build/ -linterval

INCLUDEPATH += $$PWD/Librerias/qcustomplot
DEPENDPATH += $$PWD/Librerias/qcustomplot

unix:!macx: PRE_TARGETDEPS += $$PWD/Librerias/qcustomplot/build/libinterval.a

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/Librerias/interval/build/release/ -linterval
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/Librerias/interval/build/debug/ -linterval

INCLUDEPATH += $$PWD/Librerias/interval
DEPENDPATH += $$PWD/Librerias/interval

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Librerias/interval/build/release/libinterval.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Librerias/interval/build/debug/libinterval.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Librerias/interval/build/release/interval.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Librerias/interval/build/debug/interval.lib
