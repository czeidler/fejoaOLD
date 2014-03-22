include(../defaults.pri)

TARGET = fejoa_support
TEMPLATE = lib

SOURCES += databaseinterface.cpp \
    databaseutil.cpp \
    gitinterface.cpp \
    logger.cpp \
    protocolparser.cpp \
    BigInteger/BigUnsigned.cpp \
    BigInteger/BigIntegerUtils.cpp \
    BigInteger/BigIntegerAlgorithms.cpp \
    BigInteger/BigInteger.cpp \
    BigInteger/BigUnsignedInABase.cc \
    cryptointerface.cpp \
    cryptoppcryptointerface.cpp \
    diffmonitor.cpp \
    remoteconnection.cpp \
    remoteauthentication.cpp \
    remoteconnectionmanager.cpp \
    remotestorage.cpp \

HEADERS += \
    cryptointerface.h \
    databaseinterface.h \
    databaseutil.h \
    error_codes.h \
    gitinterface.h \
    logger.h \
    protocolparser.h \
    BigInteger/BigUnsigned.hh \
    BigInteger/BigIntegerUtils.hh \
    BigInteger/BigIntegerAlgorithms.hh \
    BigInteger/BigInteger.hh \
    BigInteger/NumberlikeArray.hh \
    BigInteger/BigUnsignedInABase.hh \
    cryptoppcryptointerface.h \
    diffmonitor.h \
    remoteconnectionmanager.h \
    remoteconnection.h \
    remoteauthentication.h \
    remotestorage.h \
