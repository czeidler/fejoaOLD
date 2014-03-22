#include "cryptoppcryptointerface.h"

#include <string>

#include <QDebug>
#include <QString>

#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/modes.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/rsa.h>

using namespace CryptoPP;


CryptoPPCryptoInterface::~CryptoPPCryptoInterface()
{
}

WP::err CryptoPPCryptoInterface::generateKeyPair(QString &certificate, QString &publicKey, QString &privateKey, const SecureArray &keyPassword)
{
    try {
        CryptoPP::RSAES_OAEP_SHA_Decryptor decryptor(randomGenerator, 2048 /*, e */);
        CryptoPP::RSAES_OAEP_SHA_Encryptor encryptor(decryptor);

        std::string privateKeyStd;
        decryptor.AccessKey().Save(
                 CryptoPP::HexEncoder(
                    new CryptoPP::StringSink(privateKeyStd)
                 ).Ref()
              );
        privateKey = QString::fromStdString(privateKeyStd);

        std::string publicKeyStd;
        encryptor.AccessKey().Save(
                 CryptoPP::HexEncoder(
                    new CryptoPP::StringSink(publicKeyStd)
                 ).Ref()
              );
        publicKey = QString::fromStdString(publicKeyStd);
        certificate = QString::fromStdString(publicKeyStd);
        /*
        RSA::PrivateKey rsaPrivateKey;
        rsaPrivateKey.GenerateRandomWithKeySize(randomGenerator, 2048);
        RSA::PublicKey rsaPublicKey(rsaPrivateKey);

        std::string buffer;
        rsaPrivateKey.Save(StringSink(buffer).Ref());
        std::string bufferHex;
        StringSource(buffer, true, new HexEncoder(new StringSink(bufferHex)));
        privateKey = QString::fromStdString(bufferHex);

        buffer = "";
        bufferHex = "";
        rsaPublicKey.Save(StringSink(buffer).Ref());

        StringSource(buffer, true, new HexEncoder(new StringSink(bufferHex)));

        publicKey = QString::fromStdString(bufferHex);
        certificate = QString::fromStdString(bufferHex);*/
    } catch (Exception& e) {
        qDebug() << "CryptoPP::Exception caught: "<< e.what() << endl;
        return WP::kError;
    } catch (...) {
        return WP::kError;
    }

    return WP::kOk;
}

SecureArray CryptoPPCryptoInterface::deriveKey(const SecureArray &secret, const QString &kdf, const QString &kdfAlgo, const SecureArray &salt, unsigned int keyLength, unsigned int iterations)
{
    SecByteBlock derivedKey(AES::DEFAULT_KEYLENGTH);
    try {
        PKCS5_PBKDF2_HMAC<SHA256> pbkdf;
        pbkdf.DeriveKey(derivedKey, derivedKey.size(), 0x00, (byte*)secret.data(), secret.size(), (byte*)salt.data(), salt.size(), iterations);
    } catch (Exception& e) {
        qDebug() << "CryptoPP::Exception caught: "<< e.what() << endl;
        return SecureArray();
    } catch (...) {
        return SecureArray();
    }
    SecureArray outKey;
    return outKey.append((const char*)derivedKey.BytePtr(), derivedKey.size());
}

QByteArray CryptoPPCryptoInterface::generateSalt(const QString &value)
{
    return value.toLatin1();
}

QByteArray CryptoPPCryptoInterface::generateInitalizationVector(int size)
{
    SecByteBlock data(AES::DEFAULT_KEYLENGTH);
    randomGenerator.GenerateBlock(data, data.size());
    SecureArray outData;
    return outData.append((const char*)data.BytePtr(), data.size());
}

SecureArray CryptoPPCryptoInterface::generateSymmetricKey(int size)
{
    return generateInitalizationVector(size);
}

QString CryptoPPCryptoInterface::generateUid()
{
    SecByteBlock data(AES::DEFAULT_KEYLENGTH);
    randomGenerator.GenerateBlock(data, data.size());

    SHA1 sha1;
    std::string hash = "";
    StringSource(data.BytePtr(), data.size(), true,
                           new HashFilter(sha1, new HexEncoder(
                                                        new StringSink(hash))));
    QString out = hash.c_str();
    return out;
}

WP::err CryptoPPCryptoInterface::encryptSymmetric(const SecureArray &input, QByteArray &encrypted, const SecureArray &key, const QByteArray &iv, const char *algo)
{
    std::string cipher;
    try {
        CTR_Mode<AES>::Encryption encryptor;
        encryptor.SetKeyWithIV((byte*)key.data(), key.size(), (byte*)iv.data(), iv.size());

        StreamTransformationFilter stf(encryptor, new StringSink(cipher));
        stf.Put((byte*)input.data(), input.size());
        stf.MessageEnd();
    } catch (Exception& e) {
        qDebug() << "CryptoPP::Exception caught: "<< e.what() << endl;
        return WP::kError;
    } catch (...) {
        return WP::kError;
    }

    encrypted.clear();
    encrypted.append(cipher.c_str(), cipher.size());
    return WP::kOk;
}

WP::err CryptoPPCryptoInterface::decryptSymmetric(const QByteArray &input, SecureArray &decrypted, const SecureArray &key, const QByteArray &iv, const char *algo)
{
    std::string decryptedStd;
    try {
        CTR_Mode<AES>::Decryption decryptor;
        decryptor.SetKeyWithIV((byte*)key.data(), key.size(), (byte*)iv.data(), iv.size());

        StreamTransformationFilter stf(decryptor, new StringSink(decryptedStd));
        stf.Put((byte*)input.data(), input.size() );
        stf.MessageEnd();
    } catch (Exception& e) {
        qDebug() << "CryptoPP::Exception caught: "<< e.what() << endl;
        return WP::kError;
    } catch (...) {
        return WP::kError;
    }
    decrypted.clear();
    decrypted.append(decryptedStd.c_str(), decryptedStd.size());
    return WP::kOk;
}

