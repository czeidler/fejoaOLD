#include <QString>
#include <QtTest>

#include "cryptointerface.h"

class FejoaTest : public QObject
{
    Q_OBJECT

public:
    FejoaTest();

private Q_SLOTS:
    void testCyrptoInterface();
};

FejoaTest::FejoaTest()
{
}

void FejoaTest::testCyrptoInterface()
{
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();

    QString privateKey;
    QString publicKey;
    QString certificate;
    WP::err error = crypto->generateKeyPair(certificate, publicKey, privateKey, "");
    QVERIFY2(error == WP::kOk, "key pair generation");

    const char *kTestString = "Fejoa encryption test string.";
    QByteArray input = kTestString;
    QByteArray encrypted;
    error = crypto->encyrptAsymmetric(input, encrypted, certificate);
    QVERIFY2(error == WP::kOk, "asymmetric encryption");

    QByteArray plain;
    error = crypto->decryptAsymmetric(encrypted, plain, privateKey, "", certificate);
    QVERIFY2(error == WP::kOk, "asymmetric decryption");

    QVERIFY2(plain == kTestString, "asymmetric decrypted text == plain?");

    QByteArray signature;
    error = crypto->sign(input, signature, privateKey, "");
    QVERIFY2(error == WP::kOk, "signing");

    bool verified = crypto->verifySignatur(input, signature, publicKey);
    QVERIFY2(verified == true, "verifing");

    // symmetric encryption
    QByteArray symmetricKey = crypto->generateSymmetricKey(256);
    QByteArray iv = crypto->generateInitalizationVector(256);
    error = crypto->encryptSymmetric(input, encrypted, symmetricKey, iv);
    QVERIFY2(error == WP::kOk, "symmetric encryption");

    error = crypto->decryptSymmetric(encrypted, plain, symmetricKey, iv);
    QVERIFY2(error == WP::kOk, "symmetric decryption");

    QVERIFY2(plain == kTestString, "symmetric decrypted text == plain?");
}

QTEST_APPLESS_MAIN(FejoaTest)

#include "fejoatest.moc"
