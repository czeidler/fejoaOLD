#include "cryptopluspluscryptointerface.h"

#include <string>

#include <QString>

#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/rsa.h>
using CryptoPP::RSA;


CryptoPlusPlusCryptoInterface::CryptoPlusPlusCryptoInterface()
{
}

WP::err CryptoPlusPlusCryptoInterface::generateKeyPair(QString &certificate, QString &publicKey, QString &privateKey, const SecureArray &keyPassword)
{
    RSA::PrivateKey rsaPrivateKey;
    rsaPrivateKey.GenerateRandomWithKeySize(randomGenerator, 2048);
    RSA::PublicKey rsaPublicKey(rsaPrivateKey);

    std::string buffer;
    CryptoPP::StringSink privateSink(buffer);
    rsaPrivateKey.Save(privateSink);
    privateKey.fromStdString(buffer);

    buffer = "";
    CryptoPP::StringSink publicSink(buffer);
    rsaPublicKey.Save(publicSink);
    publicKey.fromStdString(buffer);
    certificate.fromStdString(buffer);

    return WP::kOk;
}

SecureArray CryptoPlusPlusCryptoInterface::deriveKey(const SecureArray &secret, const QString &kdf, const QString &kdfAlgo, const SecureArray &salt, unsigned int keyLength, unsigned int iterations)
{
    CryptoPP::SecByteBlock derivedKey(CryptoPP::AES::DEFAULT_KEYLENGTH);

    CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA256> pbkdf;
    pbkdf.DeriveKey(derivedKey, derivedKey.size(), 0x00, (byte*)secret.data(), secret.size(), (byte*)salt.data(), salt.size(), iterations);

    SecureArray outKey;
    return outKey.setRawData((const char*)derivedKey.BytePtr(), derivedKey.size());
}

QByteArray CryptoPlusPlusCryptoInterface::generateSalt(const QString &value)
{
    return value.toLatin1();
}

QByteArray CryptoPlusPlusCryptoInterface::generateInitalizationVector(int size)
{
    CryptoPP::SecByteBlock data(CryptoPP::AES::DEFAULT_KEYLENGTH);
    randomGenerator.GenerateBlock(data, data.size());
    SecureArray outData;
    return outData.setRawData((const char*)data.BytePtr(), data.size());
}

SecureArray CryptoPlusPlusCryptoInterface::generateSymmetricKey(int size)
{
    return generateInitalizationVector(size);
}

QString CryptoPlusPlusCryptoInterface::generateUid()
{
    CryptoPP::SecByteBlock data(CryptoPP::AES::DEFAULT_KEYLENGTH);
    randomGenerator.GenerateBlock(data, data.size());

    CryptoPP::SHA1 sha1;
    std::string hash = "";
    CryptoPP::StringSource(data.BytePtr(), data.size(), true,
                           new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(
                                                        new CryptoPP::StringSink(hash))));
    QString out = hash.c_str();
    return out;
}
