#-------------------------------------------------
#
# Project created by QtCreator 2018-07-01T19:56:51
#
#-------------------------------------------------

QT       -= gui
QT       += quick concurrent

TARGET = Axq
TEMPLATE = lib
!dynlink:CONFIG += staticlib

dynlink: message (Dynamic build)
else: message(Static build)

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += AXQSHAREDLIB_LIBRARY

INCLUDEPATH +=  .. ../inc

HEADERS +=                      \
    ../axq.h                    \
    ../inc/axq_qml.h            \
    ../inc/axq_streams.h        \
    ../inc/axq_producer.h       \
    ../inc/axq_operators.h      \
    ../inc/axq_private.h \
    ../inc/axq_threads.h

SOURCES +=                      \
    ../src/axq_qml.cpp          \
    ../src/axq_streams.cpp      \
    ../src/axq_core.cpp         \
    ../src/axq_producers.cpp    \
    ../src/axq_operators.cpp    \
    ../src/axq_threads.cpp      \
    ../src/axq_private.cpp


unix {
    target.path = /usr/lib
    INSTALLS += target
}

win32{
    win32-msvc*:LIBS += Ws2_32.lib
}

INCLUDEPATH += inc

OTHER_FILES += Axq.pri Axq.md

CONFIG += c++11
