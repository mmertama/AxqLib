QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS
SOURCES += \
        main.cpp \
    unittest.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include(../../../lib/Axq.pri)

win32:!contains(QMAKE_TARGET.arch, x86_64): DEFINES += A32BIT

HEADERS += \
    unittest.h

IPATH="\"$$join($$list($$clean_path($$INCLUDEPATH)), ";")\""
DEFINES+=INCLUDEPATH=$$shell_quote($$IPATH)

SRCLIST=
for(a, SOURCES):SRCLIST+=$${PWD}/$${a}
SPATH="\"$$join($$list($$clean_path($$SRCLIST)), ";")\""
DEFINES+=SOURCEPATH=$$shell_quote($$SPATH)

