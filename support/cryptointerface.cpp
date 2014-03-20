#include "cryptointerface.h"

#include "qcacryptointerface.h"


CryptoInterface *CryptoInterfaceSingleton::sCryptoInterface = NULL;

CryptoInterface *CryptoInterfaceSingleton::getCryptoInterface()
{
    if (sCryptoInterface == NULL)
        sCryptoInterface = new QCACryptoInterface;
    return sCryptoInterface;
}

void CryptoInterfaceSingleton::destroy()
{
    delete sCryptoInterface;
    sCryptoInterface = NULL;
}
