INCLUDEPATH += $$PWD/..

CONFIG +=c++11

win32:CONFIG(release, debug|release): LIBS += -L$$shadowed($$PWD)/release -lAxq
else:win32:CONFIG(debug, debug|release): LIBS += -L$$shadowed($$PWD)/debug -lAxq
else:unix:!macx: LIBS += -L$$shadowed($$PWD) -lAxq
else:macx: LIBS += -L$$shadowed($$PWD) -lAxq
else: error(Not defined)

INCLUDEPATH += \
            $$PWD/../../../libs \
            $$PWD/..

DEPENDPATH += $$PWD/../../../lib

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$shadowed($$PWD)/release/libAxq.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$shadowed($$PWD)/debug/libAxq.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$shadowed($$PWD)/release/Axq.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$shadowed($$PWD)/debug/Axq.lib
else:unix:!macx: PRE_TARGETDEPS += $$shadowed($$PWD)/libAxq.so
else:macx:PRE_TARGETDEPS += $$shadowed($$PWD)/libAxq.dylib
else: error(Not defined)


win32:!win32-g++:DEFINES*=NOMINMAX #MSVC problem with std::min/max
