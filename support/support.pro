include(../defaults.pri)

TARGET = fejoa_support
TEMPLATE = lib

SOURCES += \
    BigInteger/BigUnsigned.cpp \
    BigInteger/BigIntegerUtils.cpp \
    BigInteger/BigIntegerAlgorithms.cpp \
    BigInteger/BigInteger.cpp \
    BigInteger/BigUnsignedInABase.cc \
    cryptointerface.cpp \
    cryptoppcryptointerface.cpp \
    databaseinterface.cpp \
    databaseutil.cpp \
    diffmonitor.cpp \
    gitinterface.cpp \
    logger.cpp \
    protocolparser.cpp \
    remoteauthentication.cpp \
    remoteconnection.cpp \
    remoteconnectionmanager.cpp \
    remotestorage.cpp \
    remotesync.cpp \
    syncmanager.cpp \

HEADERS += \
    BigInteger/BigUnsigned.hh \
    BigInteger/BigIntegerUtils.hh \
    BigInteger/BigIntegerAlgorithms.hh \
    BigInteger/BigInteger.hh \
    BigInteger/NumberlikeArray.hh \
    BigInteger/BigUnsignedInABase.hh \
    cryptointerface.h \
    cryptoppcryptointerface.h \
    databaseinterface.h \
    databaseutil.h \
    error_codes.h \
    gitinterface.h \
    logger.h \
    protocolparser.h \
    diffmonitor.h \
    remoteauthentication.h \
    remoteconnection.h \
    remoteconnectionmanager.h \
    remotestorage.h \
    remotesync.h \
    syncmanager.h \
