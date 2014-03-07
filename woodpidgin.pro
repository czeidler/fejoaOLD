# Add more folders to ship with the application, here
#folder_01.source = qml/woodpidgin
#folder_01.target = qml
#DEPLOYMENTFOLDERS = folder_01

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += network

# Additional import path used to resolve QML modules in Creator's code model
#QML_IMPORT_PATH =

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
    messagereceiver.cpp \
    useridentity.cpp \
    profile.cpp \
    mainapplication.cpp \
    remotesync.cpp \
    remotestorage.cpp \
    remoteconnection.cpp \
    remoteauthentication.cpp \
    mailbox.cpp \
    syncmanager.cpp \
    contactrequest.cpp \
    contact.cpp \
    mailmessenger.cpp \
    mail.cpp \
    messagethreaddatamodel.cpp \
    gui/createprofiledialog.cpp \
    gui/mainwindow.cpp \
    gui/messageview.cpp \
    gui/passworddialog.cpp \
    gui/threadview.cpp \
    gui/useridentityview.cpp \
    support/cryptointerface.cpp \
    support/databaseinterface.cpp \
    support/databaseutil.cpp \
    support/gitinterface.cpp \
    support/logger.cpp \
    support/protocolparser.cpp \
    support/BigInteger/BigUnsigned.cpp \
    support/BigInteger/BigIntegerUtils.cpp \
    support/BigInteger/BigIntegerAlgorithms.cpp \
    support/BigInteger/BigInteger.cpp \
    support/BigInteger/BigUnsignedInABase.cc \
    gui/newmessageview.cpp \
    gui/infostatuswidget.cpp

# Installation path
# target.path =

HEADERS += \
    messagereceiver.h \
    useridentity.h \
    profile.h \
    mainapplication.h \
    remotesync.h \
    remotestorage.h \
    remoteconnection.h \
    remoteauthentication.h \
    mailbox.h \
    syncmanager.h \
    contactrequest.h \
    contact.h \
    mailmessenger.h \
    mail.h \
    messagethreaddatamodel.h \
    gui/createprofiledialog.h \
    gui/mainwindow.h \
    gui/messageview.h \
    gui/passworddialog.h \
    gui/threadview.h \
    gui/useridentityview.h \
    support/cryptointerface.h \
    support/databaseinterface.h \
    support/databaseutil.h \
    support/error_codes.h \
    support/gitinterface.h \
    support/logger.h \
    support/protocolparser.h \
    support/BigInteger/BigUnsigned.hh \
    support/BigInteger/BigIntegerUtils.hh \
    support/BigInteger/BigIntegerAlgorithms.hh \
    support/BigInteger/BigInteger.hh \
    support/BigInteger/NumberlikeArray.hh \
    support/BigInteger/BigUnsignedInABase.hh \
    gui/newmessageview.h \
    gui/infostatuswidget.h
FORMS += \
    gui/mainwindow.ui \
    gui/createprofiledialog.ui \
    gui/passworddialog.ui

unix: LIBS += -L$$PWD/../libgit2/build -lgit2 -lz

INCLUDEPATH += $$PWD/../libgit2/include gui support
DEPENDPATH += $$PWD/../libgit2

#unix: PRE_TARGETDEPS += $$PWD/../libgit2/.a

#unix|win32: LIBS += -lgit2
unix|win32: LIBS += -lqca

#OTHER_FILES += \
 #   qml/woodpidgin/main.qml
