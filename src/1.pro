TARGET = 1
SOURCES += gridwidget.cpp aio.cpp
HEADERS += defs.h gridwidget.h
QT += core gui widgets
CONFIG += debug
win32 {
    CONFIG += console
}
unix {
    CONFIG += console
}