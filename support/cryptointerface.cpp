#include "cryptointerface.h"

#include <stdio.h>

#include <QtCrypto/QtCrypto>

#include <BigInteger/BigIntegerAlgorithms.hh>
#include <BigInteger/BigIntegerUtils.hh>

class CryptoInterface::Private {
public:
    Private()
    {
        fInitializer = new QCA::Initializer;
    }

    ~Private()
    {
        delete fInitializer;
    }
public:
    QCA::Initializer* fInitializer;
};


CryptoInterface::CryptoInterface()
{
    p = new Private;
}

CryptoInterface::~CryptoInterface()
{
    delete p;
}

void CryptoInterface::generateKeyPair(const char* certificateFile, const char *publicKeyFile, const char *privateKeyFile, const char *keyPassword)
{
    if(!QCA::isSupported("pkey") || !QCA::PKey::supportedIOTypes().contains(QCA::PKey::RSA)) {
        printf("RSA not supported!\n");
        return;
    }

    QCA::PrivateKey secretKey = QCA::KeyGenerator().createRSA(2048);
    if(secretKey.isNull()) {
        printf("Failed to make private RSA key\n");
        return;
    }

    secretKey.toPEMFile(privateKeyFile, keyPassword);

    // public
    QCA::CertificateOptions opts;
    QCA::Certificate cert(opts, secretKey);

    cert.toPEMFile(certificateFile);

    QCA::PublicKey pubkey = secretKey.toPublicKey();
    pubkey.toPEMFile(publicKeyFile);
}

WP::err CryptoInterface::generateKeyPair(QString &certificate, QString &publicKey,
                                     QString &privateKey,
                                     const SecureArray &keyPassword)
{
    if(!QCA::isSupported("pkey") || !QCA::PKey::supportedIOTypes().contains(QCA::PKey::RSA)) {
        printf("RSA not supported!\n");
        return WP::kError;
    }

    QCA::PrivateKey secretKey = QCA::KeyGenerator().createRSA(2048);
    if(secretKey.isNull()) {
        printf("Failed to make private RSA key\n");
        return WP::kError;
    }

    privateKey = secretKey.toPEM(keyPassword);

    // public
    QCA::CertificateOptions opts;
    QCA::Certificate cert(opts, secretKey);

    certificate = cert.toPEM();

    QCA::PublicKey pubkey = secretKey.toPublicKey();
    publicKey = pubkey.toPEM();

    return WP::kOk;
}

SecureArray CryptoInterface::deriveKey(const SecureArray &secret, const QString &kdf, const QString &kdfAlgo, const SecureArray &salt, unsigned int keyLength, unsigned int iterations)
{
    QCA::SymmetricKey key;
    if (kdf == "pbkdf2") {
        QCA::PBKDF2 keyDerivationFunction(kdfAlgo);
        key = keyDerivationFunction.makeKey(secret, salt, keyLength, iterations);
    }
    return key.toByteArray();
}

QByteArray CryptoInterface::generateSalt(const QString &value)
{
    return QCA::InitializationVector(value.toLatin1()).toByteArray();
}

QByteArray CryptoInterface::generateInitalizationVector(int size)
{
    return QCA::InitializationVector(size).toByteArray();
}

SecureArray CryptoInterface::generateSymmetricKey(int size)
{
    return QCA::SymmetricKey(size).toByteArray();
}

QString CryptoInterface::generateUid()
{
    QByteArray uid = QCA::InitializationVector(512).toByteArray();
    QCA::Hash shaHash("sha1");
    shaHash.update(uid);
    return QCA::arrayToHex(shaHash.final().toByteArray());
}

WP::err CryptoInterface::encryptSymmetric(const SecureArray &input, QByteArray &encrypted,
                                      const SecureArray &key, const QByteArray &iv,
                                      const char *algo)
{
    QCA::Cipher encoder(algo, QCA::Cipher::CBC, QCA::Cipher::DefaultPadding, QCA::Encode, key, iv);
    encrypted = encoder.process(input).toByteArray();
    if (!encoder.ok())
        return WP::kError;
    return WP::kOk;
}

WP::err CryptoInterface::decryptSymmetric(const QByteArray &input, SecureArray &decrypted,
                                      const SecureArray &key, const QByteArray &iv,
                                      const char *algo)
{
    QCA::Cipher decoder(algo, QCA::Cipher::CBC, QCA::Cipher::DefaultPadding, QCA::Decode, key, iv);
    decrypted = decoder.process(input).toByteArray();
    if (!decoder.ok())
        return WP::kBadKey;
    return WP::kOk;
}

WP::err CryptoInterface::encyrptAsymmetric(const QByteArray &input, QByteArray &encrypted,
                                 const QString &certificate)
{
    // Read in a matching public key cert
    // you could also build this using the fromPEMFile() method
    QCA::ConvertResult convRes;
    QCA::Certificate pubCert = QCA::Certificate::fromPEM(certificate, &convRes);
    if (convRes != QCA::ConvertGood) {
        std::cout << "Sorry, could not import public key certificate" << std::endl;
        return (WP::err)convRes;
    }
    // We are building the certificate into a SecureMessageKey object, via a
    // CertificateChain
    QCA::SecureMessageKey secMsgKey;
    QCA::CertificateChain chain;
    chain += pubCert;
    secMsgKey.setX509CertificateChain(chain);

    // build up a SecureMessage object, based on our public key certificate
    QCA::CMS cms;
    QCA::SecureMessage msg(&cms);
    msg.setRecipient(secMsgKey);

    // Now use the SecureMessage object to encrypt the plain text.
    msg.startEncrypt();
    msg.update(input);
    msg.end();
    // I think it is reasonable to wait for 1 second for this
    msg.waitForFinished(1000);

    // check to see if it worked
    if(!msg.success()) {
        std::cout << "Error encrypting: " << msg.errorCode() << std::endl;
        return (WP::err)msg.errorCode();
    }

    // get the result
    encrypted = msg.read();
    return WP::kOk;
}

