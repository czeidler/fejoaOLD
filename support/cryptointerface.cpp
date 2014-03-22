#include "cryptointerface.h"

#include "cryptoppcryptointerface.h"


CryptoInterface *CryptoInterfaceSingleton::sCryptoInterface = NULL;

CryptoInterface *CryptoInterfaceSingleton::getCryptoInterface()
{
    if (sCryptoInterface == NULL)
        sCryptoInterface = new CryptoPPCryptoInterface();
    return sCryptoInterface;
}

void CryptoInterfaceSingleton::destroy()
{
    delete sCryptoInterface;
    sCryptoInterface = NULL;
}
