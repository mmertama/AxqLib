# AxqLib

## Axq provides Functional Reactive programming approach with Qt.

The obvious question is "Why", and besides the just an obvious answer "For Fun", the Reactice Programming concept is an interesting paradigm not too much used within compiled languages. However that would make writing reactive/responsive applications easier - not just UI, but also things like networking, robotics IoT etc. Axq provides a Qt C++ API, but also a implements Javascript interface for QML apps. 

This is a version 1.0 and likely to be a bit premature, sorry about bugs, they are not intentational, I have fixed all I have found as well as all peculiarities worth of fix along with performance issues found.

[Axq API documentation](Axq.md)

Axq has been tested on (Ubuntu) Linux, OSX, Windows MSCV17 and Windows MinGW32.

Axq has been published under LGPL License v3

Normal Qt/platform build procedures (issues) for dynamic library apply, there are not any configure or make install magic helping you, yet.

In Linux you may have to call
```
export LD_LIBRARY_PATH=/<path-to-build>/build-Axq-Desktop_<your-Qt-version>/lib
```

###### Copyright Markus Mertama 2018

