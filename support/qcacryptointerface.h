#ifndef QCACRYPTOINTERFACE_H
#define QCACRYPTOINTERFACE_H

#include "cryptointerface.h"

class QCACryptoInterface : public CryptoInterface
{
public:
    QCACryptoInterface();
    ~QCACryptoInterface();

    void generateKeyPair(const char* certificateFile, const char* publicKeyFile, const char* privateKeyFile, const char *keyPassword);

    WP::err generateKeyPair(QString &certificate, QString &publicKey,
                            QString &privateKey, const SecureArray &keyPassword);

    SecureArray deriveKey(const SecureArray &secret, const QString& kdf, const QString &kdfAlgo, const SecureArray &salt,
                                         unsigned int keyLength, unsigned int iterations);

    QByteArray generateSalt(const QString& value);
    QByteArray generateInitalizationVector(int size);
    SecureArray generateSymmetricKey(int size);
    QString generateUid();

    WP::err encryptSymmetric(const SecureArray &input, QByteArray &encrypted,
                             const SecureArray &key, const QByteArray &iv,
                             const char *algo = "aes256");
    WP::err decryptSymmetric(const QByteArray &input, SecureArray &decrypted,
                             const SecureArray &key, const QByteArray &iv,
                             const char *algo = "aes256");

    WP::err encyrptAsymmetric(const QByteArray &input, QByteArray &encrypted, const QString& certificate);
    WP::err decryptAsymmetric(const QByteArray &input, QByteArray &plain, const QString &privateKey,
                    const SecureArray &keyPassword, const QString& certificate);

    QByteArray sha1Hash(const QByteArray &string) const;
    QByteArray sha2Hash(const QByteArray &string) const;
    QString toHex(const QByteArray& string) const;

    WP::err sign(const QByteArray& input, QByteArray &signature, const QString &privateKeyString,
                 const SecureArray &keyPassword);
    bool verifySignatur(const QByteArray& message, const QByteArray &signature, const QString &publicKeyString);

    void generateDHParam(QString &prime, QString &base, QString &secret, QString &pub);
    SecureArray sharedDHKey(const QString &prime, const QString &base, const QString &secret);

private:
    class Private;
    Private* p;
};

#endif // QCACRYPTOINTERFACE_H
