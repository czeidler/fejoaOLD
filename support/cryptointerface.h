#ifndef CRYPTOINTERFACE_H
#define CRYPTOINTERFACE_H

#include <QByteArray>

#include "error_codes.h"

typedef QByteArray SecureArray;

class CryptoInterface
{
public:
    virtual ~CryptoInterface() {}

    virtual WP::err generateKeyPair(QString &certificate, QString &publicKey,
                            QString &privateKey, const SecureArray &keyPassword) = 0;

    virtual SecureArray deriveKey(const SecureArray &secret, const QString& kdf, const QString &kdfAlgo, const SecureArray &salt,
                                         unsigned int keyLength, unsigned int iterations) = 0;

    virtual QByteArray generateSalt(const QString& value) = 0;
    virtual QByteArray generateInitalizationVector(int size) = 0;
    virtual SecureArray generateSymmetricKey(int size) = 0;
    virtual QString generateUid() = 0;

    virtual WP::err encryptSymmetric(const SecureArray &input, QByteArray &encrypted,
                             const SecureArray &key, const QByteArray &iv,
                             const char *algo = "aes256") = 0;
    virtual WP::err decryptSymmetric(const QByteArray &input, SecureArray &decrypted,
                             const SecureArray &key, const QByteArray &iv,
                             const char *algo = "aes256") = 0;

    virtual WP::err encyrptAsymmetric(const QByteArray &input, QByteArray &encrypted, const QString& certificate) = 0;
    virtual WP::err decryptAsymmetric(const QByteArray &input, QByteArray &plain, const QString &privateKey,
                    const SecureArray &keyPassword, const QString& certificate) = 0;

    virtual QByteArray sha1Hash(const QByteArray &string) const = 0;
    virtual QByteArray sha2Hash(const QByteArray &string) const = 0;
    virtual QString toHex(const QByteArray& string) const = 0;

    virtual WP::err sign(const QByteArray& input, QByteArray &signature, const QString &privateKeyString,
                 const SecureArray &keyPassword) = 0;
    virtual bool verifySignatur(const QByteArray& message, const QByteArray &signature, const QString &publicKeyString) = 0;

    virtual void generateDHParam(QString &prime, QString &base, QString &secret, QString &pub) = 0;
    virtual SecureArray sharedDHKey(const QString &prime, const QString &base, const QString &secret) = 0;
};


class CryptoInterfaceSingleton {
public:
    static CryptoInterface *getCryptoInterface();
    static void destroy();
private:
    static CryptoInterface *sCryptoInterface;
};

#endif // CRYPTOINTERFACE_H
