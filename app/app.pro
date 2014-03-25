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
SOURCES += \
    contact.cpp \
    contactrequest.cpp \
    main.cpp \
    mainapplication.cpp \
    mail.cpp \
    mailbox.cpp \
    mailmessenger.cpp \
    messagereceiver.cpp \
    messagethreaddatamodel.cpp \
    gui/createprofiledialog.cpp \
    gui/infostatuswidget.cpp \
    gui/mainwindow.cpp \
    gui/messageview.cpp \
    gui/newmessageview.cpp \
    gui/passworddialog.cpp \
    gui/threadview.cpp \
    gui/useridentityview.cpp \
    profile.cpp \
    useridentity.cpp \

# Installation path
# target.path =

HEADERS += \
    contact.h \
    contactrequest.h \
    mail.h \
    mailbox.h \
    mailmessenger.h \
    mainapplication.h \
    messagereceiver.h \
    messagethreaddatamodel.h \
    gui/createprofiledialog.h \
    gui/infostatuswidget.h \
    gui/mainwindow.h \
    gui/messageview.h \
    gui/newmessageview.h \
    gui/passworddialog.h \
    gui/threadview.h \
    gui/useridentityview.h \
    profile.h \
    useridentity.h \

FORMS += \
    gui/mainwindow.ui \
    gui/createprofiledialog.ui \
    gui/passworddialog.ui

LIBS += -L../support -lfejoa_support -lcryptopp -lgit2

INCLUDEPATH += gui

#OTHER_FILES += \
 #   qml/woodpidgin/main.qml
