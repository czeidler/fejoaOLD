# Add more folders to ship with the application, here
#folder_01.source = qml/woodpidgin
#folder_01.target = qml
#DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
#QML_IMPORT_PATH =

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

include(../defaults.pri)

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
    messagereceiver.cpp \
    useridentity.cpp \
    profile.cpp \
    mainapplication.cpp \
    remotesync.cpp \
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
    gui/newmessageview.h \
    gui/infostatuswidget.h

FORMS += \
    gui/mainwindow.ui \
    gui/createprofiledialog.ui \
    gui/passworddialog.ui

LIBS += -L../support -lfejoa_support -lcryptopp -lgit2

INCLUDEPATH += gui

#OTHER_FILES += \
 #   qml/woodpidgin/main.qml
