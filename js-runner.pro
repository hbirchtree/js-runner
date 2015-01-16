#-------------------------------------------------
#
# Project created by QtCreator 2015-01-09T12:18:43
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = js-runner
CONFIG   += console c++11 pthread
CONFIG   -= app_bundle

LIBS += -lSDL2

TEMPLATE = app


SOURCES += main.cpp

HEADERS += \
    keyaliases.h
