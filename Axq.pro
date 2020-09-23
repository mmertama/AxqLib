TEMPLATE        = subdirs
SUBDIRS         = lib quick rot unit figma

lib.file        = lib/Axqlib.pro
quick.file      = test/quick/QuickTest/QuickTest.pro
rot.file        = test/cpp/Rot13/Rot13.pro
unit.file       = test/cpp/unit/unit.pro
figma.file      = test/cpp/Figma/Figma.pro

rot.depends    = lib
quick.depends  = lib
unit.depends   = lib