WP::err CryptoPPCryptoInterface::encyrptAsymmetric(const QByteArray &input, QByteArray &encrypted, const QString &certificate)
{
    std::string cipher;
    try {
        StringSource keySource((byte*)certificate.toLatin1().data(), certificate.size(), true, new HexDecoder());
        ByteQueue byteQueue;
        keySource.TransferAllTo(byteQueue);
        byteQueue.MessageEnd();

        RSA::PublicKey rsaPublicKey;
        rsaPublicKey.Load(byteQueue);
        RSAES_OAEP_SHA_Encryptor encryptor(rsaPublicKey);

        StringSource((byte*)input.data(), input.size(), true,
                            new PK_EncryptorFilter(randomGenerator, encryptor,
                                                             new StringSink(cipher)));
    } catch (Exception& e) {
        qDebug() << "CryptoPP::Exception caught: "<< e.what() << endl;
        return WP::kError;
    } catch (...) {
        return WP::kError;
    }

    encrypted.clear();
    encrypted.append(cipher.c_str(), cipher.size());
    return WP::kOk;
}

WP::err CryptoPPCryptoInterface::decryptAsymmetric(const QByteArray &input, QByteArray &plain, const QString &privateKey, const SecureArray &keyPassword, const QString &certificate)
{
    std::string result;
    try {
        StringSource keySource((byte*)privateKey.toLatin1().data(), privateKey.size(), true, new HexDecoder());
        ByteQueue byteQueue;
        keySource.TransferTo(byteQueue);
        byteQueue.MessageEnd();

        RSAES_OAEP_SHA_Decryptor decoder(byteQueue);

        StringSource((byte*)input.data(), input.size(), true,
                               new PK_DecryptorFilter(randomGenerator, decoder, new StringSink(result)));
    } catch (Exception& e) {
        qDebug() << "CryptoPP::Exception caught: "<< e.what() << endl;
        return WP::kError;
    } catch (...) {
        return WP::kError;
    }
    plain.clear();
    plain.append(result.c_str(), result.size());
    return WP::kOk;
}

QByteArray CryptoPPCryptoInterface::sha1Hash(const QByteArray &string) const
{
    SHA1 sha1;
    std::string hash = "";
    StringSource((byte*)string.data(), string.size(), true,
                           new HashFilter(sha1, new HexEncoder(
                                                        new StringSink(hash))));
    QByteArray out;
    return out.append(hash.data(), hash.size());
}

QByteArray CryptoPPCryptoInterface::sha2Hash(const QByteArray &string) const
{
    SHA256 sha;
    std::string hash = "";
    StringSource((byte*)string.data(), string.size(), true,
                           new HashFilter(sha, new HexEncoder(
                                                        new StringSink(hash))));
    QByteArray out;
    return out.append(hash.data(), hash.size());
}

QString CryptoPPCryptoInterface::toHex(const QByteArray &string) const
{
    std::string encoded;
    StringSource source((byte*)string.data(), string.size(), true,
                        new HexEncoder(new StringSink(encoded)));
    return QString::fromStdString(encoded);
}

WP::err CryptoPPCryptoInterface::sign(const QByteArray &input, QByteArray &signature,
                                      const QString &privateKeyString,
                                      const SecureArray &keyPassword)
{
    std::string signatureStd;
    try {
        StringSource keySource((byte*)privateKeyString.toLatin1().data(), privateKeyString.size(),
                                      true, new HexDecoder());
        ByteQueue byteQueue;
        keySource.TransferTo(byteQueue);
        byteQueue.MessageEnd();

        RSASSA_PKCS1v15_SHA_Signer signer(byteQueue);

        StringSource((byte*)input.data(), input.size(), true,
                               new SignerFilter(randomGenerator, signer,
                                                          new StringSink(signatureStd)));
    } catch (Exception& e) {
        qDebug() << "CryptoPP::Exception caught: "<< e.what() << endl;
        return WP::kError;
    } catch (...) {
        return WP::kError;
    }
    signature.clear();
    signature.append(signatureStd.c_str(), signatureStd.size());
    return WP::kOk;
}

bool CryptoPPCryptoInterface::verifySignatur(const QByteArray &message, const QByteArray &signature, const QString &publicKeyString)
{
    try {
        StringSource keySource((byte*)publicKeyString.toLatin1().data(), publicKeyString.size(), true, new HexDecoder());
        ByteQueue byteQueue;
        keySource.TransferTo(byteQueue);
        byteQueue.MessageEnd();

        RSASSA_PKCS1v15_SHA_Verifier verifier(byteQueue);

        QByteArray data;
        data.append(message);
        data.append(signature);
        StringSource((byte*)data.data(), data.size(), true,
                               new SignatureVerificationFilter(
                                   verifier, NULL,
                                   SignatureVerificationFilter::THROW_EXCEPTION));
    } catch (Exception& e) {
        qDebug() << "CryptoPP::Exception caught: "<< e.what() << endl;
        return false;
    } catch (...) {
        return false;
    }
    return true;
}

void CryptoPPCryptoInterface::generateDHParam(QString &prime, QString &base, QString &secret, QString &pub)
{
    // todo implement
}

SecureArray CryptoPPCryptoInterface::sharedDHKey(const QString &prime, const QString &base, const QString &secret)
{
    // todo implement
    return QByteArray();
}