WP::err CryptoInterface::decryptAsymmetric(const QByteArray &input, QByteArray &plain,
                                 const QString &privateKey, const SecureArray &keyPassword,
                                 const QString &certificate)
{
    QCA::PrivateKey privKey;
    QCA::ConvertResult convRes;
    privKey = QCA::PrivateKey::fromPEM(privateKey, keyPassword, &convRes);
    if (convRes != QCA::ConvertGood) {
        std::cout << "Sorry, could not import Private Key" << std::endl;
        return (WP::err)convRes;
    }

    // Read in a matching public key cert
    // you could also build this using the fromPEMFile() method
    QCA::Certificate pubCert = QCA::Certificate::fromPEM(certificate, &convRes);
    if (convRes != QCA::ConvertGood) {
        std::cout << "Sorry, could not import public key certificate" << std::endl;
        return (WP::err)convRes;
    }
    // We are building the certificate into a SecureMessageKey object, via a
    // CertificateChain
    QCA::CertificateChain chain;
    chain += pubCert;

    QCA::SecureMessageKeyList skeys;
    QCA::SecureMessageKey skey;
    skey.setX509CertificateChain(chain);
    skey.setX509PrivateKey(privKey);
    skeys += skey;
    QCA::CMS sms;
    sms.setPrivateKeys(skeys);

    QCA::SecureMessage decryptedMessage(&sms);
    decryptedMessage.startDecrypt();
    decryptedMessage.update(input);
    decryptedMessage.end();
    decryptedMessage.waitForFinished(-1);

    plain = decryptedMessage.read();
    return WP::kOk;
}

QByteArray CryptoInterface::sha1Hash(const QByteArray &string) const
{
    QCA::Hash shaHash("sha1");
    shaHash.update(string);
    return shaHash.final().toByteArray();
}

QByteArray CryptoInterface::sha2Hash(const QByteArray &string) const
{
    QCA::Hash shaHash("sha256");
    shaHash.update(string);
    return shaHash.final().toByteArray();
}

QString CryptoInterface::toHex(const QByteArray &string) const
{
    return QCA::arrayToHex(string);
}

WP::err CryptoInterface::sign(const QByteArray &input, QByteArray &signature,
                          const QString &privateKeyString, const SecureArray &keyPassword)
{
    QCA::PrivateKey privateKey;
    QCA::ConvertResult convRes;
    privateKey = QCA::PrivateKey::fromPEM(privateKeyString, keyPassword, &convRes);
    if (convRes != QCA::ConvertGood) {
        std::cout << "Sorry, could not import Private Key" << std::endl;
        return WP::kError;
    }
    if(!privateKey.canSign()) {
        std::cout << "Error: this kind of key cannot sign" << std::endl;
        return WP::kError;
    }
    privateKey.startSign(QCA::EMSA3_MD5);
    privateKey.update(input);
    signature = privateKey.signature();
    return WP::kOk;
}

bool CryptoInterface::verifySignatur(const QByteArray &message, const QByteArray &signature,
                                     const QString &publicKeyString)
{
    QCA::PublicKey publicKey;
    QCA::ConvertResult convRes;
    publicKey = QCA::PublicKey::fromPEM(publicKeyString, &convRes);
    if (convRes != QCA::ConvertGood) {
        std::cout << "Sorry, could not import Public Key" << std::endl;
        return false;
    }
    // to check a signature, we must check that the key is
    // appropriate
    if(publicKey.canVerify()) {
        publicKey.startVerify(QCA::EMSA3_MD5);
        publicKey.update(message);
        if (publicKey.validSignature(signature) )
            return true;
        else
            return false;
    }
    return true;
}

void CryptoInterface::generateDHParam(QString &prime, QString &base, QString &secret, QString &pub)
{
    QCA::KeyGenerator generator;
    QCA::DLGroup group = generator.createDLGroup(QCA::DSA_512);
    prime = group.p().toString();
    base = group.g().toString();
    QCA::PrivateKey privateKey = generator.createDH(group);
    secret = privateKey.toDH().x().toString();
    pub = privateKey.toDH().y().toString();
}


SecureArray CryptoInterface::sharedDHKey(const QString &prime, const QString &base, const QString &secret)
{
    BigUnsigned primeNumber = stringToBigUnsigned(prime.toStdString());
    BigInteger baseNumber = stringToBigUnsigned(base.toStdString());
    BigUnsigned secretNumber = stringToBigUnsigned(secret.toStdString());
    BigUnsigned result = modexp(baseNumber, secretNumber, primeNumber);

    QByteArray key;

    BigUnsigned result2(result);
    while (result2 !=  0) {
         char rest = (result2 % 256).toUnsignedShort();
         key.prepend(rest);
         result2 = result2 / 256;
    }

    int size = key.size();
    return key;
}

CryptoInterface *CryptoInterfaceSingleton::sCryptoInterface = NULL;

CryptoInterface *CryptoInterfaceSingleton::getCryptoInterface()
{
    if (sCryptoInterface == NULL)
        sCryptoInterface = new CryptoInterface;
    return sCryptoInterface;
}

void CryptoInterfaceSingleton::destroy()
{
    delete sCryptoInterface;
    sCryptoInterface = NULL;
}
