#include "cryptopluspluscryptointerface.h"

#include <string>

#include <QString>

#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/modes.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/rsa.h>
using CryptoPP::RSA;


CryptoPlusPlusCryptoInterface::~CryptoPlusPlusCryptoInterface()
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

WP::err CryptoPlusPlusCryptoInterface::encryptSymmetric(const SecureArray &input, QByteArray &encrypted, const SecureArray &key, const QByteArray &iv, const char *algo)
{
    CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption encryptor;
    encryptor.SetKeyWithIV((byte*)key.data(), key.size(), (byte*)iv.data(), iv.size());

    std::string cipher;
    CryptoPP::StreamTransformationFilter stf(encryptor, new CryptoPP::StringSink(cipher));
    stf.Put((byte*)input.data(), input.size());
    stf.MessageEnd();

    encrypted.setRawData(cipher.c_str(), cipher.size());
    return WP::kOk;
}

WP::err CryptoPlusPlusCryptoInterface::decryptSymmetric(const QByteArray &input, SecureArray &decrypted, const SecureArray &key, const QByteArray &iv, const char *algo)
{
    CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption decryptor;
    decryptor.SetKeyWithIV((byte*)key.data(), key.size(), (byte*)iv.data(), iv.size());

    std::string decryptedStd;
    CryptoPP::StreamTransformationFilter stf(decryptor, new CryptoPP::StringSink(decryptedStd));
    stf.Put((byte*)input.data(), input.size() );
    stf.MessageEnd();

    decrypted.setRawData(decryptedStd.c_str(), decryptedStd.size());
    return WP::kOk;
}

WP::err CryptoPlusPlusCryptoInterface::encyrptAsymmetric(const QByteArray &input, QByteArray &encrypted, const QString &certificate)
{
    CryptoPP::StringSource keySource((byte*)certificate.data(), certificate.size(), true);

    RSA::PublicKey rsaPublicKey;
    rsaPublicKey.Load(keySource);
    CryptoPP::RSAES_OAEP_SHA_Encryptor encryptor(rsaPublicKey);

    std::string cipher;
    CryptoPP::StringSource source((byte*)input.data(), input.size(), true,
                        new CryptoPP::PK_EncryptorFilter(randomGenerator, encryptor,
                                                         new CryptoPP::StringSink(cipher)));

    encrypted.setRawData(cipher.c_str(), cipher.size());
    return WP::kOk;
}

WP::err CryptoPlusPlusCryptoInterface::decryptAsymmetric(const QByteArray &input, QByteArray &plain, const QString &privateKey, const SecureArray &keyPassword, const QString &certificate)
{
    CryptoPP::StringSource privateKeySource((byte*)privateKey.data(), privateKey.size(), true);

    CryptoPP::RSAES_OAEP_SHA_Decryptor decoder(privateKeySource);

    std::string result;
    CryptoPP::StringSource((byte*)input.data(), input.size(), true,
                           new CryptoPP::PK_DecryptorFilter(randomGenerator, decoder, new CryptoPP::StringSink(result)));
    plain.setRawData(result.c_str(), result.size());
    return WP::kOk;
}

QByteArray CryptoPlusPlusCryptoInterface::sha1Hash(const QByteArray &string) const
{
    CryptoPP::SHA1 sha1;
    std::string hash = "";
    CryptoPP::StringSource((byte*)string.data(), string.size(), true,
                           new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(
                                                        new CryptoPP::StringSink(hash))));
    QByteArray out;
    return out.setRawData(hash.data(), hash.size());
}

QByteArray CryptoPlusPlusCryptoInterface::sha2Hash(const QByteArray &string) const
{
    CryptoPP::SHA256 sha;
    std::string hash = "";
    CryptoPP::StringSource((byte*)string.data(), string.size(), true,
                           new CryptoPP::HashFilter(sha, new CryptoPP::HexEncoder(
                                                        new CryptoPP::StringSink(hash))));
    QByteArray out;
    return out.setRawData(hash.data(), hash.size());
}

QString CryptoPlusPlusCryptoInterface::toHex(const QByteArray &string) const
{
    std::string encoded;
    CryptoPP::StringSource source((byte*)string.data(), string.size(), true,
                        new CryptoPP::HexEncoder(new CryptoPP::StringSink(encoded)));
    QString out;
    out.fromStdString(encoded);
    return out;
}

WP::err CryptoPlusPlusCryptoInterface::sign(const QByteArray &input, QByteArray &signature, const QString &privateKeyString, const SecureArray &keyPassword)
{
    CryptoPP::StringSource privateKeySource((byte*)privateKeyString.data(), privateKeyString.size(), true);
    CryptoPP::RSASSA_PKCS1v15_SHA_Signer signer(privateKeySource);

    std::string signatureStd;
    CryptoPP::StringSource((byte*)input.data(), input.size(), true,
                           new CryptoPP::SignerFilter(randomGenerator, signer,
                                                      new CryptoPP::StringSink(signatureStd)));
    signature.setRawData(signatureStd.c_str(), signatureStd.size());
    return WP::kOk;
}

bool CryptoPlusPlusCryptoInterface::verifySignatur(const QByteArray &message, const QByteArray &signature, const QString &publicKeyString)
{
    try {
        CryptoPP::StringSource publicKeySoruce((byte*)publicKeyString.data(), publicKeyString.size(), true);
        CryptoPP::RSASSA_PKCS1v15_SHA_Verifier verifier(publicKeySoruce);

        std::string combinedMessage;
        combinedMessage = QString(message + signature).toStdString();
        CryptoPP::StringSource(combinedMessage, true,
                               new CryptoPP::SignatureVerificationFilter(
                                   verifier, NULL,
                                   CryptoPP::SignatureVerificationFilter::THROW_EXCEPTION));
    } catch (CryptoPP::Exception& e) {
        return false;
    } catch (...) {
        return false;
    }
    return true;
}

void CryptoPlusPlusCryptoInterface::generateDHParam(QString &prime, QString &base, QString &secret, QString &pub)
{
    // todo implement
}

SecureArray CryptoPlusPlusCryptoInterface::sharedDHKey(const QString &prime, const QString &base, const QString &secret)
{
    // todo implement
    return QByteArray();
}
