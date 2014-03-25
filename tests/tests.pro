#-------------------------------------------------
#
# Project created by QtCreator 2014-03-21T08:37:34
#
#-------------------------------------------------

include(../defaults.pri)

QT       += testlib
QT       -= gui

TARGET = fejoatest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += \
    fejoatest.cpp

LIBS += -L$$PWD/../../build-fejoa-Desktop-Debug/support/ -lfejoa_support
LIBS += -L/user/lib -lcryptopp -lgit2
