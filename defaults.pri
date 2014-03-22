QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += network

INCLUDEPATH += $$PWD/support
SRC_DIR = $$PWD

#own git2:
INCLUDEPATH += $$PWD/../libgit2/include
LIBS += -L$$PWD/../libgit2/build
DEPENDPATH += $$PWD/../libgit2
